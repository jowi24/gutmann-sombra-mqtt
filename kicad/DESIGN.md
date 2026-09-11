# Hood-Control PCB – Redesign

Diese Doku beschreibt die Schematic in `hood-control.kicad_pro` /
`hood-control.kicad_sch`. Alle Werte sind aus der Fehlersuche an der
Lochraster-Version abgeleitet (siehe CHANGELOG.md / Debugging-Session
vom 2026-09-03) sowie aus dem Blogartikel von 2017
("Do-It-Yourself IoT Modul für die Dunstabzugshaube").

**Bauweise: THT (bedrahtet).** Alle Widerstände/Kondensatoren bedrahtet,
alle ICs in DIP-Sockeln, Stecker als Pfostenleisten 2,54 mm, Taster als
6-mm-THT-Kurzhubtaster. Einziges SMD-Teil ist das ESP-12E-Modul selbst.
Der Step-Down-Wandler ist **kein diskreter Schaltregler**, sondern ein
3-poliger Steckplatz für ein fertiges Mini-Buck-Modul.

Blattformat der Schematic: **A5** (420 × 297 mm).

> **Namenskollision beachten:** Die Haubenelektronik benennt ihre eigenen
> Matrixleitungen ebenfalls J1–J8 (J1–J4 = Scan-Eingänge, J5–J6 = Tasten-
> Ausgänge, J7–J8 = LED-Ausgänge). Die Steckverbinder *dieser Platine*
> heißen ebenfalls J1–J6. Wo Verwechslungsgefahr besteht, ist im Folgenden
> "Haube-J5" bzw. "Board-J5" ausgeschrieben.

## Stückliste (BOM)

