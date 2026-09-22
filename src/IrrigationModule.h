#pragma once
#include "OpenKNX.h"
#include "IrrigationChannel.h"
#include "hardware.h"
#include "knxprod.h"

#define IRR_FLASH_MAGIC_WORD_LEN 4
#define IRR_FLASH_VERSION        1
class IrrigationModule : public OpenKNX::Module
{
  protected:
    bool _debug = false;

  public:
    IrrigationModule();
    ~IrrigationModule();
    void processInputKo(GroupObject &ko) override;
    void showHelp() override;
    bool processCommand(const std::string command, bool diagnose) override;
    bool debug();
    void loop() override;
    void setup(bool configured) override;
    const std::string name() override;
    const std::string version() override;
    

    // persistance handling
    uint16_t flashSize() override;
    void writeFlash() override;
    void readFlash(const uint8_t *iBuffer, const uint16_t iSize) override;




  private:
    IrrigationChannel *_channels[IRR_ChannelCount] = {}; // init mit // channel[0] = nullptr, // channel[1] = nullptr, usw
    uint8_t _numChannels = 0;

    float ET0_gestern;

    float letzteEmpfangeneTemperatur = 0;
    float letzteRegenmengeHeute = 0.0f;
    int16_t letzterBekannterTag = -1; // -1 = "noch nie gesehen"
    int16_t letzterBewaesserungsTag = -1;

    
    struct RaResult
    {
        float ra_mm;    // Ra bereits in mm/Tag (inkl. 0.408-Faktor)
        float dr;       // Diagnose
        float delta;    // Diagnose 
        float omegaS;   // Diagnose 
        float phi;      // Diagnose
        uint16_t Kalendertag_des_Jahres_J;  // Diagnose
    }; 

        // heutige Werte
    float Temperatur_max_heute = -42.0;
    float Temperatur_min_heute = 42.0;
    float Temperatur_Durchschnitt_heute = -42.0;
    bool gueltigeWerte_heute = false;

    // Gestrige Werte
    float Temperatur_max_gestern = -42.0;
    float Temperatur_min_gestern = 42.0;
    float Temperatur_Durchschnitt_gestern = -42.0;
    float Regenmenge_gestern = 0.0f;

        // Eingänge vom Bus
    void process_Temperatur_Wetterstation(float aktuelleTemperatur);
    void process_Regenmenge_Wetterstation(float regenmengeHeuteMm);
    void process_Bewaesserungsberechnung(void);
    bool ET0_processCommand(const std::string cmd, bool debugKo);
    uint16_t getYearDay(void); // liefert J, 1-basiert (1..366)
    float get_Geographische_Breite_Radiant();
    RaResult calc_Ra(uint16_t J);
    void Tageswechsel_Werte_speichern(uint16_t gestern);

    void calculateEt0(uint16_t TagdesJahres);
    float calc_ET0(float T_mean, float T_max, float T_min, float Ra_mm);
    bool ermittleglobaleFreigabe();

    static const uint8_t _magicWord[IRR_FLASH_MAGIC_WORD_LEN];
};

extern IrrigationModule openknxIrrigationModule;











