/*
  # Arylic's UART API developer documentation
  # https://developer.arylic.com/uartapi/#uart-api

# Basic Rules
   * messages are defined in 3 characters, and will use : to seperate the different part.
   * messages sent over UART need to be terminated with ;
   * messages might be received without query when state changed.
   * Content between {} is variable name, you need to replace with the real content, and {} itself is not meant to be sent.
   * Content between [] means optional, and [] itself is not meant to be sent.
   * normally, messages sent by host without param means to query current state or direct control
   * messages sent with param means to control or change state.
   * messages received normally with param indicating current state.
*/

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
            Zone_NIEDERSCHLAGSRATE = KoIRR_ChNiederschlagsrate.value(DPT_Value_Temp);
            logDebugP("processInputKo: Zone_NIEDERSCHLAGSRATE");
            break;
        }
        case IRR_KoChSchwellwert:
        {
            Zone_SCHWELLWERT_P = KoIRR_ChSchwellwert.value(DPT_Value_Temp);
            logDebugP("processInputKo: Zone_SCHWELLWERT_P");
            break;
        }
        case IRR_KoChnFK: // Nutzbare Feldkapazität (nFK) [mm]    
        {
            Zone_NFK_MM = KoIRR_ChnFK.value(DPT_Value_Temp);
            logDebugP("processInputKo: Zone_NFK_MM");
            break;
        }
        case IRR_KoChKc: // Kc-Faktor der Zone  
        {
            Zone_KC = KoIRR_ChKc.value(DPT_Value_Temp);
            logDebugP("processInputKo: Zone_KC");
            break;
        }
        case IRR_KoChZonenFreigabe:
        {
            
            break;
        }
        case IRR_KoChBedarf:
        {

            break;
        }
        case IRR_KoChFehlmenge:
        {
            break;
        }
        case IRR_KoChLaufzeit:
        {
            break;
        }
        case IRR_KoChWasserbilanzkonto:
        {
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

    
    // // ---- Ventil-Timer: läuft die Bewässerung gerade, ist sie fertig? ----
    // if (BewaesserungszoneVentilOffen)
    // {
    //     uint32_t laufSek = (millis() - BewaesserungszoneVentilStartMillis) / 1000;
    //     if ((float)laufSek >= BewaesserungszoneGeplanteLaufzeitSek)
    //     {
    //         // set_Ventil_State(RASENZONE_VENTIL_INDEX, false);
    //         // BewaesserungszoneVentilOffen = false;

    //         // ---- Rückbuchung ----
    //         float bewaessert_mm = calc_Zugefuehrte_Wassermenge(BewaesserungszoneGeplanteLaufzeitSek, RASEN_NIEDERSCHLAGSRATE);
    //         BewaesserungszoneKonto = calc_Bodenwasserkonto_final(BewaesserungszoneKonto, bewaessert_mm, RASEN_NFK_MM);
    //         BewaesserungszoneGeplanteLaufzeitSek = 0.0f;

    //         // SERIAL_DEBUG.print("WB Rasenzone: Bewässerung beendet, Konto final=");
    //         // SERIAL_DEBUG.println(BewaesserungszoneKonto);
    //             // ---- KO-Ausgabe ----
    //         KoIRR_ChWasserbilanzkonto.value(BewaesserungszoneKonto, DPT_Value_Temp);
         
    //     }
    // }
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
    String empty = "";
    // Initialwerte für KOs setzen
    //KoAMP_ChVolumeStatus.value(currentVolume, DPT_Scaling);

    if (openknxIrrigationModule.debug())
    {
        logDebugP("[INIT] KO Initial Values gesetzt");
    }
}

void IrrigationChannel::save()
{
    openknx.flash.writeFloat(bodenwasserkonto);
    logDebugP("saved: bodenwasserkonto=%f", bodenwasserkonto);
}

