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

uint16_t IrrigationChannel::getYearDay() const
{
    if (!openknx.time.isValid())
    {
        return 0;
    }

    tm tmNow;
    openknx.time.getLocalTime().toTm(tmNow);
    return static_cast<uint16_t>(tmNow.tm_yday + 1);
}

// will be called once a KO received a telegram
void IrrigationChannel::processInputKo(GroupObject &iKo)
{
    if (!_channelActive)
    {
       //logDebugP("processInputKo: channel %u not active", _channelIndex); // nur debug
        return;
    }

    logIndentUp();
    logDebugP("[channel]processInputKo: channel %u", _channelIndex);
  
    switch (IRR_KoCalcIndex(iKo.asap()))
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
            _Kulturfaktor_Kc_Zone = KoIRR_ChKc.value(DPT_Value_Temp);
            logDebugP("processInputKo: Kulturfaktor_Kc_Zone=%.2f", _Kulturfaktor_Kc_Zone);
            break;
        }
        case IRR_KoChZonenSperre:
        {
            _Sperre_Zone = KoIRR_ChZonenSperre.value(DPT_Enable);
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
            logDebugP("processInputKo: Bodenfeuchte=%.1f%%", _bodenfeuchteProzent);
            break;
        }
   
        default:
            logDebugP("default case processInputKo: unknown KO index %u", IRR_KoCalcIndex(iKo.asap()));
            break;   
    }
    logIndentDown();
}

void IrrigationChannel::loop()
{
    if (!_channelActive) return;

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
            //nocchmal zur Sicherheit abschalten 
            KoIRR_ChVentilansteuerung.value(false, DPT_Switch);
            setZonenStatus(ZonenStatus::TimeoutFehler);
            logDebugP( "Kanal %u: Timeout Ventil-Rueckmeldung", _channelIndex);
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
    _Kulturfaktor_Kc_Zone = ParamIRR_CHKcValue / 10.0f;
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

    logDebugP("restored: Wasserbilanzkonto=%f Status=%u", _Wasserbilanzkonto, static_cast<uint8_t>(_ZonenStatus));
}


