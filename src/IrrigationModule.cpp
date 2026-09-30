#include "IrrigationModule.h"
#include "OpenKNX.h"
#include "ModuleVersionCheck.h"



IrrigationModule openknxIrrigationModule;

IrrigationModule::IrrigationModule()
{
    for (uint8_t i = 0; i < IRR_ChannelCount; i++)
    {
        _channels[i] = new IrrigationChannel(i);  // nur Platzhalter, damit restore() schon funktioniert
        //logInfoP("Channel %d: new IrrigationChannel", i);
    }
}

IrrigationModule::~IrrigationModule()
{
    for (uint8_t i = 0; i < IRR_ChannelCount; i++)
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
                _channels[i]->process_Bewaesserungsberechnung_channel(ET0_gestern, Regenmenge_gestern, _Sperre_Global);
            }
        } 
        pruefeUndStarteBewaesserungsfenster();
    }

    for (uint8_t i = 0; i < MIN(ParamIRR_VisibleChannels, IRR_ChannelCount); i++)  
    {      
        _channels[i]->loop();
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
        process_Temperatur_Wetterstation(iKo.value(DPT_Value_Temp));
    }
    else if (iKo.asap() == IRR_KoRegenmenge_Wetterstation)  
    {
        process_Regenmenge_Wetterstation(iKo.value(DPT_Rain_Amount));
    }
    else if  (iKo.asap() == IRR_KoGlobaleSperre)
    {
        _Sperre_Global = KoIRR_GlobaleSperre.value(DPT_Enable);
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

// magicYday(4) Tmax_heute(4) Tmin_heute(4) gueltigeWerte_heute(1)
// Tmax_gestern(4) Tmin_gestern(4) TDurchschnitt_gestern(4) Regenmenge_gestern(4) = 29 Byte
static constexpr uint16_t IRR_MODULE_FLASH_SIZE = 4 + 4 + 4 + 1 + 4 + 4 + 4 + 4;
static constexpr uint16_t IRR_CHANNEL_FLASH_SIZE = 4 + 1; // float Wasserbilanzkonto + 1 Byte ZonenStatus


uint16_t IrrigationModule::flashSize()
{
    // [4] Magic Word + [1] Version + [N] 
    return 4 + 1 + IRR_MODULE_FLASH_SIZE + (IRR_ChannelCount * IRR_CHANNEL_FLASH_SIZE);
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


    // jeder Kanal-Slot wird IMMER geschrieben, unabhängig von VisibleChannels ----
    for (uint8_t i = 0; i < IRR_ChannelCount; i++)
    {
        _channels[i]->save();
    }

    logDebugP("write [done]");
}



void IrrigationModule::readFlash(const uint8_t* data, const uint16_t size)
{
    logIndentUp();
    if (size < flashSize()) // no channels present
    {
        logDebugP("Flash data short (have %u, need %u)!", size, flashSize());
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


    // jeder Kanal-Slot wird IMMER gelesen, unabhängig von VisibleChannels 
    for (uint8_t i = 0; i < IRR_ChannelCount; i++)
    {
        _channels[i]->restore();
    }
    logDebugP("read [done]");
    logIndentDown();
}

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

    if (!openknx.time.isValid())
    {
        return 0; // Uhr hat noch kein gültiges Datum vom Bus
    }

    // use current time
    tm tmNow;
    openknx.time.getLocalTime().toTm(tmNow);
    return (uint16_t)(tmNow.tm_yday + 1); // +1 damit es Tagbasiert (1..366) 1.1. ist dann der 1. Tag
}

void IrrigationModule::calculateEt0(uint16_t TagdesJahres)
{
    if (!gueltigeWerte_heute) 
    { 
        ET0_gestern = 0.0f; 
        logDebugP("calculateEt0: keine gültigen Temperaturwerte für gestern, ET0_gestern=0.0");
    }
    else
    {
        RaResult RaErgebnis;
        RaErgebnis = calc_Ra(TagdesJahres);
        ET0_gestern = calc_ET0(Temperatur_Durchschnitt_gestern, Temperatur_max_gestern, Temperatur_min_gestern, RaErgebnis.ra_mm);
    }  
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
    // eigentlich 
    // float_t Sonnenuntergangswinkel_omega_s = acos(-tan(geografischeBreiteRadiant_Phi) * tan(Solare_Deklination_delta));

    //aber abfangen
    float arg = -tan(geografischeBreiteRadiant_Phi) * tan(Solare_Deklination_delta);
    if (arg < -1.0) arg = -1.0;
    if (arg > 1.0) arg = 1.0;
    float_t Sonnenuntergangswinkel_omega_s = acos(arg);

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


bool IrrigationModule::get_globaleSperre()
{
    // gloable Freigaeb abrufen via KO
    return _Sperre_Global;
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
    if (command.substr(0, 3) == "irr")
    {
        if (!diagnose && command == "irr debug")
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

void IrrigationModule::pruefeUndStarteBewaesserungsfenster(void)
{
    tm tmNow;
    openknx.time.getLocalTime().toTm(tmNow);
    uint16_t heute = getYearDay();

    if (!_zeitfensterAktiv && _zeitfensterTag != heute &&
        tmNow.tm_hour == ParamIRR_BewaesserungsstartStunde &&
        tmNow.tm_min == ParamIRR_BewaesserungsstartMinute)
    {
        _zeitfensterAktiv = true;
        _zeitfensterTag = heute;
        logInfoP("Bewaesserungsfenster gestartet");
    }

    if (_zeitfensterAktiv) koordiniereZonenstart();
}

bool IrrigationModule::sindKompatibel(uint8_t zoneA, uint8_t zoneB)
{
    if (zoneA == zoneB) return true; // wird praktisch nie gebraucht, aber sauber definiert

    uint8_t a = MIN(zoneA, zoneB);
    uint8_t b = MAX(zoneA, zoneB);

    if (a == 1 && b == 2) return ParamIRR_KompatibelZone1Zone2;
    if (a == 1 && b == 3) return ParamIRR_KompatibelZone1Zone3;
    if (a == 1 && b == 4) return ParamIRR_KompatibelZone1Zone4;
    if (a == 1 && b == 5) return ParamIRR_KompatibelZone1Zone5;
    if (a == 1 && b == 6) return ParamIRR_KompatibelZone1Zone6;
    if (a == 2 && b == 3) return ParamIRR_KompatibelZone2Zone3;
    if (a == 2 && b == 4) return ParamIRR_KompatibelZone2Zone4;
    if (a == 2 && b == 5) return ParamIRR_KompatibelZone2Zone5;
    if (a == 2 && b == 6) return ParamIRR_KompatibelZone2Zone6;
    if (a == 3 && b == 4) return ParamIRR_KompatibelZone3Zone4;
    if (a == 3 && b == 5) return ParamIRR_KompatibelZone3Zone5;
    if (a == 3 && b == 6) return ParamIRR_KompatibelZone3Zone6;
    if (a == 4 && b == 5) return ParamIRR_KompatibelZone4Zone5;
    if (a == 4 && b == 6) return ParamIRR_KompatibelZone4Zone6;
    if (a == 5 && b == 6) return ParamIRR_KompatibelZone5Zone6;

    return false; // sollte bei 1..6 nie erreicht werden
}

void IrrigationModule::koordiniereZonenstart(void)
{
    bool nochOffenerBedarf = false;
    bool nochWelcheAmLaufen = false;

    for (uint8_t i = 0; i < _numChannels; i++)
    {
        if (_channels[i] == nullptr) continue; // Falls der Kanal nicht existiert (leerer Zeiger) - überspringen
        if (_channels[i]->laeuftGerade()) { nochWelcheAmLaufen = true; continue; } // Kanalbewässerung läuft - überspringen.
        if (!_channels[i]->hatOffenenBedarf()) continue; // Kanal hat kein Bedarf - überspringen

        nochOffenerBedarf = true;
        bool startenErlaubt = true;

        // Bevor Kanal i gestartet wird, muss geprüft werden, ob er sich mit den Kanälen verträgt, die aktuell schon laufen.
        for (uint8_t j = 0; j < _numChannels; j++)
        {
            if (j == i || _channels[j] == nullptr || !_channels[j]->laeuftGerade()) continue; // Es werden nur Kanäle j betrachtet, die ungleich i sind und gerade aktiv laufen.
                if (!sindKompatibel(i + 1, j + 1))  // Hier wird geprüft, ob Kanal i und der laufende Kanal j gleichzeitig aktiv sein dürfen
                {
                    startenErlaubt = false; // Start verbieten, nicht kompatibel
                    break;                  // Weitere Prüfung nicht nötig, da bereits inkompatibel
                }
        }

        if (startenErlaubt) _channels[i]->starteBewaesserung();
    }

    if (!nochOffenerBedarf && !nochWelcheAmLaufen)
    {
        _zeitfensterAktiv = false;
        logInfoP("Bewaesserungsfenster beendet - alle Zonen abgearbeitet");
    }
}