| Ref | Bauteil | Wert/Typ | Footprint | Zweck |
|---|---|---|---|---|
| U1 | ESP-12E/F | ESP8266 Modul (SMD) | `RF_Module:ESP-12E` | Microcontroller |
| U2 | CD4050BE / HEF4050BP / 74HC4050N | Hex-Buffer, **16-pol. DIP** | `Package_DIP:DIP-16_W7.62mm_Socket` | Pegelwandlung 5V→3,3V (Lesen: Scan-Bus, Haube-J7/J8) |
| U3 | **74HCT08** (z.B. SN74HCT08N) | Quad-AND, 14-pol. DIP | `Package_DIP:DIP-14_W7.62mm_Socket` | Pegelwandlung 3,3V→5V (Treiben: Haube-J5/J6) |
| U4 | **TPS63802-Buck-Boost-Modul** (eBay, PCB-Tronic24) | 1,5–5,5 V → 3,3 V, 2,7 A | `hood-control:TPS63802_Module_25.8x13.0mm` (projekteigene Bibliothek) | Stromversorgung (Ersatz für den 2017er Mini-Wandler) |
| U5 (optional) | Supervisor/Reset-IC MCP130-315 | TO-92 | `Package_TO_SOT_THT:TO-92_Inline` | sauberer Power-on-Reset, verhindert Brownout-Boot-Loops |
| R2 | 10 kΩ | 0207 bedrahtet | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal` | Pull-up GPIO0 → 3,3V, über Board-J5 Pin 1 |
| R3 | 10 kΩ | 0207 bedrahtet | s.o. | Pull-down GPIO15 → GND |
| R4 | 10 kΩ | 0207 bedrahtet | s.o. | Pull-up CH_PD/EN → 3,3V |
| R5 | 2,4 kΩ | 0207 bedrahtet | s.o. | Pull-down auf den Scan-Bus (empirisch, siehe Blogartikel 2017) |
| R6 | 10 kΩ | 0207 bedrahtet | s.o. | Pull-up RST → 3,3V |
| C1 | 470 µF / 10 V | Elko radial, D8/RM3,5 | `Capacitor_THT:CP_Radial_D8.0mm_P3.50mm` | Stützelko auf `VCC` nah am ESP – überbrückt die Regelzeit des Buck-Moduls |
| C8 | 1000 µF / 10 V | Elko radial, D10/RM5, 12,5 mm hoch (Reichelt `RD1A108M1012M128`) | `Capacitor_THT:CP_Radial_D10.0mm_P5.00mm` | Bulk-Kapazität auf `5V_IN` – puffert die strombegrenzte Haubenversorgung |
| C3–C6 | 100 nF | Keramik-Scheibe, RM5 | `Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm` | Abblockung: C3=U1(VCC), C4=U2(VCC 3,3V), C5=U3(5V_IN), C6=U4-Ausgang |
| C7 | 100 nF | Keramik-Scheibe, RM5 | `Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm` | RST-Kopplung FTDI-DTR → RST (Auto-Reset) |
| D1, D2 | 1N4148 | DO-35 | `Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal` | Entkopplung der LED-Leseleitungen Richtung Frontpanel |
| SW1 | Taster 6 mm THT | – | `Button_Switch_THT:SW_PUSH_6mm` | Reset (RST → GND) |
| SW2 | Taster 6 mm THT | – | `Button_Switch_THT:SW_PUSH_6mm` | Flash-Mode (GPIO0 → GND) |
| J1 | Pfostenleiste 1×8, 2,54 mm | "Hoodcontrol" | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | Anschluss Haubenelektronik |
| J2 | Pfostenleiste 1×6, 2,54 mm | FTDI-Header | `Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical` | DTR, RXI, TXO, VCC, n.c., GND |
| J3 | Pfostenleiste 1×8, 2,54 mm | "Frontpanel" | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | Anschluss Frontpanel |
| J4 | Pfostenleiste 1×2, 2,54 mm | "Reset" | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | externer Reset-Taster (RST → GND) |
| J5 | Pfostenleiste 1×3, 2,54 mm | "Flash/Run" | `Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical` | Jumper: 1–2 = Run, 2–3 = Flash |
| J6 | Pfostenleiste 1×2, 2,54 mm | "PWR_IN" | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | Versorgung + Masse vom Service-Anschluss der Haube |

> **Achtung bei alten Notizen:** In CHANGELOG und Blogartikel ist mit "dem großen
> Elko" immer C1 gemeint. Seit der Umverteilung der Pufferung ist **C8** der große
> Bulk-Elko (seit 2026-09-07 **1000 µF** auf `5V_IN`) und **C1** der kleinere
> Stützelko (470 µF auf `VCC`).
>
> Die Nummerierung hat Lücken (kein C2, kein R1). C2 war der alte 4,7-µF-Elko
> der RST-Kopplung und wurde durch C7 ersetzt; der 2,5-kΩ-Pull-down heißt in
> der Schematic R5, nicht R1. Ein Re-Annotate würde nur neue Abweichungen
> zwischen Doku und Schematic erzeugen und unterbleibt bewusst.

### U4 – Spannungswandler (entschieden 2026-09-07)

Bestückt wird ein **TPS63802-Buck-Boost-Modul** (eBay, Händler PCB-Tronic24,
4,79 €).

| | |
|---|---|
| IC | TPS63802 (Texas Instruments) |
| Eingang | **1,5 – 5,5 V** (Anlauf ab 1,8 V) |
| Ausgang | 3,3 / 4,2 / 5 V per Lötbrücke, **2,7 A** bei 5 V → 3,3 V |
| Maße | **25,8 × 13,0** × 3,5 mm (nachgemessen 2026-09-11) |
| Anschlüsse | vier Eckkontakte mit je 2 Bohrungen im RM 2,54: VIN links oben, VOUT rechts oben, GND links und rechts unten |
| Eigene Beschaltung | 100 µF/16 V Eingangskondensator und Betriebs-LED bereits an Bord |

**Warum Buck-Boost und nicht Buck.** Die Haube liefert 4,9 V im Leerlauf. Jeder
gängige Buck braucht mindestens 4,5 – 6 V und fällt darunter aus der Regelung –
das ist der Brownout-Boot-Loop der Lochraster-Version. Ein Buck-Boost ab 1,5 V
hat diese Kante nicht: sackt der Eingang unter 3,3 V, regelt er einfach weiter.

**Was bei Reichelt geprüft wurde und ausscheidet** (Stand 2026-09-07):

| Artikel-Nr. | Vin min | Preis | |
|---|---|---|---|
| `TSRN 1-2433` | 4,6 V | 14,10 € | 0,3 V Reserve, dreifacher Preis |
| `TSR 1-2433` | 4,75 V | 6,85 € | zu knapp |
| `R-78E33-05` | 6 V | 3,55 € | zu hoch |
| `TSR 1-2433E` | 6 V | 3,99 € | zu hoch, lieferbar erst ab 2.11.2026 |

Auch die gesamte OKI-78SR-Familie scheidet aus (3,3-V-Variante 7–36 V).

> **Achtung beim Einbau:** Das Modul kommt **ab Werk auf 4,2 V** eingestellt.
> Der ESP8266 verträgt absolut maximal 3,6 V. Also erst die Lötbrücke auf
> **3V3** umsetzen, dann das Modul allein an 4,9 V hängen, den Ausgang messen –
> und erst danach einbauen.

#### Footprint (gezeichnet 2026-09-11)

Das Modul lag vor und wurde mit dem Messschieber vermessen. Die Fotomessung
lag quer richtig (Bohrungen bei ±2,54 und ±5,08 mm von der Mittellinie), beim
Reihenabstand daneben: es sind **22,86 mm = 9 × 2,54**, nicht ~23,5 mm. Damit
liegt das ganze Lochbild auf dem 2,54-Raster.

| | |
|---|---|
| Datei | `kicad/hood-control.pretty/TPS63802_Module_25.8x13.0mm.kicad_mod` |
| Bibliothek | `hood-control` (`fp-lib-table` im Projekt, `${KIPRJMOD}`) |
| Körper | 25,8 × 13,0 mm auf `F.SilkS`/`F.Fab`, Courtyard +0,25 mm |
| Bohrbild | x = ±11,43 mm, y = ±2,54 und ±5,08 mm → 8 Bohrungen |
| Pads | 1,7 mm rund, Bohrung 1,0 mm; Pad 1 rechteckig als Pin-1-Marke |
| Pin-Nummern | 1 = VIN, 2 = GND (4 ×), 3 = VOUT – passend zu den Pins des Symbols `Converter_DCDC:OKI-78SR-12_1.0-W36-C` |

Die vier Kontakte haben je zwei Bohrungen, die auf dem Modul selbst verbunden
sind. Im Footprint tragen sie deshalb dieselbe Pad-Nummer, und die Option
`duplicate_pad_numbers_are_jumpers` steht auf `yes`: KiCad weiß damit, dass
die Verbindung im Bauteil steckt und verlangt sie nicht auf der Platine.
Trotzdem sollten beim Routen **beide** Bohrungen eines Kontakts angebunden
werden – bei 2,7 A ist das kein Luxus.

Die ursprünglich geplante zusätzliche 2,54-Reihe VIN/GND/VOUT neben dem Modul
ist entfallen: das Modul liegt flach auf der Platine und deckt seinen Platz
vollständig ab, und zwischen C6 und TP1 bleibt neben dem Körper nicht einmal
ein Millimeter frei. Als Messpunkte dienen TP3 (5V) und TP4 (3V3).

Auf dem Bestückungsdruck steht innerhalb des Umrisses **„Loetbruecke auf
3V3!"** – die Warnung von oben als Aufdruck, sichtbar solange das Modul noch
nicht sitzt.

### Hinweis zu U3 (74HCT08)

In den installierten KiCad-Bibliotheken existiert kein eigenes 74HCT08-Symbol.
Verwendet wird deshalb das Symbol `74xx:74LS08` (**identische Pinbelegung**),
das Value-Feld ist aber auf `74HCT08` gesetzt, damit BOM und Bestückungsdruck
stimmen. **Bestückt werden muss ein HCT-Typ** – nur der HCT-Eingangspegel
(V_IH ≈ 2,0 V) lässt sich sicher aus 3,3 V ansteuern.

## Beschaffung

### Bezugsquellen

| Was | Wo |
|---|---|
| U4, TPS63802-Buck-Boost-Modul | eBay, Händler **PCB-Tronic24** (Cuxhaven), Artikel-Nr. 376238468668 – <https://www.ebay.de/itm/376238468668> – 4,79 € (ab 4 Stück 4,31 €) |
| alle übrigen Bauteile | reichelt.de, Artikelnummern unten |

Das TPS63802 gibt es auch bei AliExpress (diymore, 10,4 × 7,9 mm, 2,59 €), dort
kommen aber ~3 € Einfuhrgebühr je Artikel dazu und die Lieferzeit liegt bei
~10 Tagen. Das eBay-Angebot ist damit günstiger, schneller und hat Rückgaberecht.

### Reichelt-Artikelnummern

| Ref | Stk | Artikel-Nr. | Beschreibung |
|---|---|---|---|
| C3–C7 | 5 | `X7R-5 100N` | Vielschicht-Kerko 100 nF, 50/100 V, X7R ±10 %, RM 5,0 |
| C1 | 1 | `WL1C477M0811M` | Elko 470 µF/16 V, 105 °C, Low ESR, RM 3,5, 8 × 11,5 mm |
| C8 | 1 | `RD1A108M1012M128` | Elko 1000 µF/**10 V**, 105 °C, RM 5, D10 × **12,5 mm** |
| R2, R3, R4, R6 | 4 | `METALL 10,0K` | 10,0 kΩ, 0207, 0,6 W, 1 % |
| R5 | 1 | `METALL 2,40K` | 2,40 kΩ, 0207, 0,6 W, 1 % |
| D1, D2 | 2 | `1N 4148` | Schalt-Diode 100 V / 150 mA, DO-35 |
| SW1, SW2 | 2 | `DIP DTS-61K` | Kurzhubtaster 6 × 6 × 4,3 mm, Print |
| U2 | 1 | `CD 4050BE TEX` | CMOS Hex Non-Inverting Buffer, 3–18 V, DIP-16 |
| U3 | 1 | `74HCT 08` | AND-Gate, 4-fach, 4,5–5,5 V, DIP-14 |
| U2 (Fassung) | 1 | `GS 16` | IC-Sockel 16-polig, RM 7,62 |
| U3 (Fassung) | 1 | `GS 14` | IC-Sockel 14-polig, RM 7,62 |
| J2, J4, J5 | 1 | `ECON SL40G1` | Stiftleiste 1 × 40, gerade, 2,54 mm (11 Pins gebraucht) |
| J5 | 1 | `JUMPER 2,54 SW` | Kurzschlussbrücke RM 2,54 |
| Werkzeug | 1 | `ANMAS MG81000N` | Kopfbandlupe mit LED, 5 Linsen |

**J1, J3 und J6 stehen bewusst nicht in der Liste** – die werden direkt
verdrahtet, dafür braucht es nur Litze (siehe „Platzierung").

Ein paar Hinweise zur Auswahl, damit sie nicht wieder verlorengeht:

* **`X7R-5 100N`, nicht `KERKO 100N`.** Letzterer ist zwar einen Cent billiger,
  aber Y5V mit −20/+80 % – die Kapazität bricht mit Temperatur und Spannung weg.
* **C8 in 10 V, nicht 16 V.** Alle 1000-µF-Typen mit 16 V bauen bei Reichelt
  mindestens 16 mm hoch und reißen damit das 14-mm-Limit. Die 10-V-Variante gibt
  es in D10 × 12,5 mm, und 10 V auf einer 4,9-V-Schiene ist doppelte Reserve.
* **R5 mit 2,40 kΩ.** Bis 2026-09-07 stand im Schaltplan 2,5 kΩ – kein Normwert,
  die E24-Reihe hat 2,4 k und 2,7 k. Für einen Pull-down auf dem Scan-Bus ist der
  Wert unkritisch (er stammt empirisch aus dem Blogartikel von 2017). Schaltplan
  und Board sind auf **2,4 kΩ** nachgezogen.
* **M2-Schrauben führt Reichelt nicht** – die Suche liefert nur M.2-SSD-Zubehör.

### Bestellt am 2026-09-07

| Artikel-Nr. | Stk | Anmerkung |
|---|---|---|
| `X7R-5 100N` | 10 | 5 gebraucht |
| `WL1C477M0811M` | 2 | 1 gebraucht |
| `RD1A108M1012M128` | 4 | 1 gebraucht |
| `METALL 10,0K` | 10 | 4 gebraucht |
| `METALL 2,40K` | 10 | 1 gebraucht |
| `1N 4148` | 5 | 2 gebraucht |
| `DIP DTS-61K` | 2 | 2 gebraucht, **kein Ersatz** |
| `GS 16` | 2 | 1 gebraucht |
| `GS 14` | 2 | 1 gebraucht |
| `CD 4050BE TEX` | 1 | Reserve, U2 ist vorhanden |
| `74HCT 08` | 1 | Reserve, U3 ist vorhanden |
| `ANMAS MG81000N` | 1 | Werkzeug |
| TPS63802-Modul (eBay) | 1 | |

Nicht mitbestellt, weil **vorhanden**: Stiftleiste 2,54 mm (für J2, J4, J5),
Kurzschlussbrücke für J5 und die vier M2-Schrauben. Die Bestellung ist damit
vollständig – es fehlt nichts mehr.

## GPIO-Zuordnung (aus src/main.cpp)

| GPIO | Funktion | Über welchen Levelshifter |
|---|---|---|
| GPIO13 | Scan-Eingang der Haube (Interrupt, RISING) | U2 (4050), Gatter C (Pin 7 → 6) |
| GPIO5  | LED-Status Haube-J7 lesen | U2 (4050), Gatter B (Pin 5 → 4) |
| GPIO4  | LED-Status Haube-J8 lesen | U2 (4050), Gatter A (Pin 3 → 2) |
| GPIO12 | Tastenemulation Haube-J5 (Ausgang) | U3 (HCT08), Gatter A (Pin 1+2 → 3) |
| GPIO14 | Tastenemulation Haube-J6 (Ausgang) | U3 (HCT08), Gatter B (Pin 4+5 → 6) |
| GPIO2  | Onboard-Status-LED (active LOW) | – (nur intern im ESP-12-Modul, No-Connect) |
| GPIO0  | Flash-Taster / WLAN-Config-Reset beim Boot | – (lokal, SW2 + R2 über Board-J5) |
| CH_PD  | Enable, muss beim Boot HIGH sein | – (lokal, R4) |
| GPIO15 | Boot-Strapping, muss beim Boot LOW sein | – (lokal, R3) |
| GPIO16 | unbenutzt (kein Deep-Sleep-Wakeup vorgesehen) | – |
| GPIO9/GPIO10 | **nicht verwenden** – bei ESP-12 intern für Flash-Zugriff (QIO) reserviert | – |

## Stromversorgung und Masse

Beide kommen **nicht** über das 8-polige Steuerkabel, sondern über den
separaten Service-Anschluss der Haubenelektronik (im Blogartikel Markierung
3 = Masse, 4 = Versorgung), abgegriffen mit einem gekürzten ISA-Bus-Slot:

* **ca. 4,8 V, belastbar mit ca. 500 mA** (experimentell mit Belastungs-
  widerstand bestimmt)
* Das RJ45-Kabel zwischen Haube und Frontpanel führt **keine Referenzmasse**.
  Für das Frontpanel allein ist das nicht nötig, für Messung und aktive
  Ansteuerung schon. Das Gehäuse als Bezugsmasse funktioniert nicht.

Daraus folgt für J1/J3: **dort liegen weder 5V_IN noch GND** – das ist
korrekt so und kein Versehen.

### Versorgungsnetze

* `5V_IN` → J6/1, U4/VIN, U3/VCC (74HCT08 läuft auf 5 V), C5, C8
* `VCC` (3,3 V) → U4/VOUT, U1, U2/VDD, R2, R4, R6, J2/4, C1, C3, C4, C6
* `GND` → J6/2, gemeinsame Masse
* Alle drei Netze tragen je ein `PWR_FLAG` (`#FLG01` auf GND, `#FLG02` auf
  5V_IN); VCC wird vom `power_out`-Pin des Wandlermoduls getrieben.

