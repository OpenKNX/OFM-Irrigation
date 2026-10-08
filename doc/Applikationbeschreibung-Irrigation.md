<!-- DOC HelpContext="Dokumentation" -->

<!-- DOCCONTENT
Eine vollständige Applikationsbeschreibung ist unter folgendem Link verfügbar: https://github.com/openknx/OFM-Meter/blob/v1/doc/Applikationsbeschreibung-Irrigation.md
DOCCONTENT -->

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

<!-- DOCEND -->

<!-- DOC -->
#### **Beschreibung des Kanals**

Der hier angegebene Name dient zur eindeutigen Wiedererkennung der Bewässerungszone, zum Beispiel „Rasen“, „Beet“ oder „Vorgarten“. Er wird auch als Titel der Zonenseite verwendet.

<!-- DOC -->
#### **Kanalaktivität**

Hier wird festgelegt, ob die Bewässerungszone verwendet wird.
<!-- DOC -->
#### Deaktiviert
Die Zone wird nicht verwendet. Ihre Detailseite mit den Zonenparametern und Kommunikationsobjekten wird nicht angezeigt.
<!-- DOC -->
#### Aktiv
Die Zone wird in die Bewässerungssteuerung einbezogen. Ihre Detailseite mit den Zonenparametern, Eingängen und Ausgängen wird angezeigt.

#### Suspendiert
Eine aktive Zone kann vorübergehend von der Bewässerungssteuerung ausgenommen werden, ohne ihre Konfiguration zu verlieren. Nach dem Aufheben der Suspendierung nimmt sie wieder an der Steuerung teil.


### Bewässerung: Allgemein

Geräteweite Wetterdaten und Berechnungsergebnisse für die Wasserbilanz-Bewässerungssteuerung. Diese Werte gelten für **alle** Bewässerungszonen gemeinsam – Details je Zone werden auf der jeweiligen Zonenseite eingestellt.

Die Berechnung folgt der Hargreaves-Samani-Methode (FAO-56) und wird einmal täglich beim Tageswechsel (00:00 Uhr) mit den Temperaturwerten des abgelaufenen Tages durchgeführt. Eine ausführliche Herleitung der Formeln findet sich in der Projekt-Dokumentation (`Doku/Bewaesserung-Wasserbilanz.md`).
<!-- DOC -->
#### Aktuelle Temperatur

Laufender Außentemperaturwert (z. B. von einer Wetterstation oder aus Home Assistant), DPT 9.001. Die Firmware sammelt daraus fortlaufend Tageshöchst- und Tagestiefstwert; erst beim Tageswechsel werden daraus Tmax, Tmin und der Mittelwert des abgelaufenen Tages gebildet und für die ET0-Berechnung verwendet.
<!-- DOC -->
#### Regenmenge heute

Bereits aufsummierter Tages-Niederschlag in mm (= l/m²), typischerweise von einem Regenmesser. Der Eingang unterstützt DPT 9.026 (2-Byte-Gleitkomma) oder DPT 14.xxx (4-Byte-Gleitkomma). Beim Tageswechsel wird der Wert als Niederschlag des abgelaufenen Tages in die Wasserbilanz aller Zonen eingerechnet.

<!-- DOC -->
#### Globale Sperre Bewaesserung

Geräteweite Sperre. Nur wenn dieses Objekt false ist **und** die Zonensperre der jeweiligen Zone false ist **und** die Wasserbilanz einen Bedarf ermittelt hat, wird tatsächlich bewässert. Gedacht für übergeordnete Bedingungen wie Wasserdruck, Störungsfreiheit oder eine manuelle Anlagensperre.
<!-- DOC -->
#### ET0

Berechnete Referenz-Evapotranspiration des abgelaufenen Tages in mm/Tag (DPT 9.026). Wird einmal täglich beim Tageswechsel neu berechnet und aktualisiert.
<!-- DOC -->
#### Durchschnittstemperatur heute / gestern, Temperatur max/min heute / gestern

Diagnose-Ausgänge zur Nachvollziehbarkeit der Tagesaggregation. "Heute" zeigt den aktuellen Zwischenstand der laufenden Sammlung, "gestern" die beim letzten Tageswechsel eingefrorenen, für die ET0-Berechnung tatsächlich verwendeten Werte.

<!-- DOC -->
### Bewässerungszone

