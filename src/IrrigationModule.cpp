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

#include "IrrigationModule.h"
#include "OpenKNX.h"
#include "ModuleVersionCheck.h"
#include "KnxHelper.h"


IrrigationModule openknxIrrigationModule;

IrrigationModule::IrrigationModule()
{
    for (uint8_t i = 0; i < IRR_ChannelCount; i++)
    {
        _channels[i] = new IrrigationChannel(i);  // nur Platzhalter, damit restore() schon funktioniert
        logInfoP("Channel %d: new IrrigationChannel", i);
    }
}

IrrigationModule::~IrrigationModule()
{
    for (uint8_t i = 0; i < _numChannels; i++)
    {
        delete _channels[i];
    }
}

const std::string IrrigationModule::name()
{
    return "Irrigation";
}

const std::string IrrigationModule::version()
{
    return MODULE_Irrigation_Version;
}

void IrrigationModule::loop()
{
    if (!openknx.time.isValid())
    {
        return; // Uhr hat noch kein gültiges Datum vom Bus
    }
    else
    {
        uint16_t heute = getYearDay();

        if (letzterBekannterTag == -1)
        {
            // erster gültiger Aufruf nach Neustart - nur merken, NICHT als
            // Tageswechsel werten (sonst würde beim Boot sofort "gestern"
            // mit leeren Werten überschrieben)
            letzterBekannterTag = heute;
            return;
        }

        else if (heute != letzterBekannterTag)
        {
            // plausi, letzerbekannter tag sollte heute -1 sein, bzw 366 zu 1
            Tageswechsel_Werte_speichern((uint16_t)letzterBekannterTag);
            calculateEt0(letzterBekannterTag);
            letzterBekannterTag = heute;
            for (uint8_t i = 0; i < MIN(ParamIRR_VisibleChannels, IRR_ChannelCount); i++)
            {
                if (_channels[i] == nullptr) continue;
                _channels[i]->process_Bewaesserungsberechnung_channel(ET0_gestern, Regenmenge_gestern);;
            }
        }

        if (ermittleglobaleFreigabe() == 1)
        {

        }
    
        //     // ---- Bewässerungsstart-Trigger (einmal pro Tag) ----
        // if (BewaesserungszoneBedarf && !BewaesserungszoneVentilOffen && letzterBewaesserungsTag != heute &&
        //     tmNow.tm_hour == BEWAESSERUNG_START_STUNDE && tmNow.tm_min == 0)
        // {
        //     set_Ventil_State(RASENZONE_VENTIL_INDEX, true);
        //     BewaesserungszoneVentilOffen = true;
        //     BewaesserungszoneVentilStartMillis = millis();
        //     letzterBewaesserungsTag = heute;
        //     SERIAL_DEBUG.print("WB Rasenzone: Bewässerung gestartet, Laufzeit[s]=");
        //     SERIAL_DEBUG.println(BewaesserungszoneGeplanteLaufzeitSek);
        // }
    }



}
void IrrigationModule::setup(bool configured)
{
    logInfoP("setup() START");
    
    // Number of available channels is the minimum of configured and available channels
    _numChannels = MIN(ParamIRR_VisibleChannels, IRR_ChannelCount);
    logInfoP("_numChannels=%d", _numChannels);
    for (uint8_t i = 0; i < _numChannels; i++)
    {
        logInfoP("Channel %d: Setup IrrigationChannel", i);
        _channels[i]->setup(configured);   
    }
    logInfoP("setup() DONE");
}



// will be called once a KO received a telegram
void IrrigationModule::processInputKo(GroupObject &iKo)
{
    logDebugP("[Modul] processInputKo");
    logIndentUp();

   
    if (iKo.asap() == IRR_KoTemperatur_Wetterstation) 
    {
        process_Temperatur_Wetterstation(iKo.value(getDPT(VAL_DPT_9)));
    }
    else if (iKo.asap() == IRR_KoRegenmenge_Wetterstation)  
    {
        process_Regenmenge_Wetterstation(iKo.value(getDPT(VAL_DPT_9)));
    }

    for (uint8_t i = 0; i < MIN(ParamIRR_VisibleChannels, IRR_ChannelCount); i++)
    {
        if (_channels[i] == nullptr) continue;
        logDebugP("_channels[ %i ]", i+1);
        _channels[i]->processInputKo(iKo); 
    }
    logIndentDown();



}

void IrrigationModule::showHelp()
{
    openknx.console.printHelpLine("IRR debug", "enable/disable debug mode");
    
}





const uint8_t IrrigationModule::_magicWord[IRR_FLASH_MAGIC_WORD_LEN] = {
    'x',
    'I',
    'R',
    'R',
};