### Dimensionierung der Pufferung

Beim WiFi-Senden zieht der ESP kurzzeitig ~350 mA aus 3,3 V, was über den
Wandler ~270 mA aus 4,9 V bedeutet. Die Quelle liefert 500 mA – **der Strom
war also nie der Engpass.** Der Engpass war die *Untergrenze*: sackt die
Eingangsspannung unter die minimale Eingangsspannung eines Buck-Moduls
(4,5 – 6 V je nach Typ), fällt es aus der Regelung. Das ist der
Brownout-Boot-Loop aus der Lochraster-Version.

**Mit dem TPS63802 (1,5 – 5,5 V) gibt es diese Kante nicht mehr**, und die
Pufferung durfte deshalb am 2026-09-07 von 3300 µF auf **1000 µF** schrumpfen.
Der Kondensator überbrückt jetzt nur noch die Regelzeit der Haubenversorgung,
nicht mehr ein Dropout-Risiko. Nebeneffekt: C8 steht wieder aufrecht (12,5 mm
statt 35 mm liegend) und der 13,5 × 39 mm große Streifen im Layout ist frei.

Deshalb ist die Pufferung auf beide Seiten verteilt (entschieden 2026-09-06):

| Netz | Kondensatoren |
|---|---|
| `5V_IN` | C5 (100 nF) + **C8, 1000 µF/10 V** – puffert die 500-mA-Quelle |
| `VCC` | C3/C4/C6 (100 nF) + **C1, 470 µF/10 V** – überbrückt die ~100 µs Regelzeit |

