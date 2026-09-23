# Wasserbilanz- und ET₀-Methode für die automatische Bewässerungssteuerung

## 1. Ziel und Grundprinzip

Die Bewässerungssteuerung basiert auf einer **Wasserbilanz des Bodens**. Dabei wird täglich abgeschätzt, wie viel pflanzenverfügbares Wasser im Boden vorhanden ist.

Das Grundprinzip lautet:

$$
\text{Bodenwasserkonto}_{heute}
=
\text{Bodenwasserkonto}_{gestern}
+
\text{Niederschlag}
-
ET_c
$$

Der Niederschlag wird über einen Regenmesser erfasst. Der Wasserverlust des Bodens wird über die **kulturspezifische Evapotranspiration \(ET_c\)** berechnet.

Dieses Prinzip entspricht grundsätzlich dem Ansatz moderner bedarfsabhängiger Bewässerungssteuerungen.

---

# 2. Evapotranspiration

## 2.1 ET₀ – Referenz-Evapotranspiration

\(ET_0\) beschreibt die Verdunstungsleistung einer standardisierten Referenzfläche unter den gegebenen Wetterbedingungen.

Sie stellt damit einen **wetterabhängigen Basiswert** dar:

> Wie viel Wasser würde eine gut mit Wasser versorgte Referenzvegetation unter den aktuellen Wetterbedingungen verlieren?

Die vollständige Berechnung erfolgt üblicherweise über die **FAO-Penman-Monteith-Gleichung**. Dafür werden unter anderem Temperatur, Luftfeuchtigkeit, Windgeschwindigkeit und Strahlung benötigt.

Für die hier verfügbare Sensorik ist jedoch eine vereinfachte Berechnung sinnvoll.

---

## 2.2 Hargreaves-Samani-Methode

Wenn lediglich Temperaturdaten zur Verfügung stehen, kann \(ET_0\) mit der **Hargreaves-Samani-Formel** abgeschätzt werden:

$$
ET_0
=
0{,}0023
\cdot
(T_{\mathrm{mean}} + 17{,}8)
\cdot
\sqrt{T_{\mathrm{max}} - T_{\mathrm{min}}}
\cdot
R_a
$$

Dabei gilt:

| Variable | Bedeutung                      |
| -------- | ------------------------------ |
| `T_mean` | mittlere Tagestemperatur in °C |
| `T_max`  | Tageshöchsttemperatur in °C    |
| `T_min`  | Tagestiefsttemperatur in °C    |
| `R_a`    | extraterrestrische Strahlung   |

\(R_a\) muss nicht gemessen werden. Der Wert lässt sich aus **Breitengrad und Kalendertag** mathematisch bestimmen.

### 2.2.1 Berechnung der extraterrestrischen Strahlung `Ra` nach FAO-56

Für die **extraterrestrische Strahlung** `Ra` nach FAO-56 kann folgende Formel verwendet werden:

$$
R_a =
\frac{24 \cdot 60}{\pi}
\cdot G_{sc}
\cdot d_r
\cdot
\left[
\omega_s \sin(\varphi)\sin(\delta)
+
\cos(\varphi)\cos(\delta)\sin(\omega_s)
\right]
$$

Dabei sind:

* `Ra` = extraterrestrische Strahlung in **MJ/(m²·Tag)**
* `Gsc` = Solarkonstante = **0,0820 MJ/(m²·min)**
* `dr` = inverse relative Entfernung Erde–Sonne
* `ωs` = Sonnenuntergangswinkel in Radiant
* `φ` = geografische Breite des Standorts in Radiant
* `δ` = solare Deklination in Radiant

Die benötigten Zwischenwerte lassen sich aus dem **Kalendertag** `J` berechnen.

#### 1. Inverse relative Entfernung Erde–Sonne

$$
d_r =
1 + 0{,}033
\cdot
\cos
\left(
\frac{2\pi}{365} \cdot J
\right)
$$

#### 2. Solare Deklination

$$
\delta =
0{,}409
\cdot
\sin
\left(
\frac{2\pi}{365} \cdot J - 1{,}39
\right)
$$

#### 3. Sonnenuntergangswinkel

Der Sonnenuntergangswinkel `ωs` wird aus dem Breitengrad `φ` und der solaren Deklination `δ` berechnet:

$$
\omega_s =
\arccos
\left(
-\tan(\varphi)
\cdot
\tan(\delta)
\right)
$$