Parameter, Eingänge und Ausgänge einer einzelnen Bewässerungszone. Die zonenweiten Wetterdaten (Temperatur, Regenmenge, ET0) werden zentral auf der Seite "Allgemein" gepflegt – hier werden nur die Werte eingestellt, die diese eine Zone von anderen unterscheiden.
<!-- DOC -->
#### Niederschlagsrate der Zone [mm/h]

Hydraulische Ausbringrate der eingesetzten Bewässerungstechnik dieser Zone in mm/h (z. B. Sprinklerdüsen, Tropfrohr). Kein pflanzenbezogener Wert, sondern abhängig von der verbauten Hardware. Bestimmt zusammen mit der berechneten Fehlmenge die Ventil-Laufzeit.
<!-- DOC -->
#### Bewässerungsschwelle [%]

Anteil der nutzbaren Feldkapazität (nFK) in Prozent, der aufgebraucht sein darf, bevor eine Bewässerung ausgelöst wird (Bewässerung startet, wenn das Bodenwasserkonto unter diesen Schwellwert fällt). In der Bewässerungswissenschaft als "p" bzw. "depletion fraction" bezeichnet. 50 % ist ein gängiger Startwert.
<!-- DOC -->
#### Nutzbare Feldkapazität (nFK) [mm]

Wassermenge in mm, die der Boden dieser Zone in der durchwurzelten Schicht pflanzenverfügbar speichern kann (nFK = Feldkapazität − permanenter Welkepunkt). Bestimmt die Obergrenze des Bodenwasserkontos dieser Zone.
<!-- DOC -->
#### Kc-Faktor der Zone

Kulturfaktor (Crop Coefficient). Skaliert die geräteweit berechnete Referenz-Evapotranspiration (ET0) auf den tatsächlichen Wasserbedarf der in dieser Zone vorhandenen Vegetation (ETc = ET0 × Kc).
Eingabe in der ETS mit Division durch 10. Beispiel: Eingabewert 80 entspricht dann 0,8. 
<!-- DOC -->
#### ... über KO vorgeben?

Schaltet den jeweiligen Parameter von einem festen ETS-Wert auf eine Vorgabe per Kommunikationsobjekt um. Solange diese Option deaktiviert ist, gilt ausschließlich der eingestellte ETS-Wert.
<!-- DOC -->
#### Zonensperre

Zusätzliche, nur für diese Zone geltende Sperre. Eine Bewässerung dieser Zone findet nur statt, wenn zusätzlich zur geräteweiten "Globalen Sperre" (siehe Seite "Allgemein") auch diese Zonensperre nicht aktiv ist. Ermöglicht es, einzelne Zonen unabhängig stillzulegen (z. B. Neuansaat, Bauarbeiten), ohne die gesamte Anlage zu sperren.
<!-- DOC -->
#### Bewässerungsbedarf

Ausgang (DPT 1.001): zeigt an, ob das Bodenwasserkonto dieser Zone aktuell unter der Bewässerungsschwelle liegt.
<!-- DOC -->
#### Fehlmenge

Ausgang in mm: rechnerisch fehlende Wassermenge bis zur vollständigen nFK, sobald ein Bedarf ermittelt wurde.
<!-- DOC -->
#### Laufzeit

Ausgang in Sekunden: aus der Fehlmenge und der Niederschlagsrate dieser Zone berechnete Ventil-Öffnungsdauer.
<!-- DOC -->
#### Wasserbilanzkonto

Ausgang in mm: aktueller Kontostand des simulierten Bodenwasserspeichers dieser Zone. Wird täglich um Niederschlag und ETc fortgeschrieben und nach jeder Bewässerung um die zugeführte Wassermenge erhöht.
<!-- DOC -->
### Bewässerungszonen

Übersicht aller verfügbaren Bewässerungszonen (z. B. Rasen, Beet). Jede Zone entspricht einem eigenen Kanal mit eigenem Bodenwasserkonto, eigenen Zonenparametern (Niederschlagsrate, nFK, Kc, Schwellwert) und eigener (Zonen-)Sperre.

<!-- DOC -->
#### Aktiv

Schaltet die jeweilige Zone frei. Erst wenn eine Zone aktiviert ist, erscheint dazu die zugehörige Detailseite mit allen Zonenparametern, Eingängen und Ausgängen. Nicht benötigte Zonen sollten deaktiviert bleiben, um die Anzahl der Kommunikationsobjekte gering zu halten.

<!-- DOC -->
#### Beschreibung der Zone