Die große Kapazität gehört bewusst **nicht** auf den Ausgang: dort reichen
330–470 µF, und sehr große Werte können beim Einschalten die Strombegrenzung
des Buck-Moduls auslösen.

## Reset- und Flash-Beschaltung

| Netz | Bauteile |
|---|---|
| `/RST` | U1/1, R6 (Pull-up), C7 (Kopplung von FTDI-DTR), SW1, J4 |
| `Net-(U1-EN)` | U1/3, R4 (eigener Pull-up) |
| `/GPIO0` | U1/18, SW2, Board-J5 Pin 2 |
| `/FTDI_DTR` | J2/1, C7 |

**EN und RST haben bewusst getrennte Pull-ups.** Ein gemeinsamer Widerstand
würde beide Pins zu einem Netz verschmelzen: SW1 und J4 würden dann den Chip
abschalten statt zurückzusetzen, und ein späteres RC-Glied auf EN würde über
den kapazitiven Teiler mit C7 den Auto-Reset unbrauchbar machen.

### Auto-Reset über J2

esptool fährt beim Verbinden die Sequenz `DTR=0/RTS=1 → 100 ms → DTR=1/RTS=0`.
J2 führt nur **DTR** heraus (Pin 5 ist beim verwendeten FTDI-Adapter CTS, ein
Eingang des Adapters, und kann nichts treiben). Damit gilt:

