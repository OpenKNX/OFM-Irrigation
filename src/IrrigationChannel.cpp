#include "OpenKNX.h"
#include "IrrigationChannel.h"
#include "IrrigationModule.h"


IrrigationChannel::IrrigationChannel(uint8_t iChannelNumber) 
{
    _channelIndex = iChannelNumber;
}

IrrigationChannel::~IrrigationChannel() 
{
}

const std::string IrrigationChannel::name()
{
    return "IrrigationChannel";
}

uint16_t IrrigationChannel::getYearDay()
{
    if (!openknx.time.isValid())
    {
        return 0;
    }

    tm tmNow;
    openknx.time.getLocalTime().toTm(tmNow);
    return static_cast<uint16_t>(tmNow.tm_yday + 1);
}

// Bodenfeuchte-Wert gilt nur, wenn mindestens alle 24 h ein Telegramm kommt (Sensorausfall erkennen)
static constexpr uint32_t BODENFEUCHTE_TIMEOUT_MS = 24UL * 60UL * 60UL * 1000UL;

// will be called once a KO received a telegram
void IrrigationChannel::processInputKo(GroupObject &iKo)
{
    if (!_channelActive)
    {
       //logDebugP("processInputKo: channel %u not active", _channelIndex); // nur debug
        return;
    }

    int32_t idx = IRR_KoCalcIndex(iKo.asap());
    if (idx < 0) return; // KO gehoert zu einem anderen Kanal oder Modul
    logDebugP("[channel]processInputKo: channel %u", _channelIndex);
    logIndentUp();
    switch (idx)
    {
        case IRR_KoChNiederschlagsrate:
        {
            _Niederschlagsrate_Zone = KoIRR_ChNiederschlagsrate.value(DPT_Value_Temp);
            logDebugP("processInputKo: Niederschlagsrate_Zone=%.2f", _Niederschlagsrate_Zone);
            break;
        }
        case IRR_KoChSchwellwert:
        {
            _Schwellwert_P_Prozent_Zone = KoIRR_ChSchwellwert.value(DPT_Scaling);
            logDebugP("processInputKo: Schwellwert_P_Prozent_Zone=%u", _Schwellwert_P_Prozent_Zone);
            break;
        }
        case IRR_KoChnFK: // Nutzbare Feldkapazität (nFK) [mm]    
        {
            _nutzbareFeldkapazitaet_nFK_Zone = KoIRR_ChnFK.value(DPT_Value_Temp);
            logDebugP("processInputKo: nutzbareFeldkapazitaet_nFK_Zone=%.2f", _nutzbareFeldkapazitaet_nFK_Zone);
            break;
        }
        case IRR_KoChKc: // Kc-Faktor der Zone  
        {
            _Kulturfaktor_Kc_Zone = KoIRR_ChKc.value(DPT_Value_1_Ucount);
            logDebugP("processInputKo: Kulturfaktor_Kc_Zone=%u", _Kulturfaktor_Kc_Zone);
            break;
        }
        case IRR_KoChZonenSperre:
        {
            _Sperre_Zone = KoIRR_ChZonenSperre.value(DPT_Switch);
            logDebugP("processInputKo: Zonensperre=%u", _Sperre_Zone);
            break;
        }
        case IRR_KoChStatusMagnetventil:
        {
            bool offen = KoIRR_ChStatusMagnetventil.value(DPT_Switch);
            onStatusMagnetventilChanged(offen);
            break;
        }
        case IRR_KoChBodenfeuchte:
        {
            _bodenfeuchteProzent = KoIRR_ChBodenfeuchte.value(DPT_Value_Temp);
            _bodenfeuchteGueltig = true;
            _bodenfeuchteEmpfang_ms = millis();
            logDebugP("processInputKo: Bodenfeuchte=%.1f%%", _bodenfeuchteProzent);
            break;
        }
   
        default:
            logDebugP("default case processInputKo: unknown KO index %d", idx);
            break;   
    }
    logIndentDown();
}

