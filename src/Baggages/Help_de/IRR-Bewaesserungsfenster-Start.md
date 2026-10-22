### Bewässerungsfenster Start

*Startstunde*

Stunde (0 bis 23), ab der die Bewässerung beginnen darf. Zusammen mit der *Startminute* ergibt sich die Startzeit des Bewässerungsfensters.

*Startminute*

Minute (0 bis 59) der Startzeit. Zusammen mit der *Startstunde* legt sie fest, wann das Bewässerungsfenster beginnt.
Beispiel: Startstunde 5 und Startminute 30 ergeben die Startzeit 05:30 Uhr.


Zum Start prüft das Modul, welche Zonen Bedarf haben, und startet sie nacheinander. Zonen, die laut Kompatibilitätsmatrix nicht gleichzeitig laufen dürfen, warten, bis die laufende Zone fertig ist.

Das Fenster öffnet einmal pro Durchlauf. Sind alle Zonen früher fertig, bleibt das Fenster bis zum eingestellten Ende geschlossen und öffnet erst am nächsten Tag wieder.

**Hinweise**

- Die Uhrzeit muss gültig sein (Zeitsynchronisation über den Bus). Ohne gültige Zeit wird nicht bewässert.
- Liegt die Startzeit nach der Endzeit, gilt das Fenster über Mitternacht (z. B. 22:00 bis 06:00).
- Sind Startzeit und Endzeit identisch, ist das Fenster leer und es wird nie bewässert.
- Eine Sperre (global, Zone oder Bodenfeuchte), die erst nach dem Fensterstart gesetzt wird, verhindert den Start der betroffenen Zone. Läuft die Zone bereits, wird sie gestoppt.