* **Auto-Reset funktioniert** – die DTR-Flanke wird über C7 auf RST gekoppelt.
* **Auto-Flash funktioniert nicht** – GPIO0 wird nicht ferngesteuert.
* Nach dem Flashen bleibt der ESP im Bootloader, weil esptools `hard_reset`
  ausschließlich RTS schaltet. SW1 drücken (oder auf die DTR-Flanke beim
  Schließen des Ports hoffen).

Voller Auto-Flash würde einen Adapter mit RTS **und** das kreuzgekoppelte
NPN-Paar der NodeMCU-Schaltung erfordern. Bewusst nicht umgesetzt: seriell
geflasht wird nur beim Erstflash und zur Rettung, im Alltag läuft OTA
(`[env:nodemcuv2-ota]`).

### Jumper Board-J5 (Flash/Run)

| Stellung | Wirkung |
|---|---|
| **1–2 (Normalbetrieb)** | R2 zieht GPIO0 auf 3,3 V – der ESP bootet die Firmware |
| 2–3 | GPIO0 fest auf GND – der ESP bootet in den Flash-Mode |

Der Jumper steht im Normalbetrieb auf **1–2**. Der Pull-up R2 wirkt dadurch
nur bei gestecktem Jumper; ohne Jumper floatet GPIO0 und der Boot ist nicht
zuverlässig. Das ist bewusst so gewählt (ein Bauteil statt zwei, Flash-Mode
ohne Tasterakrobatik) – der Jumper muss dafür immer stecken.

## Unbenutzte Gatter (wichtig!)

CMOS- und HCT-Eingänge dürfen **nicht floaten** (Oszillation, Querstrom,
Störungen). Deshalb:

* U2 (4050): die drei unbenutzten Buffer D/E/F – **Eingänge (Pin 9, 11, 14)
  fest auf GND**, Ausgänge (Pin 10, 12, 15) offen mit No-Connect-Flag.
* U3 (74HCT08): die beiden unbenutzten Gatter C/D – **beide Eingänge je Gatter
  (Pin 9+10 bzw. 12+13) fest auf GND**, Ausgänge (Pin 8, 11) offen mit No-Connect.

## Steckerbelegung J1 / J3 (je 8-polig)

Im Original wurde ein altes Netzwerkkabel verwendet. Aderfarben nach **T568B**.
Die Platine klinkt sich in die vorhandene Verbindung Haube ↔ Frontpanel ein.

| Pin | Netz | J1 (Haube) | J3 (Frontpanel) | T568B |
|---|---|---|---|---|
| 1 | `LED_CH_2` | LED-Status Haube-J7 → U2/5 → GPIO5 | über D1 (Anode) auf dasselbe Netz | orange-weiß |
| 2 | `LED_CH_1` | LED-Status Haube-J8 → U2/3 → GPIO4 | über D2 (Anode) auf dasselbe Netz | orange |
| 3 | `BTN_CH_1` | Tastenemulation Haube-J5, von U3/3 getrieben | durchverbunden | grün-weiß |
| 4 | `BTN_CH_2` | Tastenemulation Haube-J6, von U3/6 getrieben | durchverbunden | blau |
| 5 | – | ↔ J3/5 | ↔ J1/5 | blau-weiß |
| 6 | – | ↔ J3/6 | ↔ J1/6 | grün |
| 7 | – | ↔ J3/7 | ↔ J1/7 | braun-weiß |
| 8 | `SCAN` | Scan-Bus → R5 (Pull-down) → U2/7 → GPIO13 | durchverbunden | braun |

Pin 5–7 sind reine Durchleitungen zwischen Haube und Frontpanel; die Platine
greift sie nicht ab. **Weder 5V noch GND liegen auf diesem Stecker**
(siehe "Stromversorgung und Masse").