void IrrigationChannel::processAfterStartupDelay()
{
    // start after device global delay
    if (_channelActive)
    {
        logDebugP("processAfterStartupDelayChannel %u: channel active", _channelIndex);
        if (!KoIRR_ChZonenSperre.initialized())
        {
            KoIRR_ChZonenSperre.requestObjectRead();
        }
    }
}


void IrrigationChannel::loop()
{
    if (!_channelActive) return;

    // Bodenfeuchte veraltet? Dann ignorieren. Die Pruefung laeuft staendig, damit der Wert
    // vor dem Ueberlauf von millis() (nach ca. 49 Tagen) ungueltig wird.
    if (_bodenfeuchteGueltig && (uint32_t)(millis() - _bodenfeuchteEmpfang_ms) > BODENFEUCHTE_TIMEOUT_MS)
    {
        _bodenfeuchteGueltig = false;
        logInfoP("Kanal %u: Bodenfeuchte seit %u h nicht aktualisiert - Sensor wird ignoriert", _channelIndex, BODENFEUCHTE_TIMEOUT_MS / (1000 * 60 * 60));
    }

    if (_ZonenStatus == ZonenStatus::Laeuft)
    {
        uint32_t laufSek = (millis() - _kommandoStartMillis) / 1000;
        if (laufSek >= _ermittelteLaufzeit_sekunden)
        {
            KoIRR_ChVentilansteuerung.value(false, DPT_Switch);
            _rueckmeldungStartMillis = millis();
            setZonenStatus(ZonenStatus::WartetAufRueckmeldung);
            logDebugP("Kanal %u: WartetAufRueckmeldung", _channelIndex);
        }
    }

    if (_ZonenStatus == ZonenStatus::WartetAufRueckmeldung)
    {
        //Timeout abhandeln
        if (((uint32_t)(millis() - _rueckmeldungStartMillis)/1000) >= _ventil_Rueckmeldung_timeout_s)
        {
            //nochmal zur Sicherheit abschalten 
            KoIRR_ChVentilansteuerung.value(false, DPT_Switch);
            setZonenStatus(ZonenStatus::TimeoutFehler);
            logDebugP( "Kanal %u: Timeout Ventil-Rueckmeldung", _channelIndex);

            // wir gehen von aus, dass Wasser geflossen ist, daher Bodenwasserkonto anpassen
            const float Wassermenge_mm = calc_Zugefuehrte_Wassermenge((float)_ermittelteLaufzeit_sekunden, _Niederschlagsrate_Zone);
            _Wasserbilanzkonto = calc_Bodenwasserkonto_final(_Wasserbilanzkonto, Wassermenge_mm, _nutzbareFeldkapazitaet_nFK_Zone);
            KoIRR_ChWasserbilanzkonto.value(_Wasserbilanzkonto, DPT_Value_Temp);
        }
    }

}

void IrrigationChannel::setup() 
{
    _channelActive = (ParamIRR_ChActive == 1) && (ParamIRR_ChSuspended == 0);
    if (!_channelActive) 
    {
        logDebugP("Channel %u: not active!", _channelIndex);
        return;
    }

    _Niederschlagsrate_Zone = ParamIRR_CHNiederschlagsrateValue;
    _Schwellwert_P_Prozent_Zone = ParamIRR_CHSchwellwertValue;
    _nutzbareFeldkapazitaet_nFK_Zone = ParamIRR_CHnFKValue;
    _Kulturfaktor_Kc_Zone = ParamIRR_CHKcValue;

    if (_kontoWiederhergestellt) // falls Flash-Werte wiederhergestellt wurden, dann das Konto ggf. anpassen
    {
        _Wasserbilanzkonto = MIN(_Wasserbilanzkonto, _nutzbareFeldkapazitaet_nFK_Zone); // falls nFK verkleinert wurde
    } 
    else
    {    
        _Wasserbilanzkonto = _nutzbareFeldkapazitaet_nFK_Zone; // Start optimistisch
    }
    setKOInitialValues(); 
}


bool IrrigationChannel::isActive()
{
    return _channelActive; // Gibt den Aktivitätsstatus des Kanals zurück
}

void IrrigationChannel::setKOInitialValues(void)
{
    if (openknxIrrigationModule.debug())
    {
        logDebugP("[INIT] KO Initial Values gesetzt");
    }
}

