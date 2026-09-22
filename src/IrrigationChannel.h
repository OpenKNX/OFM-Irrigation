#pragma once
#include "OpenKNX.h"
#include <SoftwareSerial.h>


class IrrigationChannel : public OpenKNX::Channel
{
private:

    

    bool _channelActive = false; // is enabled in ETS?

    float Zone_KC = 0.8f;                 // als ETS-Parameter/GA veränderlich
    float Zone_NFK_MM = 25.0f;             // nutzbare Feldkapazität [mm]
    float Zone_SCHWELLWERT_P = 0.5f;       // 50 %
    float Zone_NIEDERSCHLAGSRATE = 10.0f;  // mm/h (MP-Rotator)P Rotator Standarddüsen = 10 mm/h  MP800 = 20 mm/h

    float bodenwasserkonto;

    //uint8_t BEWAESSERUNG_START_STUNDE = 4;  // 4:00 Uhr, TODO: ETS-Parameter (4 oder 5)


    // Private Methode zur Verarbeitung von empfangenen Zeilen
  

    float calc_Bodenwasserkonto(float konto_alt, float niederschlag_mm, float ETc, float nFK);
    float calc_Schwellwert(float p, float nFK);
    bool calc_bedarf(float bodenwasserkonto_neu, float p, float nFK);
    float calc_laufzeit_sek(float fehlmenge_mm, float niederschlagsrate_mm_h);
    float calc_Zugefuehrte_Wassermenge(float laufzeit_sek, float niederschlagsrate_mm_h);
    float calc_Bodenwasserkonto_final(float bodenwasserkonto_neu, float bewaesserung_mm, float nFK);
    float calc_NutzbareFeldkapazitaet(float FK, float PWP);
    float calc_ETc(float ET0, float Kc);
    bool ermittlelokaleFreigabe();

    
    void setKOInitialValues(void);

    
public:
    IrrigationChannel(uint8_t iChannelNumber);
    ~IrrigationChannel();

    const std::string name() override;
    void setup(bool configured) override;
    void loop() override;
    void processInputKo(GroupObject &ko) override;
    void process_Bewaesserungsberechnung_channel(float et0Gestern, float regenmengeGestern);
    
    bool isActive();
        
    void save();     // called by IrrigationModule::writeFlash()
    void restore();  // called by IrrigationModule::readFlash()
   
};