> **Offener Punkt:** D1/D2 (1N4148, Anode an J3, Kathode am jeweiligen
> LED-Netz) stammen aus der Lochraster-Version. Ob sie neben U3 noch gebraucht
> werden, ist nicht abschließend geklärt – siehe Fallstrick 2.

## Bekannte Fallstricke aus der Lochraster-Version (im Redesign vermieden)

1. **GPIO0 ohne Pull-up** war die Hauptursache für sporadische Bootfehler.
   → R2 eingeplant, wirkt über den Jumper Board-J5 in Stellung 1–2.
2. **D1/D2-Dioden als improvisierter Rückwärts-Levelshifter** für Haube-J5/J6 –
   funktioniert, legt aber die volle 5V-Busspannung nur diodengeschützt an
   den ESP-GPIO. → Die *Tastenemulation* übernimmt jetzt U3 (74HCT08).
   D1/D2 sind aber weiterhin in der Schematic, jetzt an den **LED-Leseleitungen**
   Richtung Frontpanel. Zu prüfen, ob das so gewollt ist.
3. **Reset-Taster war ursprünglich an GPIO16 statt RST** verdrahtet –
   im Redesign SW1 direkt an RST/EXT_RST.
4. **Kein definierter Power-on-Reset** – Wandler-Anlaufverhalten konnte zu
   instabilem Boot führen. → optional U5 (Supervisor-IC) vorgesehen. Der
   MCP130 hat einen Open-Drain-Ausgang und verträgt sich deshalb mit R6 und
   C7; seine Schwelle von 3,15 V liegt allerdings dicht an 3,3 V.
   Erst die Pufferung richtig auslegen, dann über U5 entscheiden.
5. **Bulk-Elko zu klein/unklar dimensioniert und auf der falschen Seite**
   (Originalwert im Artikel: 800 µF, gemessen später 3,88 mF) → im Redesign
   aufgeteilt: C8 (1000 µF) auf `5V_IN`, C1 (470 µF) auf `VCC`.
   Siehe "Dimensionierung der Pufferung".
6. **RST-Auto-Reset-Kondensator:** jetzt C7, 100 nF Keramik (unpolarisiert).
   Der frühere 4,7-µF-Elko (C2) war doppelt falsch – zu große Zeitkonstante
   für den 100-ms-Reset-Impuls von esptool, und ein Koppelkondensator wird
   beidseitig belastet, taugt also nicht polarisiert. Zusammen mit R6 (10 kΩ)
   ergibt sich τ = 1 ms.

## Mechanik: Gehäuse und Platinenabmessungen

Vorgaben aus dem Gehäuse (Stand 2026-09-07):

| Größe | Wert |
|---|---|
| Maximale Platinenabmessung | **55 × 80 mm** |
| Befestigungsbohrungen | 4 Stück, Rastermaß **50 × 50 mm** (vertikal und horizontal gleich) |
| Sockel-Innendurchmesser (Gewinde) | 1,8 mm |
| Sockel-Außendurchmesser | 5,0 mm |
| Erste Bohrung von der Oberkante | 3,5 mm ab Bohrungsmitte (= 1 mm Rand vom Sockelrand zur Platinenkante) |

### Orientierung auf der Platine

Die Outline in KiCad liegt **quer**: `(0,0) → (80,0) → (80,55) → (0,55)`, die
80-mm-Kante läuft also entlang x. Das ist dieselbe Platine wie „55 × 80 hoch",
nur um 90° gedreht. Damit ergeben sich die Bohrungsmitten:

```
   x = 3,5 und 53,5 mm      (Langseite 80 mm, 1 mm Rand zum Sockel)
   y = 2,5 und 52,5 mm      (Schmalseite 55 mm, Sockel bündig zur Kante)
```

**Der 1-mm-Rand gilt nur an der Langseite** (bestätigt 2026-09-07). An der
Schmalseite liegt der Sockel bündig auf der Platinenkante:

| Richtung | Bohrungsmitte | Sockel (⌀5) | Rand zur Kante |
|---|---|---|---|
| Langseite (80 mm, x) | 3,5 / 53,5 | 1,0…6,0 bzw. 51,0…56,0 | **1 mm** |
| Schmalseite (55 mm, y) | 2,5 / 52,5 | 0…5 bzw. 50…55 | **0 mm, bündig** |

Das ist so gewollt und kein Fehler.

### Bauhöhe (Vorgabe 2026-09-07)

Die Innenhöhe des Gehäuses beträgt **max. 20 mm inklusive Platine und
Montagesockeln**. Als Praxiswert gilt: **14 mm Bauteilhöhe über der Platine
sind sicher, darüber wird es unsicher.**

| Bauteil | Höhe über PCB | |
|---|---|---|
| C8 1000 µF/10 V, D10 stehend | 12,5 mm | ok |
| U4, TPS63802-Modul flach aufgelötet | 3,5 mm | ok |
| Gegenstecker (Dupont-Buchse) auf vertikaler Stiftleiste | ~14,7 mm | **grenzwertig** |
| C1 470 µF/10 V, D8 | ~11,5 mm | ok |
| U2/U3, DIP im Sockel | ~9 mm | ok |
| Stiftleiste 2,54 vertikal, ohne Gegenstecker | 8,5 mm | ok |
| Jumper-Shunt auf J5 | ~7,5 mm | ok |
| Scheibenkondensator 100 nF | 5–7 mm | ok |
| SW1/SW2 (6-mm-Taster) | ~5 mm | ok |
| U1 (ESP-12E) | ~3 mm | ok |
| Widerstände, Dioden liegend | 2,5 mm | ok |

