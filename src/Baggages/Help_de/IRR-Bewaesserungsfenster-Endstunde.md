### Bewässerungsfenster Endstunde

Stunde (0 bis 23), zu der das Bewässerungsfenster endet. Zusammen mit der **Endminute** ergibt sich die Endzeit.

Das Ende ist eine feste Grenze:

- Es werden keine neuen Zonen mehr gestartet, wenn nur noch sehr wenig Zeit bleibt (weniger als 2 Minuten).
- Passt die berechnete Laufzeit einer Zone nicht mehr komplett in das Fenster, wird sie auf die verbleibende Zeit gekürzt.
- Zonen, die nicht mehr bedient werden konnten, werden verworfen. Ihr Bedarf wird am nächsten Tag über das Wasserbilanzkonto neu berechnet.


**Hinweise**

- Liegt die Endzeit vor der Startzeit, endet das Fenster am Folgetag (z. B. Start 22:00, Ende 06:00).
- Da in ganzen Minuten gerechnet wird, kann eine Zone bis zu einer Minute über die Endzeit hinaus laufen.