static constexpr uint16_t BEWAESSERUNG_FLASH_SIZE = 4 + 4 + 4 + 1 + 4 + 4 + 4 + 4 + 4;
// magicYday(4) tmaxToday(4) tminToday(4) todayHasData(1)
// tmaxYesterday(4) tminYesterday(4) tmeanYesterday(4) regenGestern(4) konto(4)


uint16_t IrrigationModule::flashSize()
{
    // [4] Magic Word + [1] Version + [N] 
    return 4 + 1 + BEWAESSERUNG_FLASH_SIZE;
}


 void IrrigationModule::writeFlash()
{
    logDebugP("writing");

    // magic word
    for (size_t i = 0; i < IRR_FLASH_MAGIC_WORD_LEN; i++)
    {
        openknx.flash.writeByte(_magicWord[i]);
    }
    
    // version
    openknx.flash.writeByte(1);



    uint32_t currentYday = openknx.time.isValid() ? getYearDay() : 0;

    openknx.flash.writeInt(currentYday);
    openknx.flash.writeFloat(Temperatur_max_heute);
    openknx.flash.writeFloat(Temperatur_min_heute);
    openknx.flash.writeByte(gueltigeWerte_heute ? 1 : 0);
    openknx.flash.writeFloat(Temperatur_max_gestern);
    openknx.flash.writeFloat(Temperatur_min_gestern);
    openknx.flash.writeFloat(Temperatur_Durchschnitt_gestern);
    openknx.flash.writeFloat(Regenmenge_gestern);
   // openknx.flash.writeFloat(BewaesserungszoneKonto); // MUSS über Neustart erhalten bleiben! kommt aus dem Channel
    logDebugP("write [done]");
}



void IrrigationModule::readFlash(const uint8_t* data, const uint16_t size)
{
    logIndentUp();
    if (size < 4 + 1) // no channels present
    {
        logDebugP("Flash data short!");
        return;
    }
    
    for (size_t i = 0; i < IRR_FLASH_MAGIC_WORD_LEN; i++)
    {
        if (openknx.flash.readByte() != _magicWord[i])
        {
            logDebugP("Wrong magic-word!");
            return;
        }
    }

    const uint8_t version = openknx.flash.readByte();
    if (version != 1) // version unknown
    {
        logDebugP("Wrong version (%d)", version);
        return;
    }

    uint16_t restoredcurrentYday = openknx.flash.readInt();
    float restoredTemperatur_max_heute = openknx.flash.readFloat();
    float restoredTemperatur_min_heute = openknx.flash.readFloat();
    gueltigeWerte_heute = openknx.flash.readByte();  
    Temperatur_max_gestern = openknx.flash.readFloat();
    Temperatur_min_gestern = openknx.flash.readFloat();
    Temperatur_Durchschnitt_gestern = openknx.flash.readFloat();
    Regenmenge_gestern = openknx.flash.readFloat();

    // heutige gespeicherte Werte nur übernehmen, wenn sie tatsächlich von HEUTE sind
    if (openknx.time.isValid() && (uint32_t)getYearDay() == restoredcurrentYday)
    {
        Temperatur_max_heute = restoredTemperatur_max_heute;
        Temperatur_min_heute = restoredTemperatur_min_heute;
        gueltigeWerte_heute = true;
    }
    else
    {
        // Zeit noch nicht gültig ODER Tag hat sich seit dem letzten Speichern geändert. lieber frisch beginnen, stattveraltete/falsche Werte weiterzuschleppen.
        Temperatur_max_heute = -42;
        Temperatur_min_heute = 42;
        gueltigeWerte_heute = false;
    }


    const uint8_t chDataMaxCount = (size - 4 - 1) / (1);
    logDebugP("Found %d of %d channels", chDataMaxCount, IRR_ChannelCount);
    const uint8_t n = MIN(chDataMaxCount, IRR_ChannelCount);
    for (uint8_t i = 0; i < n; i++)
    {
        _channels[i]->restore();
    }
    logDebugP("read [done]");
    logIndentDown();
}

// void Bewaesserung_readFlash(const uint8_t* buffer, const uint16_t size)
// {
//     if (size < BEWAESSERUNG_FLASH_SIZE) return;

//     for (size_t i = 0; i < IRR_FLASH_MAGIC_WORD_LEN; i++)
//     {
//         if (openknx.flash.readByte() != _magicWord[i])
//         {
//             logDebugP("Wrong magic-word!");
//             return;
//         }
//     }

//     uint32_t magicYday;
//     float tmaxToday, tminToday, tmaxYesterday, tminYesterday, tmeanYesterday, regenGestern, konto;
//     uint8_t todayHasData;