void IrrigationChannel::process_Bewaesserungsberechnung_channel(float et0Gestern, float regenmengeGestern, bool Sperre_global)
{
    if (!_channelActive) return;

    // Tageswechsel von Modulebene berechnet ET0 und triggert das hier
    // jetzt berechnen wir den Bedarf, anhand von ETc, den Bodenwasserkonto, der Feldkapazität (nFK), Schwellwert

    float ETc_gestern = calc_ETc(et0Gestern, _Kulturfaktor_Kc_Zone);
    
    _Wasserbilanzkonto = calc_Bodenwasserkonto(_Wasserbilanzkonto, regenmengeGestern, ETc_gestern, _nutzbareFeldkapazitaet_nFK_Zone);
    
    // ---- Bodenfeuchtesensor, Modus 1: Konto korrigieren ----
    if (_bodenfeuchteGueltig && ParamIRR_ChBodenfeuchteVerwendung == PT_BodenfeuchteVerwendung::Korrektur)
    {
        float gemessenesKonto = (_bodenfeuchteProzent / 100.0f) * _nutzbareFeldkapazitaet_nFK_Zone;
        logDebugP("Kanal %u: Konto durch Bodenfeuchte korrigiert: %.2f -> %.2f", _channelIndex, _Wasserbilanzkonto, gemessenesKonto);
        _Wasserbilanzkonto = gemessenesKonto;
    }
    
    _Schwellwert_in_mm_Zone = calc_Schwellwert_in_mm((float)_Schwellwert_P_Prozent_Zone, _nutzbareFeldkapazitaet_nFK_Zone);
    _Bewaesserungsbedarf = calc_bedarf (_Wasserbilanzkonto, _Schwellwert_in_mm_Zone);

    // sende das berechnete auf den Bus
    KoIRR_ChWasserbilanzkonto.value(_Wasserbilanzkonto, DPT_Value_Temp);
    KoIRR_ChBedarf.value(_Bewaesserungsbedarf, DPT_Switch);

    // ---- Bodenfeuchtesensor, Modus 2: zusätzliche Sicherheitsbedingung ----
    bool bodenfeuchteSperrtBewaesserung = false;
    if (_bodenfeuchteGueltig && ParamIRR_ChBodenfeuchteVerwendung == PT_BodenfeuchteVerwendung::Sicherheit && _bodenfeuchteProzent >= (float)ParamIRR_ChSperrschwelleBodenfeuchte)
    {
        bodenfeuchteSperrtBewaesserung = true;
        logDebugP("Kanal %u: Bewaesserung durch Bodenfeuchte gesperrt (%.1f%% >= %u%%)",
                _channelIndex, _bodenfeuchteProzent, ParamIRR_ChSperrschwelleBodenfeuchte);
    }

    _Diagnose_Bewaesserung_gesperrt = _Sperre_Zone || Sperre_global || bodenfeuchteSperrtBewaesserung;
    
    if (_Bewaesserungsbedarf == true && _Diagnose_Bewaesserung_gesperrt == false)
    {
        _ermittelteFehlmenge_mm = calc_Fehlmenge_mm(_nutzbareFeldkapazitaet_nFK_Zone, _Wasserbilanzkonto);
        _ermittelteLaufzeit_sekunden = calc_laufzeit_sek(_ermittelteFehlmenge_mm, _Niederschlagsrate_Zone);

        if (_ermittelteLaufzeit_sekunden == 0)
        {
            // Bedarf vorhanden, aber nicht bewaesserbar (z. B. Niederschlagsrate <= 0 oder Fehlmenge minimal)
            if (_Niederschlagsrate_Zone <= 0.0f)
                logErrorP("Kanal %u: Niederschlagsrate <= 0, Laufzeit nicht berechenbar - Zone wird uebersprungen", _channelIndex);
            else
                logDebugP("Kanal %u: Laufzeit = 0s (Fehlmenge=%.3f mm) - Zone wird uebersprungen", _channelIndex, _ermittelteFehlmenge_mm);

            KoIRR_ChFehlmenge.value(_ermittelteFehlmenge_mm, DPT_Value_Temp);
            KoIRR_ChLaufzeit.value((uint16_t)0, DPT_TimePeriodSec);
            setZonenStatus(ZonenStatus::Inaktiv);
        }
        else
        {
            // Bewaesserungsstart vormerken und Werte auf den Bus senden
            KoIRR_ChFehlmenge.value(_ermittelteFehlmenge_mm, DPT_Value_Temp);
            KoIRR_ChLaufzeit.value(_ermittelteLaufzeit_sekunden, DPT_TimePeriodSec);
            setZonenStatus(ZonenStatus::WartetAufStart);
        }
    }
    else
    {
        setZonenStatus(ZonenStatus::Inaktiv);
        // nichts zu tun - naechster Vergleich wieder morgen
    }
    logDebugP("Kanal %u: ETc_gestern=%.2f Wasserbilanzkonto=%.2f Schwellwert[mm]=%f", _channelIndex, ETc_gestern, _Wasserbilanzkonto, _Schwellwert_in_mm_Zone);
    logDebugP("Bedarf = %i Fehlmenge[mm]=%f notw_Laufzeit[s]=%i", _Bewaesserungsbedarf , _ermittelteFehlmenge_mm, _ermittelteLaufzeit_sekunden);
}


bool IrrigationChannel::get_lokaleSperre()
{
    return _Sperre_Zone;
}

// ============================================================
// 3. Kultur-/Zonen-Evapotranspiration ETc
// ============================================================
float IrrigationChannel::calc_ETc(float ET0, float Kc)
{
    // ETc [mm/Tag]
    float_t ETc = ET0 * Kc;
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

void IrrigationChannel::starteBewaesserung(uint32_t maxLaufzeitSekunden)
{
    if (_ZonenStatus != ZonenStatus::WartetAufStart) return; // Schutz vor Fehlaufrufen

    if (_ermittelteLaufzeit_sekunden == 0)
    {
        logDebugP("Kanal %u: Start abgebrochen, Laufzeit = 0s", _channelIndex);
        setZonenStatus(ZonenStatus::Inaktiv);
        return;
    }

    // Laufzeit auf Restzeit des Fensters kuerzen (0 = keine Begrenzung)
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
            return "WartetAufRueckmeldung";
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
    if (_ZonenStatus == status) return;

    _ZonenStatus = status;
    String statusText = wandle_Zonenstatus_in_Text(_ZonenStatus);
    KoIRR_ChZonenStatus.value(statusText.c_str(), DPT_String_8859_1);
}