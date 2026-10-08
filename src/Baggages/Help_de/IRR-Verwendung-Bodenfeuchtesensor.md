### Verwendung Bodenfeuchtesensor

Das Modell arbeitet rein rechnerisch: Es kennt weder den tatsächlichen Wassergehalt des Bodens noch Regen, den der Regenmesser verpasst hat. Ein Bodenfeuchtesensor liefert dafür einen Messwert, mit dem sich das Modell absichern oder korrigieren lässt.

Wie der Wert verwendet wird, legt der ETS-Parameter fest:
*Nicht verwendet* --> Der Sensor wird ignoriert. Die Wasserbilanz läuft rein modellbasiert. 
*Bodenwasserkonto korrigieren* --> Der Messwert ersetzt beim Tageswechsel den errechneten Kontostand. 
*Zusätzliche Sicherheitsbedingung* --> Das Modell bleibt führend. Der Sensor kann eine Bewässerung nur verhindern.

