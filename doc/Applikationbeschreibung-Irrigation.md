<!-- DOC HelpContext="Dokumentation" -->



### **Kanaldefinition**

<!-- DOC -->
#### **Beschreibung des Kanals**

Der hier angegebene Name wird an verschiedenen Stellen verwendet, um diesen Kanal eindeutig zu identifizieren. z.B. Küche, Bad, etc.

<!-- DOC -->
#### **Kanalaktivität**

Hier kann man einen PowerAmp-Kanal aktivieren.

#### Inaktiv
Dieser Kanal ist inaktiv. Alle Einstellungen und Kommunikationsobjekte sind ausgeblendet.

##### **Aktiv**

Dieser Kanal ist aktiv und kann vollständig parametriert werden.

##### **Funktionslos**

Dieser Kanal ist inaktiv. Er kann vollständig parametriert sein, sendet und empfängt jedoch keine Telegramme.  
Dies eignet sich zur Fehlersuche, um einen Kanal testweise außer Betrieb zu nehmen, ohne dessen Konfiguration zu verlieren.


### Bewässerung: Allgemein

Geräteweite Wetterdaten und Berechnungsergebnisse für die Wasserbilanz-Bewässerungssteuerung. Diese Werte gelten für **alle** Bewässerungszonen gemeinsam – Details je Zone werden auf der jeweiligen Zonenseite eingestellt.

Die Berechnung folgt der Hargreaves-Samani-Methode (FAO-56) und wird einmal täglich beim Tageswechsel (00:00 Uhr) mit den Temperaturwerten des abgelaufenen Tages durchgeführt. Eine ausführliche Herleitung der Formeln findet sich in der Projekt-Dokumentation (`Doku/Bewaesserung-Wasserbilanz.md`).
<!-- DOC -->
#### Aktuelle Temperatur

Laufender Außentemperaturwert (z. B. von einer Wetterstation oder aus Home Assistant), DPT 9.001. Die Firmware sammelt daraus fortlaufend Tageshöchst- und Tagestiefstwert; erst beim Tageswechsel werden daraus Tmax, Tmin und der Mittelwert des abgelaufenen Tages gebildet und für die ET0-Berechnung verwendet.
<!-- DOC -->
#### Regenmenge heute

Bereits aufsummierter Tages-Niederschlag in mm (= l/m²), DPT 9.026, typischerweise von einem Regenmesser/Rainclick-Modul. Wird beim Tageswechsel als Niederschlagswert des abgelaufenen Tages in die Wasserbilanz aller Zonen eingerechnet.
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
<!-- DOC -->
#### ... über KO vorgeben?

Schaltet den jeweiligen Parameter von einem festen ETS-Wert auf eine Vorgabe per Kommunikationsobjekt um – z. B. um den Kc-Faktor saisonal aus Home Assistant nachzuführen. Solange diese Option deaktiviert ist, gilt ausschließlich der links eingestellte ETS-Wert.
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
Hat man aber zwei oder mehrere kleinere Zonen (z.B. mehrere Beetzonen), so können über diese Einstellungen mehrer Zonen zusammen, d.h. gleichzeitig, bewässert werden.
Die berechnete Zeitdauer der einzelnen Zonen wird dabei berücksichtigt. 