**Wichtig für die Umsetzung als Zahlenwert in mm/Tag:** Die obige `Ra`-Formel liefert das Ergebnis in **MJ/(m²·Tag)**. Für die Hargreaves-Samani-Formel (Abschnitt 2.2) wird `Ra` jedoch in **mm/Tag** benötigt. Die Umrechnung erfolgt über den festen Faktor:

$$
R_{a,\,mm} = R_a \cdot 0{,}408
$$

(1 MJ/m² Strahlungsenergie entspricht rechnerisch 0,408 mm verdunstetem Wasser.)

#### 4. Eingangsgrößen

Für die Berechnung von `Ra` werden somit lediglich folgende Größen benötigt:

| Variable | Bedeutung                                    |
| -------- | -------------------------------------------- |
| `J`      | Kalendertag des Jahres (1–366)               |
| `φ`      | geografische Breite des Standorts in Radiant |
| `Gsc`    | Solarkonstante = 0,0820 MJ/(m²·min)          |

Die drei Zwischenwerte `dr`, `δ` und `ωs` werden daraus berechnet und anschließend in die Hauptformel für `Ra` eingesetzt.

---

# 3. ETc – tatsächlicher Wasserbedarf der Vegetation

Die Referenz-Evapotranspiration \(ET_0\) muss anschließend auf die konkrete Vegetation angepasst werden.

Dazu wird der **Kulturfaktor \(K_c\)** verwendet:

$$
ET_c = ET_0 \cdot K_c
$$

\(ET_c\) ist somit die für die jeweilige Vegetationszone angenommene tatsächliche Evapotranspiration.

### Bedeutung der Begriffe

**ET₀**

* Referenz-Evapotranspiration
* beschreibt primär den Einfluss des Wetters
* wird **einmal für das gesamte Gerät** berechnet, nicht pro Zone

**\(K_c\)**

* Crop Coefficient / Kulturfaktor
* beschreibt den Einfluss der jeweiligen Vegetation
* wird je Zone festgelegt

**\(ET_c\)**

* Evapotranspiration der konkreten Vegetation
* ergibt sich aus \(ET_0\) und \(K_c\)

Der große Vorteil dieses Ansatzes ist, dass **\(ET_0\) nur einmal berechnet werden muss**. Jede Zone verwendet anschließend ihren eigenen \(K_c\)-Wert.

---

# 4. Bodenwasserkonto

Das Bodenwasserkonto bildet den pflanzenverfügbaren Wasservorrat der jeweiligen Zone ab.

Die tägliche Fortschreibung lautet:

$$
Konto_{\mathrm{neu}}
=
\operatorname{clamp}
\left(
Konto_{\mathrm{alt}}
+
Niederschlag
-
ET_c,\,
0,\,
nFK
\right)
$$

Dabei verhindert `clamp()`, dass das Konto:

* kleiner als \(0\) wird
* größer als die vorhandene nutzbare Feldkapazität \(nFK\) wird

### Beispiel

Angenommen:

* vorheriges Konto: 15mm
* Niederschlag: 8mm
* ET_c = 5mm
* nFK = 25mm

Dann:

$$
15 + 8 - 5 = 18\,\mathrm{mm}
$$

Das neue Bodenwasserkonto beträgt somit **18 mm**.

---

# 5. nFK – nutzbare Feldkapazität

Die **nutzbare Feldkapazität \(nFK\)** beschreibt die Wassermenge, die in der durchwurzelten Bodenschicht gespeichert und von den Pflanzen tatsächlich genutzt werden kann.

Die Einheit mm entspricht damit 1 Liter auf einem Quadratmeter.

$$
1\,\mathrm{mm} = 1\,\mathrm{l/m^2}
$$

## 5.1 Zusammenhang von FK, PWP und nFK

Die drei relevanten Größen sind:

### Feldkapazität (FK)

Die Wassermenge, die ein Boden nach ausreichender Durchfeuchtung gegen die Schwerkraft zurückhält.

### Permanenter Welkepunkt (PWP)

Die Restwassermenge, die zwar noch im Boden vorhanden ist, von den Pflanzen aber nicht mehr aufgenommen werden kann.

### Nutzbare Feldkapazität

$$
nFK = FK - PWP
$$

Die nFK entspricht damit dem **pflanzenverfügbaren Wasserspeicher**.

---

## 5.2 Bedeutung für die Steuerung

