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

// will be called once a KO received a telegram
void IrrigationChannel::processInputKo(GroupObject &iKo)
{
    if (!_channelActive)
    {
       //logDebugP("processInputKo: channel %u not active", _channelIndex); // nur debug
        return;
    }

    logIndentUp();
    if (openknxIrrigationModule.debug())
    {
        logDebugP("[channel]processInputKo: channel %u", _channelIndex);
        
    }   

    //logDebugP("IRR_KoCalcIndex %i", IRR_KoCalcIndex(iKo.asap()));

    switch (IRR_KoCalcIndex(iKo.asap()))
    {
        case IRR_KoChNiederschlagsrate:
        {
            Niederschlagsrate_Zone = KoIRR_ChNiederschlagsrate.value(DPT_Value_Temp);
            logDebugP("processInputKo: Niederschlagsrate_Zone=%.2f", Niederschlagsrate_Zone);
            break;
        }
        case IRR_KoChSchwellwert:
        {
            Schwellwert_P_Prozent_Zone = KoIRR_ChSchwellwert.value(DPT_Scaling);
            logDebugP("processInputKo: Schwellwert_P_Prozent_Zone=%u", Schwellwert_P_Prozent_Zone);
            break;
        }
        case IRR_KoChnFK: // Nutzbare Feldkapazität (nFK) [mm]    
        {
            nutzbareFeldkapazitaet_nFK_Zone = KoIRR_ChnFK.value(DPT_Value_Temp);
            logDebugP("processInputKo: nutzbareFeldkapazitaet_nFK_Zone=%.2f", nutzbareFeldkapazitaet_nFK_Zone);
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
   

        default:
            logDebugP("default case processInputKo: unknown KO index %u", IRR_KoCalcIndex(iKo.asap()));
            break;   
    }
    logIndentDown();
}

void IrrigationChannel::loop()
{
    if (!_channelActive) return;
    // Tageswechsel-getriebene Berechnung; hier aktuell nichts pro Loop-Durchlauf zu tun.
}

void IrrigationChannel::setup(bool configured)
{
    _channelActive = configured && (ParamIRR_ChActive == 1);
    if (!_channelActive) 
    {
        logDebugP("Channel %u: not active!", _channelIndex);
        return;
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
    openknx.flash.writeFloat(Wasserbilanzkonto);
    logDebugP("saved: Wasserbilanzkonto=%f", Wasserbilanzkonto);
}

void IrrigationChannel::restore()
{
    Wasserbilanzkonto = openknx.flash.readFloat();
    logDebugP("restored: Wasserbilanzkonto=%f", Wasserbilanzkonto);
}



void IrrigationChannel::process_Bewaesserungsberechnung_channel(float et0Gestern, float regenmengeGestern, bool Sperre_global)
{
    if (!_channelActive) return;

    // Tageswechsel von Modulebene berechnet ET0 und triggert das hier
    // jetzt berechnen wir den Bedarf, anhand von ETc, den Bodenwasserkonto, der Feldkapazität (nFK), Schwellwert

    float ETc_gestern = calc_ETc(et0Gestern, _Kulturfaktor_Kc_Zone);
    
    Wasserbilanzkonto = calc_Bodenwasserkonto(Wasserbilanzkonto, regenmengeGestern, ETc_gestern, nutzbareFeldkapazitaet_nFK_Zone);
    Schwellwert_in_mm_Zone = calc_Schwellwert_in_mm((float)Schwellwert_P_Prozent_Zone, nutzbareFeldkapazitaet_nFK_Zone);
    Bewaesserungsbedarf = calc_bedarf (Wasserbilanzkonto, Schwellwert_in_mm_Zone);

    // sende das berechnete auf den Bus
    KoIRR_ChWasserbilanzkonto.value(Wasserbilanzkonto, DPT_Value_Temp);
    KoIRR_ChBedarf.value(Bewaesserungsbedarf, DPT_Switch);

    Diagnose_Bewaesserung_gesperrt = _Sperre_Zone && Sperre_global;
    
    if (Bewaesserungsbedarf == true && Diagnose_Bewaesserung_gesperrt == false)
    {  
        //Fehlmenge und Laufzeit berechnen und für den
        ermittelteFehlmenge_mm = calc_Fehlmenge_mm(nutzbareFeldkapazitaet_nFK_Zone, Wasserbilanzkonto);
        ermittelteLaufzeit_sekunden = (uint16_t)calc_laufzeit_sek   (ermittelteFehlmenge_mm, Niederschlagsrate_Zone);

                            // Bewässerungsstart vormerken ()
        // sende das berechnete auf den Bus
        KoIRR_ChFehlmenge.value(ermittelteFehlmenge_mm, DPT_Value_Temp);
        KoIRR_ChLaufzeit.value(ermittelteLaufzeit_sekunden, DPT_Value_4_Ucount); // DPT für 7.005 prüfen
    }
    else
    {
        //nichts zu tun - nächster Vergleich wieder morgen
    }

    logDebugP("Kanal %u: ETc_gestern=%.2f Wasserbilanzkonto=%.2f Schwellwert[mm]=%f", _channelIndex, ETc_gestern, Wasserbilanzkonto, Schwellwert_in_mm_Zone);
    logDebugP("Bedarf = %i Fehlmenge[mm]=%f notw_Laufzeit[s]=%i", Bewaesserungsbedarf , ermittelteFehlmenge_mm, ermittelteLaufzeit_sekunden);

    // Bedarf/Fehlmenge/Laufzeit folgen hier als nächstes, sobald die
    // Sperre-KOs verdrahtet sind - siehe unten

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
    float schwellwert_in_mm = (float)(Schwellwert_Prozent/100) * nutzbareFeldkapazitaet;
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

float IrrigationChannel::calc_laufzeit_sek (float fehlmenge_mm, float niederschlagsrate_mm_h)
{
    // Niederschlagsrate [mm/h]
    // Laufzeit [Sekunden]
    float_t laufzeit_sek = (fehlmenge_mm / niederschlagsrate_mm_h) * 3600.0f;
    return laufzeit_sek;
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
    if (offen == _statusMagnetventilLetzter) return; // kein Flankenwechsel, nichts zu tun

    if (offen)
    {
        // steigende Flanke: Ventil geht auf -> Zeitmessung starten
        _ventilOffenSeitMillis = millis();
        logDebugP("Kanal %u: Magnetventil offen, Zeitmessung gestartet", _channelIndex);
    }
    else
    {
        // fallende Flanke: Ventil zu -> tatsächliche Laufzeit auswerten, zurückbuchen
        uint32_t gemesseneLaufzeitSek = (millis() - _ventilOffenSeitMillis) / 1000;

        float zugefuehrteWassermenge_mm = calc_Zugefuehrte_Wassermenge((float)gemesseneLaufzeitSek, Niederschlagsrate_Zone);
        Wasserbilanzkonto = calc_Bodenwasserkonto_final(Wasserbilanzkonto, zugefuehrteWassermenge_mm, nutzbareFeldkapazitaet_nFK_Zone);

        logDebugP("Kanal %u: Magnetventil zu, gemessene Laufzeit=%us, zugefuehrt=%.2fmm, Konto final=%.2f",
                  _channelIndex, gemesseneLaufzeitSek, zugefuehrteWassermenge_mm, Wasserbilanzkonto);

        KoIRR_ChWasserbilanzkonto.value(Wasserbilanzkonto, DPT_Value_Temp);

        // Bedarf erneut berechnen
        Bewaesserungsbedarf = calc_bedarf ( Wasserbilanzkonto, Schwellwert_in_mm_Zone);
        // Bedarf ist mit dieser Bewässerung hoffentlich abgearbeitet, bis zum nächsten Tageswechsel neu bewerten
        KoIRR_ChBedarf.value(Bewaesserungsbedarf, DPT_Switch);
    }

    _statusMagnetventilLetzter = offen;
}