void IrrigationChannel::save()
{
    openknx.flash.writeFloat(_Wasserbilanzkonto);
    openknx.flash.writeByte(static_cast<uint8_t>(_ZonenStatus));
    logDebugP("saved: Wasserbilanzkonto=%f Status=%u", _Wasserbilanzkonto, static_cast<uint8_t>(_ZonenStatus));
}

void IrrigationChannel::restore()
{
    _Wasserbilanzkonto = openknx.flash.readFloat();
    ZonenStatus restoredStatus = static_cast<ZonenStatus>(openknx.flash.readByte());

    if (restoredStatus == ZonenStatus::Laeuft || restoredStatus == ZonenStatus::WartetAufRueckmeldung)
    {
        // Zustand mitten in einer laufenden Bewässerung ist nach einem Neustart
        // nicht mehr sicher zuzuordnen (kein gültiger Zeitstempel) - sicherheitshalber
        // zurücksetzen und Ventil explizit aus.
        setZonenStatus(ZonenStatus::Inaktiv);
        KoIRR_ChVentilansteuerung.value(false, DPT_Switch);
        logDebugP("restored: unterbrochene Bewaesserung erkannt, Status zurueckgesetzt, Ventil aus");
    }
    else
    {
        setZonenStatus(restoredStatus);
    }
    _kontoWiederhergestellt = true;
    logDebugP("restored: Wasserbilanzkonto=%f Status=%u", _Wasserbilanzkonto, static_cast<uint8_t>(_ZonenStatus));
}

bool IrrigationChannel::istGesperrt(bool sperreGlobal)
{
    // ---- Bodenfeuchtesensor, Modus 2: zusätzliche Sicherheitsbedingung ----
    bool bodenfeuchteSperrtBewaesserung = false;
    if ((_bodenfeuchteGueltig) && (ParamIRR_ChBodenfeuchteVerwendung == PT_BodenfeuchteVerwendung::Sicherheit) && (_bodenfeuchteProzent >= (float)ParamIRR_ChSperrschwelleBodenfeuchte))
    {
        bodenfeuchteSperrtBewaesserung = true;
    }

    return _Sperre_Zone || sperreGlobal || bodenfeuchteSperrtBewaesserung;
}