Das Bodenwasserkonto kann gedanklich als Tank betrachtet werden:

```text
nFK
┌─────────────────────────┐
│        voll             │
│                         │
│    pflanzenverfügbar    │
│                         │
│─────────────────────────│ ← Bewässerungsschwelle
│                         │
│     Trockenstress       │
│                         │
└─────────────────────────┘
0 mm
```

Ist das Konto bei \(nFK\), ist der Speicher voll.

Zusätzlicher Niederschlag kann nicht mehr vollständig im betrachteten Wurzelraum gespeichert werden und wird daher durch die obere Begrenzung abgeschnitten.

---

# 6. Bewässerungsschwelle

Die Bewässerung wird nicht erst bei vollständig leerem Bodenwasserspeicher ausgelöst.

Stattdessen wird ein Anteil der nFK als **Schwellwert** verwendet.

$$
Schwellwert_{\mathrm{mm}}
=
p \cdot nFK
$$

Für die Steuerung wird beispielsweise verwendet:

$$
p = 0{,}50
$$

Damit beginnt die Bewässerung, sobald weniger als 50 % der nutzbaren Feldkapazität vorhanden sind.

---

## 6.1 Fachlicher Hintergrund: FAO-56

In der Bewässerungswissenschaft wird dieser Parameter als **\(p\) – depletion fraction for no stress** bezeichnet.

Er beschreibt den Anteil der nutzbaren Feldkapazität, der aufgebraucht werden kann, bevor Wasserstress einsetzt.

Typische Werte liegen ungefähr zwischen:

* p=0,30 bei flachwurzelnden Pflanzen und hoher Evapotranspiration
* p=0,70 bei tiefwurzelnden Pflanzen und niedriger Evapotranspiration
* p=0,50 als häufig verwendeter Standardwert

Der konkrete Wert hängt unter anderem von Pflanze, Wurzeltiefe, Boden und aktueller Verdunstungsrate ab.

Bei hoher Verdunstung kann ein niedrigerer Wert sinnvoll sein, da die Pflanze früher bewässert werden muss.

---

## 6.2 Warum 50 % als Standard?

Für die Steuerung ist \(50\,\%\) ein sinnvoller Ausgangspunkt:

$$
Schwellwert = 0{,}50 \cdot nFK
$$

Das ist kein beliebig gewählter Wert, sondern liegt im üblichen Bereich der Bewässerungsplanung.

Der Parameter sollte später bei Bedarf anhand praktischer Beobachtungen bzw. Bodenfeuchtemessungen angepasst werden.

---

# 7. Ermittlung des Bewässerungsbedarfs

Die Bedarfsprüfung lautet:

$$
Bedarf =
\begin{cases}
1, & \text{wenn } Konto_{\mathrm{neu}} < p \cdot nFK \\
0, & \text{sonst}
\end{cases}
$$

Alternativ als boolesche Bedingung:

$$
Bedarf =
\left(
Konto_{\mathrm{neu}} < p \cdot nFK
\right)
$$

---

# 8. Sperre

Der errechnete Bedarf allein darf die Bewässerung noch nicht unmittelbar starten.

Die eigentliche Sperre wird mit den bestehenden Bedingungen verknüpft:

$$
Diagnose_Bewaesserung_gesperrt
=
Bedarf
\land
Globale\_Sperre
\land
Zonen\_Sperre
$$

`Globale_Sperre` (geräteweit, ein KO) kann beispielsweise weitere Bedingungen enthalten:

* Bewässerungsanlage gesperrt
* Sperrzeit
* Störung
* nicht ausreichender Wasserdruck
* sonstige Anlagenbedingungen

`Zonen_Sperre` (pro Zone, ein eigenes KO) erlaubt zusätzlich, einzelne Zonen unabhängig voneinander stillzulegen (z. B. eine frisch gesäte Fläche, eine Baustelle im Beet), ohne die globale Sperre für die ganze Anlage zu deaktivieren.

Damit bleibt die Wasserbilanz für die **Bedarfsermittlung** zuständig, während die eigentliche Anlagenfreigabe separat behandelt wird.

---

# 9. Ermittlung der Fehlmenge

Wenn eine Bewässerung freigegeben wird, wird die fehlende Wassermenge berechnet:

$$
Fehlmenge
=
nFK - Konto_{\mathrm{neu}}
$$

Beispiel:

$$
\begin{aligned}
nFK &= 25\,\mathrm{mm} \\
Konto_{\mathrm{neu}} &= 10\,\mathrm{mm}
\end{aligned}
$$

Daraus ergibt sich:

$$
Fehlmenge
=
25 - 10
=
15\,\mathrm{mm}
$$

Es fehlen somit 15mm.

---

# 10. Umrechnung in Ventil-Laufzeit

Die Fehlmenge muss anschließend in eine Laufzeit der jeweiligen Bewässerungszone umgerechnet werden.

Dafür wird die bekannte **Niederschlagsrate** der Bewässerung verwendet:

$$
Laufzeit_{\mathrm{Sek}}
=
\frac{Fehlmenge}{Niederschlagsrate}
\cdot 3600
$$

Die Niederschlagsrate wird in mm/h angegeben.

### Beispiel Rasenzone

$$
\begin{aligned}
Fehlmenge &= 15\,\mathrm{mm} \\
Niederschlagsrate &= 10\,\mathrm{mm/h}
\end{aligned}
$$

Damit:

$$
Laufzeit
=
\frac{15}{10}
\cdot 3600
=
5400\,\mathrm{s}
$$

bzw.:

$$
5400\,\mathrm{s}
=
90\,\mathrm{min}
$$

---

# 11. Rückbuchung der Bewässerung

Nach erfolgreicher Bewässerung wird die tatsächlich eingebrachte Wassermenge wieder auf das Bodenwasserkonto gebucht:

$$
Konto_{\mathrm{final}}
=
Konto_{\mathrm{neu}}
+
\frac{Laufzeit_{\mathrm{Sek}}}{3600}
\cdot
Niederschlagsrate
$$

Idealerweise ergibt sich bei einer vollständigen Auffüllung:

$$
Konto_{\mathrm{final}} \approx nFK
$$

In einer realen Anlage kann die tatsächliche Wasserabgabe jedoch von der theoretischen Niederschlagsrate abweichen.

---

# 12. Niederschlagsrate der Bewässerung

Die Niederschlagsrate ist **kein pflanzenphysiologischer Parameter**, sondern ein rein hydraulischer Wert der eingesetzten Bewässerungstechnik.

Sie beschreibt:

> Wie viel Wasser bringt die jeweilige Bewässerungszone pro Stunde und Quadratmeter aus?

Einheit:

$$
\mathrm{mm/h}
=
\mathrm{l/(m^2 \cdot h)}
$$

Für die aktuelle Modellierung werden folgende Richtwerte verwendet:

| Zone      | Bewässerungstechnik      | Niederschlagsrate |
| --------- | ------------------------ | ----------------: |
| Rasenzone | Hunter MP-Rotatoren      |           10 mm/h |
| Beetzone  | Tropfrohr, 30 cm Abstand |           20 mm/h |

Die tatsächliche Niederschlagsrate sollte möglichst anhand der **konkret eingesetzten Düsen bzw. des Tropfrohrs** ermittelt werden.

Bei einer Änderung der Hardware muss dieser Wert entsprechend angepasst werden.

---

# 13. Zonenspezifische Parameter

Die Wetterkomponente \(ET_0\) ist für alle Zonen identisch und wird geräteweit einmal berechnet.

Die Unterschiede zwischen den Zonen werden über \(K_c\), \(nFK\), die Bewässerungsschwelle und die hydraulische Niederschlagsrate abgebildet – in der ETS als **vier Parameter pro Zonen-Kanal**, jeweils optional per KO überschreibbar (Beispielwerte Rasen-/Beetzone):

| Parameter                  |      Rasenzone |       Beetzone |
| --------------------------- | -------------: | -------------: |
| \(K_c\)                    |           0,80 |     projektabhängig |
| \(nFK\)                    |          25 mm |          20 mm |
| Bewässerungsschwelle \(p\) |           50 % |           50 % |
| Niederschlagsrate          |        10 mm/h |        20 mm/h |

Damit ergibt sich beispielsweise:

### Rasenzone

$$
nFK = 25\,\mathrm{mm}
$$

$$
p = 0{,}50
$$

$$
Schwellwert
=
0{,}50 \cdot 25
=
12{,}5\,\mathrm{mm}
$$

### Beetzone

$$
nFK = 20\,\mathrm{mm}
$$

$$
p = 0{,}50
$$

$$
Schwellwert
=
0{,}50 \cdot 20
=
10\,\mathrm{mm}
$$

---

# 14. Zusammenspiel der Sensoren