Damit ist die Höhe ein **Auswahlkriterium für U4**: das Wandlermodul darf
inklusive Stiftleiste nicht über 14 mm bauen. Zusammen mit der schon
bekannten Anforderung (min. Vin ≤ 4,5 V, ≥ 500 mA, dreipolig im 2,54-Raster)
scheidet die gesamte OKI-78SR- und R-78E-Familie aus – deren minimale
Eingangsspannung liegt bei 4,75 V bzw. 7 V.

### Schrauben und Bohrungsdurchmesser (entschieden 2026-09-07)

| Größe | Wert |
|---|---|
| Schraube | **M2** |
| Kopfdurchmesser | **4,0 mm** |
| Platinenbohrung | **2,2 mm** |
| Footprint | `MountingHole:MountingHole_2.2mm_M2` |

Der Footprint ist ein reines `np_thru_hole` – Bohrung 2,2 mm, **kein Kupfer**,
kein Netz. Er bringt zwei Kreise mit, die den Freiraum dokumentieren:
⌀4,40 mm auf `Cmts.User` (Schraubenkopf) und ⌀4,90 mm als `F.CrtYd`
(Courtyard). Eine zusätzliche Sperrfläche ist deshalb nicht nötig – der
Courtyard hält Bauteile automatisch weg und wird von der DRC geprüft.

Der ⌀5-Sockel ist ein *bestückungsseitig irrelevantes* Maß: er sitzt unter der
Platine, und dort liegen bei dieser reinen THT-Bestückung nur Lötaugen. Auf der
Oberseite begrenzt der Schraubenkopf mit ⌀4 mm, und den deckt der ⌀4,9-Courtyard
mit 0,45 mm Reserve ab.

An der Schmalseite reicht die Bohrung von y = 1,4 bis 3,6 mm; es bleibt ein
1,4 mm breiter Steg zur Platinenkante stehen. Mit den früher eingesetzten
3,2 mm (M3) wären es nur 0,9 mm gewesen.

## Platzierung (Stand 2026-09-07, 3. Durchgang)

Das alte Board (90 × 70 mm, Netznamen einer weit zurückliegenden Revision) war
nicht zu retten und wurde vollständig neu aufgebaut. **Leiterbahnen und
Kupferflächen gibt es noch nicht.**

Der dritte Durchgang war nötig, weil C8 durch die Wahl des TPS63802 von
3300 µF liegend auf 1000 µF stehend geschrumpft ist und U4 dafür von 82 auf
325 mm² gewachsen ist.

### Zwei Entscheidungen, die die Platzierung bestimmen

* **J1, J3 und J6 werden nicht gesteckt, sondern direkt verlötet.** Eine
  aufgesteckte Dupont-Buchse baut allein ~14,7 mm, mit Kabelbogen darüber real
  18–20 mm; das passt nicht unter den Deckel (Limit 14 mm). Die Pads der
  vertikalen Stiftleisten bleiben im Layout stehen, die Leisten werden nur
  nicht bestückt – die Adern gehen direkt in die Bohrungen. J2 (FTDI) und J4
  (Reset) bleiben steckbar, die werden ohnehin nur bei offenem Gehäuse benutzt.
* **U4 belegt 25,8 × 13,0 mm.** Seit 2026-09-11 steht dort der echte
  Footprint (hochkant, 270°: **VIN oben, VOUT unten**), der Platzhalter auf
  `Cmts.User` ist entfallen – siehe Abschnitt U4.

### Zonen

| Bereich | Inhalt |
|---|---|
| Oben Mitte, x 18…36 | **U1** (ESP-12E), Antenne an der Oberkante |
| Links oben | SW1, J4, R6, R4, C3 — Reset-Gruppe |
| Links unten | SW2, J5, R2, C8 |
| Mitte, x 36…44 | J2 (FTDI), C7, C4, C6 |
| Mitte rechts, x 44…60 | **U2** (4050) oben, **U3** (74HCT08) unten |
| Rechts oben | **J1**, **J3**, D1, D2, C5, R5 |
| Unten Mitte | **U4** (Wandlermodul), J6, C1 |

Abstände der Abblockkondensatoren zu ihrem Versorgungspin: C3 → U1/8 = 3,4 mm,
C4 → U2/1 = 3,5 mm, C5 → U3/14 = 3,4 mm.

Mit dem echten Footprint (2026-09-11) sitzen die Anschlüsse fest: VIN auf
(60,10 | 62,64 / 43,22), VOUT auf (60,10 | 62,64 / 66,08), GND je zweimal auf
x 52,48 / 55,02 in beiden Reihen. **C8 steht damit 7,7 mm über dem
Wandlereingang** – für den 1000-µF-Puffer gut genug, zumal das Modul einen
eigenen 100-µF-Eingangskondensator mitbringt und C8 reiner Energiespeicher für
die Millisekundenskala ist.

Offen bleibt die **Ausgangsseite**: C6 (100 nF) liegt 12,6 mm, C1 (470 µF)
gut 30 mm von den VOUT-Pads entfernt – beide sitzen noch dort, wo der
Platzhalter seinen Ausgang hatte.
Das ist der angekündigte Nachzug der U4-Umgebung – er steht noch aus und
gehört vor das Routen.

