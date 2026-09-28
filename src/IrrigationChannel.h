#pragma once
#include "OpenKNX.h"


class IrrigationChannel : public OpenKNX::Channel
{
private:

    bool _channelActive = false; // is enabled in ETS?
    bool _Sperre_Zone = false; // per KO gesetzt, Default: freigegeben

    float _bodenfeuchteProzent = 0.0f;
    bool _bodenfeuchteGueltig = false; // false = noch kein Wert empfangen, Sensor wird ignoriert

    float _Kulturfaktor_Kc_Zone = 0.8f;    // Kulturfaktor (Crop Coefficient)            // als ETS-Parameter/GA veränderlich
    float _nutzbareFeldkapazitaet_nFK_Zone = 25.0f;             // nutzbare Feldkapazität [mm] nFK = nutzbare Feldkapazität (quasi ein Schwamm) - "Größe des Behälters"
    uint8_t _Schwellwert_P_Prozent_Zone = 50;       // 50 % "Erschöpfungsfaktor ohne Stress" - quasi wann der Behälter(Schwamm) nachgefüllt wird 
    float _Schwellwert_in_mm_Zone = 0;
    float _Niederschlagsrate_Zone = 10.0f;  // mm/h (MP-Rotator)P Rotator Standarddüsen = 10 mm/h  MP800 = 20 mm/h

    bool _Diagnose_Bewaesserung_gesperrt = false; // Status als Ausgang um Anzuzeigen das eine Sperre vorliegt.


    float _Wasserbilanzkonto = _nutzbareFeldkapazitaet_nFK_Zone; // Start optimistisch: Konto voll
    bool _Bewaesserungsbedarf = false;
    float _ermittelteFehlmenge_mm = 0.0;
    uint16_t _ermittelteLaufzeit_sekunden = 0;



    enum class ZonenStatus : uint8_t
    {
        Inaktiv,               // kein Bedarf
        WartetAufStart,        // Bedarf erkannt, wartet auf Freigabe durch den Koordinator im Modul
        Laeuft,                // Ventil angesteuert, geplante Laufzeit läuft
        WartetAufRueckmeldung, // geplante Laufzeit abgelaufen, eigenes KO aus, wartet auf fallende Flanke der echten Rückmeldung
        Abgeschlossen,          // Rückmeldung kam, Rückbuchung erfolgt - für heute fertig
        TimeoutFehler           // Timeout
    };
    ZonenStatus _ZonenStatus = ZonenStatus::Inaktiv;
    unsigned long _kommandoStartMillis = 0;
    bool _statusMagnetventilLetzter = false; // letzter bekannter Zustand, für Flankenerkennung
    uint32_t _ventil_Rueckmeldung_timeout_s = 60; //Sekunden timeout - bricht ab, wenn die Laufzeit über 60s geht.
    uint32_t _rueckmeldungStartMillis = 0;

    int16_t _letzterBewaesserungsTag = -1;

    void onStatusMagnetventilChanged(bool offen);
    float calc_Bodenwasserkonto(float konto_alt, float niederschlag_mm, float ETc, float nutzbareFeldkapazitaet);
    float calc_Schwellwert_in_mm(uint8_t Schwellwert_Prozent, float nutzbareFeldkapazitaet);
    bool calc_bedarf(float Wasserbilanzkonto, float Schwellwert_in_mm);
    float calc_Fehlmenge_mm (float nutzbareFeldkapazitaet, float Bodenwasserkonto_neu);
    uint16_t calc_laufzeit_sek(float fehlmenge_mm, float niederschlagsrate_mm_h);
    float calc_Zugefuehrte_Wassermenge(float laufzeit_sek, float niederschlagsrate_mm_h);
    float calc_Bodenwasserkonto_final(float Wasserbilanzkonto, float bewaesserung_mm, float nutzbareFeldkapazitaet);
    float calc_ETc(float ET0, float Kc);
    bool get_lokaleSperre();
    uint16_t getYearDay() const;
    void setKOInitialValues(void);
    void setZonenStatus(ZonenStatus status);

    String wandle_Zonenstatus_in_Text(ZonenStatus status);


    public:
    IrrigationChannel(uint8_t iChannelNumber);
    ~IrrigationChannel();

    const std::string name() override;
    void setup(bool configured) override;
    void loop() override;
    void processInputKo(GroupObject &ko) override;
    void process_Bewaesserungsberechnung_channel(float et0Gestern, float regenmengeGestern, bool Freigabe_global);
    
    bool isActive();
        
    void save();     // called by IrrigationModule::writeFlash()
    void restore();  // called by IrrigationModule::readFlash()
   
    bool hatOffenenBedarf() const;
    bool laeuftGerade() const;
    void starteBewaesserung();

};