| Sensor / Datenquelle        | Verwendung                                                     |
| --------------------------- | -------------------------------------------------------------- |
| **Temperaturstation**       | \(T_{\min}\), \(T_{\max}\), \(T_{\mathrm{mean}}\) für \(ET_0\) |
| **Datum / Kalendertag**     | Berechnung von \(R_a\)
| **Regenmesser ** | tatsächlicher Niederschlag, geräteweit ein Eingang             |
| **Bewässerungsanlage**      | definierte Niederschlagsrate je Zone                           |
| **Zonenparameter**          | \(K_c\), \(nFK\), \(p\)                                        |
| **Globale Freigabe**        | geräteweite Sicherheitsbedingungen der Anlage                  |
| **Zonenfreigabe**           | zusätzliche, je Zone einzeln schaltbare Freigabe               |

Das Modell benötigt somit keine direkte Bodenfeuchtemessung, sondern simuliert den Wasserhaushalt anhand von **Wetter, Niederschlag und Vegetation**.

Ein Bodenfeuchtesensor kann später optional zur **Validierung und Kalibrierung** eingesetzt werden.

---

# 15. Gesamtablauf der Regelung (fachlich)

Der tägliche Berechnungsablauf lässt sich auf folgende Schritte reduzieren:

```text
                 ┌──────────────────┐
                 │ Wetterdaten      │
                 │ Tmin/Tmax/Tmean  │
                 └────────┬─────────┘
                          │
                          ▼
                 ┌──────────────────┐
                 │ ET₀ berechnen    │
                 │ Hargreaves       │
                 └────────┬─────────┘
                          │
                          ▼
                 ┌──────────────────┐
                 │ ETc berechnen    │
                 │ ET₀ × Kc         │
                 └────────┬─────────┘
                          │
                          ▼
┌──────────────┐  ┌──────────────────┐
│ Niederschlag │─▶│ Bodenwasserkonto │
│ Regenmesser  │  │ + Regen - ETc    │
└──────────────┘  └────────┬─────────┘
                           │
                           ▼
                  ┌──────────────────┐
                  │ Schwellwert      │
                  │ unterschritten?  │
                  └────────┬─────────┘
                           │
                     Ja    │    Nein
                     ▼     │
             ┌─────────────┐
             │ Globale +   │
             │ Zonen-      │
             │ freigabe?   │
             └──────┬──────┘
                    │ Ja
                    ▼
             ┌─────────────┐
             │ Fehlmenge   │
             │ berechnen   │
             └──────┬──────┘
                    │
                    ▼
             ┌─────────────┐
             │ Laufzeit    │
             │ berechnen   │
             └──────┬──────┘
                    │
                    ▼
             ┌─────────────┐
             │ Bewässerung │
             └──────┬──────┘
                    │
                    ▼
             ┌─────────────┐
             │ Wasser      │
             │ zurückbuchen│
             └─────────────┘
```

---

# 16. Wissenschaftliche Einordnung

Die Methode kombiniert drei Ebenen:

### 1. Wetter

$$
T_{\min},\,T_{\max},\,T_{\mathrm{mean}},\,Datum
\quad\longrightarrow\quad
ET_0
$$

Die Hargreaves-Samani-Methode liefert eine temperaturbasierte Schätzung der Referenz-Evapotranspiration.

### 2. Vegetation

$$
ET_0 \cdot K_c
\quad\longrightarrow\quad
ET_c
$$

Der Kulturfaktor überträgt den Wetterwert auf die jeweilige Vegetationszone.

### 3. Boden

$$
Wasserbestand + Niederschlag - ET_c
\quad\longrightarrow\quad
Bodenwasserkonto
$$

Anschließend wird geprüft:

$$
Bodenwasserkonto < p \cdot nFK
$$

Wenn diese Bedingung erfüllt ist und Globale Freigabe **und** Zonenfreigabe aktiv sind, wird bewässert.

Damit wird aus einem reinen Wettermodell eine **Wasserbilanzsteuerung**.

---

# 17. Grenzen und mögliche Verbesserungen

Die beschriebene Methode ist für eine automatische Gartenbewässerung gut geeignet, stellt aber eine **Modellierung** und keine direkte Messung des Bodenwassergehalts dar.

Insbesondere folgende Parameter sind Näherungen:

