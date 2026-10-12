### Bewässerungsfenster Ende

*Endstunde*
Stunde (0 bis 23), zu der das Bewässerungsfenster endet. Zusammen mit der *Endminute* ergibt sich die Endzeit.

*Endminute*
Minute (0 bis 59) der Endzeit. Zusammen mit der *Endstunde* legt sie fest, wann das Bewässerungsfenster endet.
Beispiel: Endstunde 6 und Endminute 0 ergeben die Endzeit 06:00 Uhr. 


Das Ende ist eine feste Grenze:
- Es werden keine neuen Zonen mehr gestartet, wenn nur noch sehr wenig Zeit bleibt (weniger als 10 Minuten).
- Passt die berechnete Laufzeit einer Zone nicht mehr komplett in das Fenster, wird sie auf die verbleibende Zeit gekürzt.
- Zonen, die nicht mehr bedient werden konnten, werden verworfen. Ihr Bedarf wird am nächsten Tag über das Wasserbilanzkonto neu berechnet.
  Beispiel:
- Eine Zone, die um 05:49 Uhr startet und länger als 10 Minuten benötigt, wird auf 11 Minuten gekürzt.
- Eine Zone, die um 05:55 Uhr starten möchte und eine berechnete Laufzeit von 30 Minuten hat, wird für diesen Tag verworfen. 

**Hinweise**

- Liegt die Endzeit vor der Startzeit, endet das Fenster am Folgetag (z. B. Start 22:00, Ende 06:00).
- Da in ganzen Minuten gerechnet wird, kann eine Zone bis zu einer Minute über die Endzeit hinaus laufen.


**Empfehlung**

Plane das Fenster so groß, dass alle Zonen im Normalfall vollständig bewässert werden können. 