Damit der 13,0 mm breite Körper überhaupt zwischen C6 und TP1 passt, sind am
2026-09-11 **C6 um 1,2 mm und TP2/TP4 um je 1,0 mm nach links** gerückt; C8s
Referenztext musste aus dem Modulumriss weichen. Danach: Courtyards frei,
Silk frei.

### Wie platziert wurde

Verankert von Hand: U1 (Antenne an der Kante), U2/U3, J1/J3 (Pin an Pin
gegenüber, damit die sechs Durchleitungen gerade Bahnen werden) und U4. Der
Rest per Skript, mit Kosten = Summe der **Pad-zu-Pad-Abstände** je Netz;
GND, VCC und 5V_IN sind ausgenommen, weil globale Netze kein Ortssignal geben.
Abblock- und Pufferkondensatoren hängen stattdessen mit hohem Gewicht an ihrem
Versorgungspin. Danach Verbesserungsdurchläufe bis zur Konvergenz.

### Prüfstand

```
kicad-cli sch erc  --severity-all            → 0 Verstöße
kicad-cli pcb drc  --severity-error --severity-warning
  → 0 Violations                    (Stand 2026-09-11, mit U4-Footprint;
                                     4 Kabelmontage-Keepouts sind Ausnahmen)
  → schematic_parity: 9 Hinweise    (TP4-Value 3V3, und 8 × „extra footprint"
                                     für die Bohrungen MH1–MH4 / H1–H4 ohne
                                     Symbol – beides so gewollt)
  → 83 unconnected items   (= die noch fehlenden Leiterbahnen, so gewollt)
```

### Positionen

```
C1 (26.5,34.5)  90   C3 (16.0,23.5)  90   C4 (40.5, 7.0)  90
C5 (61.0,30.0)   0   C6 (39.5,36.0)  90   C7 (43.5, 3.0)   0
C8 (16.0,42.0) 180   D1 (61.0, 2.5)   0   D2 (58.5, 6.5) 270
J1 (62.0, 6.0)   0   J2 (38.5,10.5)   0   J3 (68.0, 6.0)   0
J4 (12.0, 5.5) 180   J5 ( 4.5,32.5)  90   J6 (33.0,35.0)  90
R2 ( 4.5,46.0)  90   R3 (20.5,28.5) 180   R4 (16.0,13.5)  90
R5 (69.0,27.5) 270   R6 (13.0, 9.5) 180   SW1 (6.0,13.0)   0
SW2( 7.0,22.0) 270   U1 (27.0,13.0)   0   U2 (44.0, 7.0)   0
U3 (50.0,30.0)   0   U4 (33.0,42.0)   0
MH1(3.5,2.5) MH2(53.5,2.5) MH3(3.5,52.5) MH4(53.5,52.5)
```

## Stand / nächste Schritte in KiCad

* Schematic ist vollständig verdrahtet, **ERC: 0 Fehler, 0 Warnungen**
  (auch mit `--severity-all`).
* **Jedes Bauteil hat einen Footprint.** Am 2026-09-06 korrigiert:
  R5 (hatte `Package_DIP:DIP-8_W7.62mm` – ein DIP-8-Gehäuse für einen
  zweipoligen Widerstand), J4/J5/J6 (hatten gar keinen), U4 (hatte den
  Murata-Landeplatz), sowie die Umverteilung C1/C8.
* Die ERC-Regel `footprint_filter` steht in den Projekteinstellungen auf
  `ignore`. Sie hätte den R5-Fehler gemeldet – Kandidat zum Wiedereinschalten.
* **Warnung zu den KiCad-MCP-Tools:** `move_schematic_component`,
  `delete_schematic_wire`, `add_schematic_component` und `add_schematic_wire`
  löschen beim Neuschreiben der Datei Junctions – auch an Stellen, die mit der
  Operation nichts zu tun haben. Zweimal reproduziert (30 → 23 bzw. 30 → 18
  Junctions), Folge waren jeweils ~14 ERC-Fehler durch stillschweigend
  getrennte Netze. Änderungen deshalb entweder in der GUI machen oder per
  Skript direkt an der Datei, und danach **immer** Junctions zählen und die
  Netzliste gegen den Vorstand diffen.
* **Board am 2026-09-07 neu aufgebaut** (siehe „Platzierung"): Outline
  80 × 55 mm, alle 26 Bauteile platziert, MH1–MH4 als
  `MountingHole_2.2mm_M2`. DRC 0 Violations, `schematic_parity` 0.
* Die tote Clearance-Ausnahme für U5 wurde aus `hood-control.kicad_dru`
  entfernt – U5 gibt es in der Schaltung nicht mehr.
* **U4-Modul ist da, vermessen und gezeichnet** (2026-09-11): Footprint in
  `hood-control.pretty`, im Board platziert, DRC 0 Violations.
* **Nächster Schritt: die Umgebung von U4 nachziehen** – vor allem C1 und C6
  auf die VOUT-Seite holen – und erst danach routen. Die Liste unter
  „Positionen" ist damit überholt und wird danach neu erzeugt.
* Offen: Zweck von D1/D2 neben U3 (siehe Fallstrick 2), sowie die Bestellung
  bei Reichelt (Liste steht, Artikelnummern in der BOM).