* \(K_c\)
* \(nFK\)
* tatsächliche Niederschlagsrate
* effektive Niederschlagsmenge
* tatsächliche Wurzeltiefe
* Mikroklima der einzelnen Zonen

Die größten Unsicherheiten liegen dabei wahrscheinlich bei \(nFK\) und \(K_c\).

## Mögliche spätere Erweiterungen

### Bodenfeuchtesensor

Ein Bodenfeuchtesensor kann verwendet werden, um die modellierte Wasserbilanz mit der tatsächlichen Bodenfeuchte zu vergleichen.

Damit könnten beispielsweise:

$$
nFK,\quad K_c,\quad p
$$

nach einigen Wochen Praxisbetrieb angepasst werden.

---

# 18. Kernformeln

Für die Implementierung reichen im Wesentlichen folgende Formeln.

### 1. Referenz-Evapotranspiration

$$
ET_0
=
0{,}0023
\cdot
(T_{\mathrm{mean}} + 17{,}8)
\cdot
\sqrt{T_{\mathrm{max}} - T_{\mathrm{min}}}
\cdot
R_{a,\,mm}
$$

### 2. Kulturspezifische Evapotranspiration

$$
ET_c = ET_0 \cdot K_c
$$

### 3. Bodenwasserkonto

$$
Konto_{\mathrm{neu}}
=
\operatorname{clamp}
\left(
Konto_{\mathrm{alt}}
+
Niederschlag
-
ET_c,\,
0,\,
nFK
\right)
$$

### 4. Bewässerungsschwelle

$$
Schwellwert = p \cdot nFK
$$

### 5. Bewässerungsbedarf

$$
Bedarf =
\left(
Konto_{\mathrm{neu}} < Schwellwert
\right)
$$

### 6. Fehlmenge

$$
Fehlmenge = nFK - Konto_{\mathrm{neu}}
$$

### 7. Laufzeit

$$
Laufzeit_{\mathrm{Sek}}
=
\frac{Fehlmenge}{Niederschlagsrate}
\cdot 3600
$$

### 8. Rückbuchung

$$
Konto_{\mathrm{final}}
=
Konto_{\mathrm{neu}}
+
\frac{Laufzeit_{\mathrm{Sek}}}{3600}
\cdot
Niederschlagsrate
$$

---

# 19. Kurzfassung

Das Modell lässt sich auf eine einfache Kette reduzieren:

```text
Wetter
  ↓
ET₀
  ↓
ET₀ × Kc
  ↓
täglicher Wasserverlust ETc
  ↓
Bodenwasserkonto
  ↑
Niederschlag
  ↓
Schwellwert unterschritten?
  ↓
Fehlmenge berechnen
  ↓
mm → Laufzeit
  ↓
Bewässerung
  ↓
Wasser zurück auf Konto buchen
```

**Die zentrale Idee:** Nicht nach einem festen Zeitplan bewässern, sondern den angenommenen Wasserbestand des Bodens kontinuierlich fortschreiben und nur dann Wasser zuführen, wenn der modellierte pflanzenverfügbare Wasservorrat einen definierten Schwellenwert unterschreitet.

---

# 20. Umsetzung in der GardenControl-Firmware

Die vorstehenden Abschnitte beschreiben das fachliche Modell unabhängig von der konkreten Implementierung. Dieser Abschnitt ordnet es der tatsächlichen Firmware zu (Datei `Bewaesserung.cpp`/`.h`) und dem ETS-Parametermodell (`Bewaesserung.share.xml`/`.templ.xml`).

## 20.1 Geräteweit vs. je Zone

Konsequent aus Abschnitt 3 und 14 abgeleitet, sind in der ETS zwei Ebenen getrennt:

**Geräteweit, genau einmal** (Seite "Allgemein"):

| ETS-Objekt                        | Bezug in dieser Doku                  |
| ---------------------------------- | -------------------------------------- |
| Eingang: Aktuelle Temperatur       | \(T\), fließt in Tmin/Tmax/Tmean ein   |
| Eingang: Regenmenge heute          | *Niederschlag* aus Abschnitt 4         |
| Eingang: Globale Sperre          | *Globale_Sperre* aus Abschnitt 8     |
| Ausgang: ET0 [mm/Tag]              | \(ET_0\) aus Abschnitt 2               |
| Ausgang: Diagnose Tmax/Tmin/Tmean heute/gestern | Zwischenwerte der Tagesaggregation |

**Je Zonen-Kanal** (Seite "Zone N", nur sichtbar wenn die Zone aktiviert ist):