void IrrigationChannel::restore()
{
     float savedbodenwasserkonto = openknx.flash.readFloat();

    // if (!wasActive)
    // {
    //     logDebugP("restore: Kanal war beim Speichern nicht aktiv - ueberspringen");
    //     return;
    // }

    bodenwasserkonto = savedbodenwasserkonto;
  
    logDebugP("restored: bodenwasserkonto=%f", bodenwasserkonto);
}



void IrrigationChannel::process_Bewaesserungsberechnung_channel(float et0Gestern, float regenmengeGestern)
{
    if (!_channelActive) return;

    float etc = calc_ETc(et0Gestern, Zone_KC);
    bodenwasserkonto = calc_Bodenwasserkonto(bodenwasserkonto, regenmengeGestern, etc, Zone_NFK_MM);

    logDebugP("Kanal %u: ETc=%.2f Konto=%.2f", _channelIndex, etc, bodenwasserkonto);

    // Bedarf/Fehlmenge/Laufzeit folgen hier als nächstes, sobald die
    // Freigabe-KOs verdrahtet sind - siehe unten
}


bool IrrigationChannel::ermittlelokaleFreigabe()
{
    // TODO
    // lokale Freigabe abrufen via KO
    return true;
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
float IrrigationChannel::calc_Bodenwasserkonto(float konto_alt, float niederschlag_mm ,float ETc, float nFK)
{
    // Neues Bodenwasserkonto [mm]
    float_t Bodenwasserkonto = konto_alt + niederschlag_mm - ETc;

    // Begrenzung auf 0 ... nFK
    Bodenwasserkonto = MAX(0.0, MIN(Bodenwasserkonto, nFK));
    return Bodenwasserkonto;
}
// ============================================================
// 5. Bewässerungsschwelle
// ============================================================
float IrrigationChannel::calc_Schwellwert (float p, float nFK)
{
    // Schwellwert [mm]
    float_t schwellwert = p * nFK;
    return schwellwert;
}
// ============================================================
// 6. Bewässerungsbedarf
// ============================================================

// true  = Bewässerung erforderlich
// false = keine Bewässerung erforderlich
bool IrrigationChannel::calc_bedarf (float Bodenwasserkonto_neu, float p, float nFK)
{
    bool bedarf = Bodenwasserkonto_neu < (p * nFK);
    return bedarf;
}
// ============================================================
// 7. Freigabe
// ============================================================

// // Bewässerung nur wenn Bedarf UND Basisfreigabe
// bool freigabe = bedarf && basis_freigabe;

// ============================================================
// 9. Bewässerungs-Laufzeit
// ============================================================

float IrrigationChannel::calc_laufzeit_sek (float fehlmenge_mm, float niederschlagsrate_mm_h)
{
    // Niederschlagsrate [mm/h]
    // Laufzeit [Sekunden]
    float_t laufzeit_sek = (fehlmenge_mm / niederschlagsrate_mm_h) * 3600.0;
    return laufzeit_sek;
}
// ============================================================
// 10. Rückbuchung der Bewässerung
// ============================================================
float IrrigationChannel::calc_Zugefuehrte_Wassermenge (float laufzeit_sek, float niederschlagsrate_mm_h)
{
    // Zugeführte Wassermenge [mm]
    float_t bewaesserung_mm = (laufzeit_sek / 3600.0) * niederschlagsrate_mm_h;
    return bewaesserung_mm;
}

float IrrigationChannel::calc_Bodenwasserkonto_final(float Bodenwasserkonto_neu, float bewaesserung_mm, float nFK )
{
    // Bodenwasserkonto nach Bewässerung [mm]
    float_t Bodenwasserkonto_final = Bodenwasserkonto_neu + bewaesserung_mm;

    // Sicherheitshalber wieder auf 0 ... nFK begrenzen
    Bodenwasserkonto_final = MAX(0.0, MIN(Bodenwasserkonto_final, nFK));
    return Bodenwasserkonto_final;
}
// ============================================================
// 11. Nutzbare Feldkapazität
// ============================================================
float IrrigationChannel::calc_NutzbareFeldkapazitaet(float FK, float PWP)
{
    // nFK = FK - PWP
    float nFK = FK - PWP;
    return nFK;
}