// Wird bei Fensteroeffnung aufgerufen: Konto buchen, Bedarf und Laufzeit bestimmen
void IrrigationChannel::process_Bewaesserungsberechnung_channel(float et0Gestern, float regenmengeGestern, float regenmengeHeute, bool Sperre_global)
{
    if (!_channelActive) return;

    // 1. Gebuchtes Konto: Gestern-Werte
    // Tageswechsel von Modulebene berechnet ET0 und triggert das hier
    // jetzt berechnen wir den Bedarf, anhand von ETc, den Bodenwasserkonto, der Feldkapazität (nFK), Schwellwert

    float ETc_gestern = calc_ETc(et0Gestern, _Kulturfaktor_Kc_Zone);
    _Wasserbilanzkonto = calc_Bodenwasserkonto(_Wasserbilanzkonto, regenmengeGestern, ETc_gestern, _nutzbareFeldkapazitaet_nFK_Zone);
    
    // 2. Bodenfeuchtesensor, Modus 1: Konto korrigieren ----
    bool korrigiert = false;
    if (_bodenfeuchteGueltig && ParamIRR_ChBodenfeuchteVerwendung == PT_BodenfeuchteVerwendung::Korrektur)
    {
        float gemessenesKonto = (_bodenfeuchteProzent / 100.0f) * _nutzbareFeldkapazitaet_nFK_Zone;
        logDebugP("Kanal %u: Konto durch Bodenfeuchte korrigiert: %.2f -> %.2f", _channelIndex, _Wasserbilanzkonto, gemessenesKonto);
        _Wasserbilanzkonto = gemessenesKonto;
        korrigiert = true;
    }
    
    // 3. Effektives Konto fuer die Entscheidung: Regen seit Mitternacht zaehlt mit, wird aber nicht gebucht
    //    (er wird morgen als Gestern-Regen gebucht, so entsteht keine Doppelzaehlung)
    const float regenHeuteAnrechnung = korrigiert ? 0.0f : regenmengeHeute;
    const float kontoEffektiv = MIN(_Wasserbilanzkonto + regenHeuteAnrechnung, _nutzbareFeldkapazitaet_nFK_Zone);



    _Schwellwert_in_mm_Zone = calc_Schwellwert_in_mm(_Schwellwert_P_Prozent_Zone, _nutzbareFeldkapazitaet_nFK_Zone);
    _Bewaesserungsbedarf = calc_bedarf (kontoEffektiv, _Schwellwert_in_mm_Zone);

    // sende das berechnete auf den Bus
    KoIRR_ChWasserbilanzkonto.value(_Wasserbilanzkonto, DPT_Value_Temp);
    KoIRR_ChBedarf.value(_Bewaesserungsbedarf, DPT_Switch);


    _Diagnose_Bewaesserung_gesperrt = istGesperrt(Sperre_global);
    sende_Diagnose_KO_Bewaesserung_gesperrt();
    if (_Diagnose_Bewaesserung_gesperrt)
    {
        logDebugP("Kanal %u: Bewaesserung gesperrt (Sperre_global=%d Zone=%d Bodenfeuchte=%.1f%%, BodenfeuchteGueltig=%d)", _channelIndex, Sperre_global, _Sperre_Zone, _bodenfeuchteProzent, _bodenfeuchteGueltig);
    }
    _ermittelteFehlmenge_mm = 0.0f;
    _ermittelteLaufzeit_sekunden = 0;
    
    if (_Bewaesserungsbedarf == true && _Diagnose_Bewaesserung_gesperrt == false)
    {
        _ermittelteFehlmenge_mm = calc_Fehlmenge_mm(_nutzbareFeldkapazitaet_nFK_Zone, kontoEffektiv);
        _ermittelteLaufzeit_sekunden = calc_laufzeit_sek(_ermittelteFehlmenge_mm, _Niederschlagsrate_Zone);

        KoIRR_ChFehlmenge.value(_ermittelteFehlmenge_mm, DPT_Value_Temp);
        KoIRR_ChLaufzeit.value(_ermittelteLaufzeit_sekunden, DPT_TimePeriodSec);

        if (_ermittelteLaufzeit_sekunden == 0)
        {
            // Bedarf vorhanden, aber nicht bewaesserbar (z. B. Niederschlagsrate <= 0 oder Fehlmenge minimal)
            if (_Niederschlagsrate_Zone <= 0.0f)
                logErrorP("Kanal %u: Niederschlagsrate <= 0, Laufzeit nicht berechenbar - Zone wird uebersprungen", _channelIndex);
            else
                logDebugP("Kanal %u: Laufzeit unter Mindestlaufzeit (Fehlmenge=%.3f mm) - Zone wird uebersprungen", _channelIndex, _ermittelteFehlmenge_mm);
            setZonenStatus(ZonenStatus::Inaktiv);
        }
        else
        {
            // Bewaesserungsstart vormerken 
            setZonenStatus(ZonenStatus::WartetAufStart);
        }
    }
    else
    {
        setZonenStatus(ZonenStatus::Inaktiv);
        // nichts zu tun - naechster Vergleich wieder morgen
    }
    logDebugP("Kanal %u: ETc_gestern=%.2f Konto=%.2f RegenHeute=%.2f Schwellwert[mm]=%.2f gesperrt=%d",
              _channelIndex, ETc_gestern, _Wasserbilanzkonto, regenHeuteAnrechnung, _Schwellwert_in_mm_Zone, _Diagnose_Bewaesserung_gesperrt);
    logDebugP("Bedarf=%i Fehlmenge[mm]=%.2f Laufzeit[s]=%u", _Bewaesserungsbedarf, _ermittelteFehlmenge_mm, _ermittelteLaufzeit_sekunden);
}


bool IrrigationChannel::get_lokaleSperre()
{
    return _Sperre_Zone;
}