Freitext zur Wiedererkennung der Zone (z. B. "Rasenzone Vorgarten"). Wird auch als Seitentitel der zugehörigen Detailseite verwendet.

<!-- DOC -->
#### Zonen-Kombinationen
Normalerweise werden Zonen standardmäßig nacheinander bewässert. So kann der Druck und die Wassermenge gewährleistet werden.
Hat man aber zwei oder mehrere kleinere Zonen (z.B. mehrere Beetzonen), so können über diese Einstellungen mehrere Zonen zusammen, d.h. gleichzeitig, bewässert werden.
Die berechnete Zeitdauer der einzelnen Zonen wird dabei berücksichtigt. 

<!-- DOC -->
#### Verwendung Bodenfeuchtesensor

Das Modell arbeitet rein rechnerisch: Es kennt weder den tatsächlichen Wassergehalt des Bodens noch Regen, den der Regenmesser verpasst hat. Ein Bodenfeuchtesensor liefert dafür einen Messwert, mit dem sich das Modell absichern oder korrigieren lässt.

Wie der Wert verwendet wird, legt der ETS-Parameter fest:
*Nicht verwendet* --> Der Sensor wird ignoriert. Die Wasserbilanz läuft rein modellbasiert. 
*Bodenwasserkonto korrigieren* --> Der Messwert ersetzt beim Tageswechsel den errechneten Kontostand. 
*Zusätzliche Sicherheitsbedingung* --> Das Modell bleibt führend. Der Sensor kann eine Bewässerung nur verhindern.

<!-- DOC -->
#### Bewässerungsfenster Startstunde

Stunde (0 bis 23), ab der die Bewässerung beginnen darf. Zusammen mit der **Startminute** ergibt sich die Startzeit des Bewässerungsfensters.

Zum Start prüft das Modul, welche Zonen Bedarf haben, und startet sie nacheinander. Zonen, die laut Kompatibilitätsmatrix nicht gleichzeitig laufen dürfen, warten, bis die laufende Zone fertig ist.

Das Fenster öffnet einmal pro Durchlauf. Sind alle Zonen früher fertig, bleibt das Fenster bis zum eingestellten Ende geschlossen und öffnet erst am nächsten Tag wieder.

**Hinweise**

- Die Uhrzeit muss gültig sein (Zeitsynchronisation über den Bus). Ohne gültige Zeit wird nicht bewässert.
- Liegt die Startzeit nach der Endzeit, gilt das Fenster über Mitternacht (z. B. 22:00 bis 06:00).
- Sind Startzeit und Endzeit identisch, ist das Fenster leer und es wird nie bewässert.

<!-- DOC -->
#### Bewässerungsfenster Startminute

Minute (0 bis 59) der Startzeit. Zusammen mit der **Startstunde** legt sie fest, wann das Bewässerungsfenster beginnt.

Beispiel: Startstunde 5 und Startminute 30 ergeben die Startzeit 05:30 Uhr.


<!-- DOC -->
#### Bewässerungsfenster Endstunde

Stunde (0 bis 23), zu der das Bewässerungsfenster endet. Zusammen mit der **Endminute** ergibt sich die Endzeit.

Das Ende ist eine feste Grenze:

- Es werden keine neuen Zonen mehr gestartet, wenn nur noch sehr wenig Zeit bleibt (weniger als 2 Minuten).
- Passt die berechnete Laufzeit einer Zone nicht mehr komplett in das Fenster, wird sie auf die verbleibende Zeit gekürzt.
- Zonen, die nicht mehr bedient werden konnten, werden verworfen. Ihr Bedarf wird am nächsten Tag über das Wasserbilanzkonto neu berechnet.


**Hinweise**

- Liegt die Endzeit vor der Startzeit, endet das Fenster am Folgetag (z. B. Start 22:00, Ende 06:00).
- Da in ganzen Minuten gerechnet wird, kann eine Zone bis zu einer Minute über die Endzeit hinaus laufen.

<!-- DOC -->
#### Bewässerungsfenster Endminute

Minute (0 bis 59) der Endzeit. Zusammen mit der **Endstunde** legt sie fest, wann das Bewässerungsfenster endet.

Beispiel: Endstunde 7 und Endminute 0 ergeben die Endzeit 07:00 Uhr. Eine Zone, die um 06:50 Uhr startet und länger als 10 Minuten benötigt, wird auf 10 Minuten gekürzt.

**Empfehlung**

Plane das Fenster so groß, dass alle Zonen im Normalfall vollständig bewässert werden können. 