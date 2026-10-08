### Dokumentation


Eine vollständige Applikationsbeschreibung ist unter folgendem Link verfügbar: https://github.com/openknx/OFM-Meter/blob/v1/doc/Applikationsbeschreibung-Irrigation.md

Das Bewässerungsmodul steuert bis zu 12 Bewässerungszonen bedarfsgesteuert anhand von Wetterdaten. Statt nach festem Zeitplan zu bewässern, berechnet es täglich, wie viel Wasser jede Zone benötigt:

* **Referenzverdunstung (ET0)** nach Hargreaves-Samani aus Temperatur (Minimum, Maximum) und Standort
* **Zonenverdunstung (ETc)** über den Kulturfaktor (Kc) der jeweiligen Zone
* **Wasserbilanzkonto** je Zone aus Niederschlag, Verdunstung und tatsächlich ausgebrachter Wassermenge
* **Bewässerungsbedarf** durch Vergleich des Kontos mit einem Schwellwert in Prozent der nutzbaren Feldkapazität (nFK)

Die Zonen werden innerhalb eines **Bewässerungsfensters** gestartet. Zonen, die laut Kompatibilitätsmatrix nicht gleichzeitig laufen dürfen, werden nacheinander bewässert.

Weitere Funktionen:

* Globale Sperre und Sperre je Zone
* Optionaler Bodenfeuchtesensor je Zone (Korrektur des Wasserbilanzkontos oder zusätzliche Sperrbedingung)
* Rückmeldung des Magnetventils mit Überwachung und Rückbuchung der real gelaufenen Zeit
* Wasserbilanzkonto bleibt über einen Neustart erhalten