// ============================================================
// 3. Kultur-/Zonen-Evapotranspiration ETc
// ============================================================
float IrrigationChannel::calc_ETc(float ET0, uint8_t Kc)
{
    // ETc [mm/Tag]
    float_t ETc = ET0 * ((float)Kc / 100.0f);
    return ETc;
}

// ============================================================
// 4. Bodenwasserkonto
// ============================================================
float IrrigationChannel::calc_Bodenwasserkonto(float konto_alt, float niederschlag_mm ,float ETc, float nutzbareFeldkapazitaet)
{
    // Neues Bodenwasserkonto [mm]
    float_t Bodenwasserkonto = konto_alt + niederschlag_mm - ETc;

    // Begrenzung auf 0 ... nFK
    Bodenwasserkonto = MAX(0.0f, MIN(Bodenwasserkonto, nutzbareFeldkapazitaet));
    return Bodenwasserkonto;
}
// ============================================================
// 5. Bewässerungsschwelle
// ============================================================
float IrrigationChannel::calc_Schwellwert_in_mm (uint8_t Schwellwert_Prozent, float nutzbareFeldkapazitaet)
{
    // Schwellwert [mm]
    float schwellwert_in_mm = ((float)Schwellwert_Prozent/100.0f) * nutzbareFeldkapazitaet;
    return schwellwert_in_mm;
}
// ============================================================
// 6. Bewässerungsbedarf
// ============================================================

// true  = Bewässerung erforderlich
// false = keine Bewässerung erforderlich
bool IrrigationChannel::calc_bedarf (float Bodenwasserkonto_neu, float schwellwert_in_mm)
{
    bool bedarf = false; //kein Bedarf
    if (Bodenwasserkonto_neu < schwellwert_in_mm)
    {
        bedarf = true;
    }
    return bedarf;
}

float IrrigationChannel::calc_Fehlmenge_mm ( float nutzbareFeldkapazitaet, float Bodenwasserkonto_neu)
{
    float Fehlmenge=nutzbareFeldkapazitaet-Bodenwasserkonto_neu;
    return Fehlmenge;
}



// ============================================================
// 9. Bewässerungs-Laufzeit
// ============================================================
static constexpr uint32_t MIN_LAUFZEIT_S = 60;
uint16_t IrrigationChannel::calc_laufzeit_sek(float fehlmenge_mm, float niederschlagsrate_mm_h)
{
    if (niederschlagsrate_mm_h <= 0.0f)
    {
        return 0;
    }

    float laufzeitSek = (fehlmenge_mm / niederschlagsrate_mm_h) * 3600.0f;
    if (laufzeitSek < MIN_LAUFZEIT_S) // berechenete Laufzeiten unter 60s werden nicht ausgeführt, da die Ventile/Pumpen sonst zu oft geschaltet werden.
    {
        return 0;
    }

    if (laufzeitSek > 65535.0f)
    {
        return 65535;
    }

    return static_cast<uint16_t>(laufzeitSek);
}
// ============================================================
// 10. Rückbuchung der Bewässerung
// ============================================================
float IrrigationChannel::calc_Zugefuehrte_Wassermenge (float laufzeit_sek, float niederschlagsrate_mm_h)
{
    // Zugeführte Wassermenge [mm]
    float_t bewaesserung_mm = (laufzeit_sek / 3600.0f) * niederschlagsrate_mm_h;
    return bewaesserung_mm;
}

float IrrigationChannel::calc_Bodenwasserkonto_final(float Bodenwasserkonto_neu, float bewaesserung_mm, float NutzbareFeldkapazitaet_nFK )
{
    // Bodenwasserkonto nach Bewässerung [mm]
    float Bodenwasserkonto_final = Bodenwasserkonto_neu + bewaesserung_mm;

    // Sicherheitshalber wieder auf 0 ... nFK begrenzen
    Bodenwasserkonto_final = MAX(0.0f, MIN(Bodenwasserkonto_final, NutzbareFeldkapazitaet_nFK));
    return Bodenwasserkonto_final;
}