//     uint16_t o = 0;
//     memcpy(&magicYday, buffer + o, 4); o += 4;
//     memcpy(&tmaxToday, buffer + o, 4); o += 4;
//     memcpy(&tminToday, buffer + o, 4); o += 4;
//     memcpy(&todayHasData, buffer + o, 1); o += 1;
//     memcpy(&tmaxYesterday, buffer + o, 4); o += 4;
//     memcpy(&tminYesterday, buffer + o, 4); o += 4;
//     memcpy(&tmeanYesterday, buffer + o, 4); o += 4;
//     memcpy(&regenGestern, buffer + o, 4); o += 4;
//     memcpy(&konto, buffer + o, 4);

//     // gestrige Werte gelten unabhängig vom Tag weiterhin als gültige Basis
//     Temperatur_max_gestern = tmaxYesterday;
//     Temperatur_min_gestern = tminYesterday;
//     Temperatur_Durchschnitt_gestern = tmeanYesterday;

//     Regenmenge_gestern = regenGestern;
//     // BewaesserungszoneKonto = konto; // <- der wichtige Teil: Kontostand übersteht den Neustart je Zone ist im Channelmodul

//     // heutige gespeicherte Werte nur übernehmen, wenn sie tatsächlich von HEUTE sind
//     if (openknx.time.isValid() && (uint32_t)getYearDay() == magicYday)
//     {
//         Temperatur_max_heute = tmaxToday;
//         Temperatur_min_heute = tminToday;
//         gueltigeWerte_heute = (todayHasData != 0);
//         return;
//     }
//     else

//     // Zeit noch nicht gültig ODER Tag hat sich seit dem letzten Speichern
//     // geändert -> heutige Aggregation lieber frisch beginnen, statt
//     // veraltete/falsche Werte weiterzuschleppen.
//     Temperatur_max_heute = -4.2;
//     Temperatur_min_heute = -4.2;
//     gueltigeWerte_heute = false;
// }


// ---- Tageswechsel: gestern einfrieren, ET0 rechnen, heute zurücksetzen --
void IrrigationModule::Tageswechsel_Werte_speichern(uint16_t gestern)
{
    if (!gueltigeWerte_heute)
    {
        SERIAL_DEBUG.println("Bewaesserung: Tageswechsel ohne Temperaturdaten - überspringe");
    }
    else
    {
        Temperatur_max_gestern = Temperatur_max_heute;
        Temperatur_min_gestern = Temperatur_min_heute;
        Temperatur_Durchschnitt_gestern = Temperatur_Durchschnitt_heute;
    }
    Regenmenge_gestern = letzteRegenmengeHeute;


    // "heute" zurücksetzen
    Temperatur_max_heute = letzteEmpfangeneTemperatur;
    Temperatur_min_heute = letzteEmpfangeneTemperatur;
    Temperatur_Durchschnitt_heute = letzteEmpfangeneTemperatur;
    gueltigeWerte_heute = false;

    letzteRegenmengeHeute = 0.0f; // Annahme: Regenmesser resettet ebenfalls täglich

    // min max Durchschnitswerte heute senden
    KoIRR_TDurchschnittGestern.value(Temperatur_Durchschnitt_gestern, DPT_Value_Temp);
    KoIRR_TMaxGestern.value(Temperatur_max_gestern, DPT_Value_Temp);
    KoIRR_TMinGestern.value(Temperatur_min_gestern, DPT_Value_Temp);
}

uint16_t IrrigationModule::getYearDay(void)
{
    // use current time
    tm tmNow;
    openknx.time.getLocalTime().toTm(tmNow);
    return (uint16_t)(tmNow.tm_yday + 1); // +1 damit es Tagbasiert (1..366) 1.1. ist dann der 1. Tag
}

void IrrigationModule::calculateEt0(uint16_t TagdesJahres)
{
    RaResult RaErgebnis;
    RaErgebnis = calc_Ra(TagdesJahres);

    ET0_gestern = calc_ET0(Temperatur_Durchschnitt_gestern, Temperatur_max_gestern, Temperatur_min_gestern, RaErgebnis.ra_mm);

    // ---- KO-Ausgabe ----
     KoIRR_Berechnung_ET0.value(ET0_gestern, DPT_Value_Temp);
}



// Phi aus dem Standort-Parameter ableiten
float IrrigationModule::get_Geographische_Breite_Radiant()
{
    return (float)(ParamBASE_Latitude * PI / 180.0);
}