| ETS-Objekt                          | Bezug in dieser Doku              |
| ------------------------------------ | ---------------------------------- |
| Parameter/KO: Niederschlagsrate      | *Niederschlagsrate* aus Abschnitt 12 |
| Parameter/KO: Schwellwert [%]        | \(p\) aus Abschnitt 6              |
| Parameter/KO: nFK [mm]               | \(nFK\) aus Abschnitt 5            |
| Parameter/KO: Kc-Faktor              | \(K_c\) aus Abschnitt 3            |
| Eingang: ZonenSperre               | *Zonen_Sperre* aus Abschnitt 8   |
| Ausgang: Bewässerungsbedarf          | *Bedarf* aus Abschnitt 7           |
| Ausgang: Fehlmenge [mm]              | *Fehlmenge* aus Abschnitt 9        |
| Ausgang: Laufzeit [s]                | *Laufzeit_Sek* aus Abschnitt 10    |
| Ausgang: Wasserbilanzkonto [mm]      | *Konto* aus Abschnitt 4            |

Die vier Zonenparameter (Niederschlagsrate, Schwellwert, nFK, Kc) sind jeweils **entweder** als ETS-Parameter fest vorgegeben **oder** per KO zur Laufzeit überschreibbar (z. B. für einen saisonal veränderlichen Kc-Wert aus Home Assistant) – umschaltbar über "... über KO vorgeben?" auf der jeweiligen Zonenseite.

## 20.2 Zeitlicher Ablauf (Ablaufplan)

Anders als eine klassische ETS-Logikschaltung mit Zeitschaltuhr-Baustein nutzt die Firmware die im Gerät ohnehin vorhandene Uhr (`openknx.time`, gespeist über KNX-Zeittelegramme), um den Tageswechsel selbst zu erkennen – ein Vergleich des aktuellen Kalendertags gegen den zuletzt gesehenen, bei jedem `loop()`-Durchlauf.

```text
00:00 Uhr  Tageswechsel erkannt
           │
           ├─ Tmax/Tmin des abgelaufenen Tages einfrieren
           ├─ Tmean = (Tmax + Tmin) / 2        (Abschnitt 2.2, FAO-56-Konvention)
           ├─ Regenmenge des abgelaufenen Tages einfrieren
           └─ Tagesaggregation für den neuen Tag zurücksetzen
           │
           ▼
00:00 Uhr  ET0 berechnen (Abschnitt 2, mit J = Kalendertag des ABGELAUFENEN Tages)
           │
           ▼
00:00 Uhr  je Zone: ETc, Bodenwasserkonto, Schwellwert, Bedarf berechnen (Abschnitt 3-7)
           │
           ├─ Bedarf = 0  → keine weitere Aktion, nächster Vergleich erst morgen
           │
           └─ Bedarf = 1  → Fehlmenge und Laufzeit berechnen und für den
                            Bewässerungsstart vormerken (Abschnitt 9-10)
           │
           ▼
04:00/05:00 Uhr (parametrierbar)
           Globale Sperre UND ZonenSperre UND Bedarf?
           │
           └─ Ja → Ventil öffnen, für die vorgemerkte Laufzeit
           │
           ▼
Laufzeit abgelaufen
           Ventil schließen, zugeführte Wassermenge zurückbuchen (Abschnitt 11)
           → neuer Kontostand ist Ausgangspunkt für den nächsten Tageswechsel
```

Zwei Entscheidungen, die von einer wörtlichen 1:1-Umsetzung der Formeln abweichen und hier bewusst dokumentiert sind:

* **ET0 wird mit dem Kalendertag des *abgelaufenen* Tages berechnet**, nicht mit dem Tag, an dem die Berechnung tatsächlich läuft (00:00 Uhr ist ja bereits der neue Tag). Da sich die astronomischen Zwischenwerte (`dr`, `δ`, `ωs`) von Tag zu Tag nur minimal ändern, wäre der Unterschied in der Praxis vernachlässigbar – exakt ist es trotzdem nur mit dem richtigen Tag.
* **Rückbuchung erfolgt zeitgesteuert (abgelaufene Laufzeit), nicht über einen Ventil-Status-Bus-Rückmeldewechsel** – da die Firmware das Ventil selbst öffnet und schließt, ist kein zusätzlicher Bus-Roundtrip nötig, um zu wissen, wann die Bewässerung beendet ist.