void IrrigationChannel::onStatusMagnetventilChanged(bool offen)
{
    if (offen == _statusMagnetventilLetzter) return;
    _statusMagnetventilLetzter = offen;

    if (offen) return; // steigende Flanke ist hier nicht mehr relevant

    // fallende Flanke: nur auswerten, wenn wir GENAU darauf warten
    if (_ZonenStatus != ZonenStatus::WartetAufRueckmeldung) return;

    uint32_t gemesseneLaufzeitSek = (millis() - _kommandoStartMillis) / 1000;
    float zugefuehrteMm = calc_Zugefuehrte_Wassermenge((float)gemesseneLaufzeitSek, _Niederschlagsrate_Zone);
    _Wasserbilanzkonto = calc_Bodenwasserkonto_final(_Wasserbilanzkonto, zugefuehrteMm, _nutzbareFeldkapazitaet_nFK_Zone);

    KoIRR_ChWasserbilanzkonto.value(_Wasserbilanzkonto, DPT_Value_Temp);
    _Bewaesserungsbedarf = false;
    KoIRR_ChBedarf.value(_Bewaesserungsbedarf, DPT_Switch);

    setZonenStatus(ZonenStatus::Abgeschlossen);
    _letzterBewaesserungsTag = getYearDay();
    logDebugP("Kanal %u: Abgeschlossen, Konto final=%.2f", _channelIndex, _Wasserbilanzkonto);
}

bool IrrigationChannel::hatOffenenBedarf() const
{
    return _ZonenStatus == ZonenStatus::WartetAufStart;
}

bool IrrigationChannel::laeuftGerade() const
{
    // belegt einen "Kompatibilitäts-Slot", solange die Zone nicht sicher
    // wieder zu ist - auch während sie schon auf die Rückmeldung wartet
    return _ZonenStatus == ZonenStatus::Laeuft || _ZonenStatus == ZonenStatus::WartetAufRueckmeldung;
}

void IrrigationChannel::starteBewaesserung(uint16_t maxLaufzeitMinuten, bool sperreGlobal)
{
    if (_ZonenStatus != ZonenStatus::WartetAufStart) return; // Schutz vor Fehlaufrufen

    // Sperren unmittelbar vor dem Start pruefen (koennen sich seit der Berechnung geaendert haben)
    if (istGesperrt(sperreGlobal))
    {
        logDebugP("Kanal %u: Start abgebrochen, Sperre aktiv", _channelIndex);
        _Diagnose_Bewaesserung_gesperrt = true;
        sende_Diagnose_KO_Bewaesserung_gesperrt();
        setZonenStatus(ZonenStatus::Inaktiv);
        return;
    }

    // nach Reboot: Status "WartetAufStart" kommt aus dem Flash, die Laufzeit nicht -> neu rechnen
    if (_ermittelteLaufzeit_sekunden == 0)
    {
        _ermittelteFehlmenge_mm = calc_Fehlmenge_mm(_nutzbareFeldkapazitaet_nFK_Zone, _Wasserbilanzkonto);
        _ermittelteLaufzeit_sekunden = calc_laufzeit_sek(_ermittelteFehlmenge_mm, _Niederschlagsrate_Zone);
        logDebugP("Kanal %u: Laufzeit neu berechnet: %us", _channelIndex, _ermittelteLaufzeit_sekunden);
    }

    if (_ermittelteLaufzeit_sekunden == 0)
    {
        logDebugP("Kanal %u: Start abgebrochen, Laufzeit = 0s", _channelIndex);
        setZonenStatus(ZonenStatus::Inaktiv);
        return;
    }

    // Laufzeit auf Restzeit des Fensters kuerzen (0 = keine Begrenzung)
    const uint32_t maxLaufzeitSekunden = maxLaufzeitMinuten * 60UL;
    if (maxLaufzeitSekunden > 0 && _ermittelteLaufzeit_sekunden > maxLaufzeitSekunden)
    {
        logDebugP("Kanal %u: Laufzeit gekuerzt %us -> %us (Fensterende)",
                  _channelIndex, _ermittelteLaufzeit_sekunden, (uint16_t)maxLaufzeitSekunden);
        _ermittelteLaufzeit_sekunden = (uint16_t)maxLaufzeitSekunden;
        KoIRR_ChLaufzeit.value(_ermittelteLaufzeit_sekunden, DPT_TimePeriodSec);
    }

    setZonenStatus(ZonenStatus::Laeuft);
    _kommandoStartMillis = millis();
    KoIRR_ChVentilansteuerung.value(true, DPT_Switch);
    logDebugP("Kanal %u: Laeuft (geplante Laufzeit=%us)", _channelIndex, _ermittelteLaufzeit_sekunden);
}