// ============================================================
// 1. Extraterrestrische Strahlung Ra
// ============================================================
IrrigationModule::RaResult IrrigationModule::calc_Ra(uint16_t Kalendertag_des_Jahres_J)
{
    float_t geografischeBreiteRadiant_Phi = get_Geographische_Breite_Radiant();

    // Solarkonstante [MJ/(m²·min)]
    const float_t Solarkonstante_Gsc = 0.0820;
    // Inverse relative Erde-Sonne-Distanz
    float_t relativeErdeSonneDistanz_dr = 1.0 + 0.033 * cos((2.0 * M_PI / 365.0) * Kalendertag_des_Jahres_J);
    // Solare Deklination [rad]
    float_t Solare_Deklination_delta = 0.409 * sin((2.0 * M_PI / 365.0) * Kalendertag_des_Jahres_J - 1.39);

    // Sonnenuntergangswinkel [rad]
    float_t Sonnenuntergangswinkel_omega_s = acos(-tan(geografischeBreiteRadiant_Phi) * tan(Solare_Deklination_delta));

    // Extraterrestrische Strahlung [MJ/(m²·Tag)]
    float_t Extraterrestrische_Strahlung_Ra = (24.0 * 60.0 / M_PI) * Solarkonstante_Gsc * relativeErdeSonneDistanz_dr * (Sonnenuntergangswinkel_omega_s * sin(geografischeBreiteRadiant_Phi) * sin(Solare_Deklination_delta) + cos(geografischeBreiteRadiant_Phi) * cos(Solare_Deklination_delta) * sin(Sonnenuntergangswinkel_omega_s));

    //return Extraterrestrische_Strahlung_Ra;

    RaResult result;
    result.Kalendertag_des_Jahres_J = Kalendertag_des_Jahres_J;
    result.phi = geografischeBreiteRadiant_Phi;
    result.ra_mm = Extraterrestrische_Strahlung_Ra * 0.408f; // MJ/(m²·Tag) -> mm/Tag
    result.dr = relativeErdeSonneDistanz_dr;
    result.delta = Solare_Deklination_delta;
    result.omegaS = Sonnenuntergangswinkel_omega_s;
    return result;
}

// ============================================================
// 2. Hargreaves-Samani: Referenz-Evapotranspiration ET0
// ============================================================
float IrrigationModule::calc_ET0(float T_mean, float T_max, float T_min, float Ra_mm)
{
    float_t ET0 = 0;
    float deltaT = T_max - T_min;
    if (deltaT < 0) deltaT = 0; // Schutz vor NaN durch fehlerhafte/vertauschte Sensordaten

    // ET0 [mm/Tag]
    ET0 = 0.0023 * (T_mean + 17.8) * sqrt(deltaT) * Ra_mm;
    if (ET0 < 0) ET0 = 0; // ET0  nie negativ - abfangen

    return ET0;
}




// TODO: echte Freigabe (Rainclick/Füllstand/Zeitfenster) noch nicht verdrahtet
bool IrrigationModule::ermittleglobaleFreigabe()
{
    // gloable Freigaeb abrufen via KO
    return true;
}

void IrrigationModule::process_Temperatur_Wetterstation (float aktuelleTemperatur)
{
    // Sinnvolle Plausigrenzen für Außentemperatur (Sensorfehler abfangen)
    if (aktuelleTemperatur < -40.0f || aktuelleTemperatur > 60.0f)
    {
        logDebugP("Temperaturwert außerhalb Plausibereich verworfen: %f", aktuelleTemperatur);
        return;
    }
    letzteEmpfangeneTemperatur = aktuelleTemperatur;

    if (!gueltigeWerte_heute)
    {
        Temperatur_max_heute = aktuelleTemperatur;
        Temperatur_min_heute = aktuelleTemperatur;
        Temperatur_Durchschnitt_heute = aktuelleTemperatur;
        gueltigeWerte_heute = true;
    }
    else
    {
        if (aktuelleTemperatur > Temperatur_max_heute) Temperatur_max_heute = aktuelleTemperatur;
        if (aktuelleTemperatur < Temperatur_min_heute) Temperatur_min_heute = aktuelleTemperatur;
    }

    // Mittelwert bilden.
    Temperatur_Durchschnitt_heute = (aktuelleTemperatur + Temperatur_Durchschnitt_heute) / 2;

    // min max Durchschnitswerte heute senden
    KoIRR_TDurchschnittHeute.value(Temperatur_Durchschnitt_heute, DPT_Value_Temp);
    KoIRR_TMaxHeute.value(Temperatur_max_heute, DPT_Value_Temp);
    KoIRR_TMinHeute.value(Temperatur_min_heute, DPT_Value_Temp);
}

void IrrigationModule::process_Regenmenge_Wetterstation (float regenmengeHeuteMm)
{
    if (regenmengeHeuteMm < 0.0f)
    {
        logDebugP("Regenmenge negativ verworfen: %f ", regenmengeHeuteMm);
        return;
    }
    letzteRegenmengeHeute = regenmengeHeuteMm;
}


bool IrrigationModule::processCommand(const std::string command, bool diagnose)
{
    uint8_t value = 0;
    if (command.substr(0, 3) == "irr")
    {
        if (!diagnose && command == "amp debug")
        {
            _debug = !_debug;
            logDebugP(_debug ? "IRR Debug enabled" : "IRR Debug disabled");
            return true;
        }
    }
    return false;
}

bool IrrigationModule::debug()
{
    return _debug;
}