// Bricht eine laufende Bewaesserung ab. Die Rueckbuchung erfolgt ueber die normale
// Rueckmeldung (oder den Timeout) mit der real gelaufenen Zeit.
void IrrigationChannel::stoppeBewaesserung()
{
    if (_ZonenStatus != ZonenStatus::Laeuft) return;

    const uint32_t laufSek = (millis() - _kommandoStartMillis) / 1000;
    if (laufSek < _ermittelteLaufzeit_sekunden)
        _ermittelteLaufzeit_sekunden = (uint16_t)laufSek; // Ist-Laufzeit, nicht die geplante

    KoIRR_ChVentilansteuerung.value(false, DPT_Switch);
    KoIRR_ChLaufzeit.value(_ermittelteLaufzeit_sekunden, DPT_TimePeriodSec);
    _rueckmeldungStartMillis = millis();
    setZonenStatus(ZonenStatus::WartetAufRueckmeldung);
    logInfoP("Kanal %u: Bewaesserung gestoppt nach %us", _channelIndex, _ermittelteLaufzeit_sekunden);
}

void IrrigationChannel::pruefeSperreWaehrendLauf(bool sperreGlobal)
{
    if (!_channelActive) return;
    if (_ZonenStatus != ZonenStatus::Laeuft) return;
    if (!istGesperrt(sperreGlobal)) return;

    logInfoP("Kanal %u: Sperre waehrend des Laufs - stoppe Bewaesserung", _channelIndex);
    _Diagnose_Bewaesserung_gesperrt = true;
    sende_Diagnose_KO_Bewaesserung_gesperrt();
    stoppeBewaesserung();
}

void IrrigationChannel::verwerfeOffenenBedarf()
{
    if (_ZonenStatus != ZonenStatus::WartetAufStart) return;
    setZonenStatus(ZonenStatus::Inaktiv);
    logDebugP("Kanal %u: Bedarf im Fenster nicht bedient, wird morgen neu berechnet", _channelIndex);
}

String IrrigationChannel::wandle_Zonenstatus_in_Text(ZonenStatus status)
{
    switch (status)
    {
        case ZonenStatus::Inaktiv:
            return "Inaktiv";
        case ZonenStatus::WartetAufStart:
            return "WartetAufStart";
        case ZonenStatus::Laeuft:
            return "Laeuft";
        case ZonenStatus::WartetAufRueckmeldung:
            return "WartAufRckmld"; // max 14 Zeichen
        case ZonenStatus::Abgeschlossen:
            return "Abgeschlossen";
        case ZonenStatus::TimeoutFehler:
            return "TimeoutFehler";
        default:
            return "Unbekannt";
    }
}

void IrrigationChannel::setZonenStatus(ZonenStatus status)
{
    if (_ZonenStatus == status) return;// Ausgabe nur wenn es sich ändert

    _ZonenStatus = status;
    String statusText = wandle_Zonenstatus_in_Text(_ZonenStatus);
    KoIRR_ChZonenStatus.value(statusText.c_str(), DPT_String_8859_1);
}

void IrrigationChannel::sende_Diagnose_KO_Bewaesserung_gesperrt()
{
    KoIRR_ChDiagnoseBewaesserunggesperrt.value(_Diagnose_Bewaesserung_gesperrt, DPT_Switch);
}

void IrrigationChannel::aktualisiereDiagnose(bool sperreGlobal)
{
    if (!_channelActive) return;
    const bool neu = istGesperrt(sperreGlobal);
    if (neu != _Diagnose_Bewaesserung_gesperrt)
    {
        _Diagnose_Bewaesserung_gesperrt = neu;
        sende_Diagnose_KO_Bewaesserung_gesperrt();
    }
}