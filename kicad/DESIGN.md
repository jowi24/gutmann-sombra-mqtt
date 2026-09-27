🇬🇧 **English** · 🇩🇪 [Deutsch](DESIGN.de.md)

# Hood-Control PCB – Redesign

This document describes the schematic in `hood-control.kicad_pro` /
`hood-control.kicad_sch`. All values are derived from debugging the
perfboard version (see [CHANGELOG.md](../CHANGELOG.md) / debugging session
of 2026-09-03) and from the 2017 blog post
("Do-It-Yourself IoT Modul für die Dunstabzugshaube").

For a beginner-friendly explanation of *how* the circuit works, see
[`docs/CIRCUIT.md`](../docs/CIRCUIT.md).

**Construction: THT (through-hole).** All resistors/capacitors are leaded,
all ICs sit in DIP sockets, connectors are 2.54 mm pin headers, buttons are
6 mm THT tactile switches. The only SMD part is the ESP-12E module itself.
The voltage converter is **not a discrete switching regulator** but a
ready-made TPS63802 buck-boost module (U4, 8 holes), soldered flat or
plugged onto pin headers.

Schematic sheet size: **A5** (210 × 148 mm).

> **Mind the name clash:** the hood electronics names its own matrix lines
> J1–J8 as well (J1–J4 = scan inputs, J5–J6 = key outputs, J7–J8 = LED
> outputs). The connectors *of this board* are also called J1–J6. Where
> there is a risk of confusion, "hood-J5" or "board-J5" is spelled out.

## Bill of materials (BOM)

| Ref | Part | Value/type | Footprint | Purpose |
|---|---|---|---|---|
| U1 | ESP-12E/F | ESP8266 module (SMD) | `RF_Module:ESP-12E` | microcontroller |
| U2 | CD4050BE / HEF4050BP / 74HC4050N | hex buffer, **16-pin DIP** | `Package_DIP:DIP-16_W7.62mm_Socket` | level shifting 5 V→3.3 V (reading: scan bus, hood-J7/J8) |
| U3 | **74HCT08** (e.g. SN74HCT08N) | quad AND, 14-pin DIP | `Package_DIP:DIP-14_W7.62mm_Socket` | level shifting 3.3 V→5 V (driving: hood-J5/J6) |
| U4 | **TPS63802 buck-boost module** (eBay, PCB-Tronic24) | 1.5–5.5 V → 3.3 V, 2.7 A | `hood-control:TPS63802_Module_25.8x13.0mm` (project library) | power supply (replaces the 2017 mini converter) |
| R2 | 10 kΩ | 0207 leaded | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal` | pull-up GPIO0 → 3.3 V, via board-J5 pin 1 |
| R3 | 10 kΩ | 0207 leaded | same | pull-down GPIO15 → GND |
| R4 | 10 kΩ | 0207 leaded | same | pull-up CH_PD/EN → 3.3 V |
| R5 | 2.4 kΩ | 0207 leaded | same | pull-down on the scan bus (empirical, see 2017 blog post) |
| R6 | 10 kΩ | 0207 leaded | same | pull-up RST → 3.3 V |
| R7, R8 | 10 kΩ | 0207 leaded, upright | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical` | pull-down of U3 inputs (GPIO12/14), since rev. 0.3 |
| C1 | 470 µF / 10 V | radial electrolytic, D8/P3.5 | `Capacitor_THT:CP_Radial_D8.0mm_P3.50mm` | support cap on `VCC` near the ESP – bridges the response time of the buck-boost module |
| C8 | 1000 µF / 10 V | radial electrolytic, D10/P5, 12.5 mm tall (Reichelt `RD1A108M1012M128`) | `Capacitor_THT:CP_Radial_D10.0mm_P5.00mm` | bulk capacitance on `5V_IN` – buffers the current-limited hood supply |
| C3–C6 | 100 nF | ceramic disc, P5 | `Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm` | decoupling: C3=U1 (VCC), C4=U2 (VCC 3.3 V), C5=U3 (5V_IN), C6=U4 output |
| C7 | 100 nF | ceramic disc, P5 | `Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm` | RST coupling FTDI-DTR → RST (auto-reset) |
| D1, D2 | 1N4148 | DO-35 | `Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal` | decoupling of the LED read lines towards the front panel |
| D3, D4 | 1N4148 | DO-35, upright | `Diode_THT:D_DO-35_SOD27_P2.54mm_Vertical_AnodeUp` | U3 output diodes → key lines, since rev. 0.3 |
| SW1 | 6 mm THT button | – | `Button_Switch_THT:SW_PUSH_6mm` | reset (RST → GND) |
| SW2 | 6 mm THT button | – | `Button_Switch_THT:SW_PUSH_6mm` | flash mode (GPIO0 → GND) |
| J1 | pin header 1×8, 2.54 mm | "Hoodcontrol" | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | connection to hood electronics |
| J2 | pin header 1×6, 2.54 mm | FTDI header | `Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical` | DTR, RXI, TXO, VCC, n.c., GND |
| J3 | pin header 1×8, 2.54 mm | "Frontpanel" | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | connection to front panel |
| J4 | pin header 1×2, 2.54 mm | "Reset" | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | external reset button (RST → GND) |
| J5 | pin header 1×3, 2.54 mm | "Flash/Run" | `Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical` | jumper: 1–2 = run, 2–3 = flash |
| J6 | pin header 1×2, 2.54 mm | "PWR_IN" | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | supply + ground from the hood's service connector |

> **Beware of old notes:** in the changelog and the blog post, "the big
> electrolytic" always means C1. Since the buffering was redistributed,
> **C8** is the large bulk capacitor (since 2026-09-07 **1000 µF** on
> `5V_IN`) and **C1** the smaller support capacitor (470 µF on `VCC`).
>
> The numbering has gaps (no C2, no R1). C2 was the old 4.7 µF electrolytic
> for RST coupling and was replaced by C7; the 2.5 kΩ pull-down is called R5
> in the schematic, not R1. A re-annotation would only create new mismatches
> between documentation and schematic and is deliberately not done.

### U4 – voltage converter (decided 2026-09-07)

A **TPS63802 buck-boost module** is fitted (eBay, seller PCB-Tronic24,
€4.79).

| | |
|---|---|
| IC | TPS63802 (Texas Instruments) |
| Input | **1.5 – 5.5 V** (start-up from 1.8 V) |
| Output | 3.3 / 4.2 / 5 V via solder bridge, **2.7 A** at 5 V → 3.3 V |
| Dimensions | **25.8 × 13.0** × 3.5 mm (measured 2026-09-11) |
| Connections | four corner contacts with 2 holes each on 2.54 mm pitch: VIN top left, VOUT top right, GND bottom left and right |
| On-board parts | 100 µF/16 V input capacitor and power LED already on the module |

**Why buck-boost and not buck.** The hood supplies 4.9 V at no load. Every
common buck needs at least 4.5 – 6 V and drops out of regulation below that
– that is the brownout boot loop of the perfboard version. A buck-boost from
1.5 V doesn't have this edge: if the input sags below 3.3 V, it simply keeps
regulating.

**What was checked at Reichelt and rejected** (as of 2026-09-07):

| Part no. | Vin min | Price | |
|---|---|---|---|
| `TSRN 1-2433` | 4.6 V | €14.10 | 0.3 V margin, three times the price |
| `TSR 1-2433` | 4.75 V | €6.85 | too tight |
| `R-78E33-05` | 6 V | €3.55 | too high |
| `TSR 1-2433E` | 6 V | €3.99 | too high, available only from 2026-11-02 |

The whole OKI-78SR family is ruled out as well (3.3 V variant 7–36 V).

> **Caution when fitting:** the module ships **set to 4.2 V**. The ESP8266
> tolerates an absolute maximum of 3.6 V. So first move the solder bridge to
> **3V3**, then connect the module alone to 4.9 V, measure the output – and
> only then fit it.

#### Footprint (drawn 2026-09-11)

The module was at hand and measured with calipers. The photo-based estimate
was right across (holes at ±2.54 and ±5.08 mm from the centre line) but off
for the row spacing: it is **22.86 mm = 9 × 2.54**, not ~23.5 mm. So the
whole hole pattern sits on the 2.54 mm grid.

| | |
|---|---|
| File | `kicad/hood-control.pretty/TPS63802_Module_25.8x13.0mm.kicad_mod` |
| Library | `hood-control` (`fp-lib-table` in the project, `${KIPRJMOD}`) |
| Body | 25.8 × 13.0 mm on `F.SilkS`/`F.Fab`, courtyard +0.25 mm |
| Hole pattern | x = ±11.43 mm, y = ±2.54 and ±5.08 mm → 8 holes |
| Pads | 1.7 mm round, 1.0 mm drill; pad 1 rectangular as pin-1 marker |
| Pin numbers | 1 = VIN, 2 = GND (4 ×), 3 = VOUT – matching the pins of symbol `Converter_DCDC:OKI-78SR-12_1.0-W36-C` |
| 3D model | `kicad/3dmodels/TPS63802_Module_25.8x13.0mm.step` (generated by `create_tps63802_module.py` in FreeCAD) |

Each of the four contacts has two holes that are connected on the module
itself. In the footprint they therefore share the same pad number, and the
option `duplicate_pad_numbers_are_jumpers` is set to `yes`: KiCad then knows
the connection is inside the part and doesn't require it on the board.
Still, **both** holes of a contact should be connected when routing – at
2.7 A that is no luxury.

The originally planned extra 2.54 mm row VIN/GND/VOUT next to the module was
dropped: the module lies flat on the board and covers its area completely,
and between C6 and TP1 there is not even a millimetre left next to the body.
TP3 (5V) and TP4 (3V3) serve as test points.

The silkscreen inside the outline reads **"Loetbruecke auf 3V3!"** ("solder
bridge to 3V3!") – the warning above as a print, visible while the module is
not yet fitted.

### Note on U3 (74HCT08)

The installed KiCad libraries have no dedicated 74HCT08 symbol. The symbol
`74xx:74LS08` (**identical pinout**) is used instead, but the value field is
set to `74HCT08` so BOM and silkscreen are correct. **An HCT type must be
fitted** – only the HCT input threshold (V_IH ≈ 2.0 V) can be driven safely
from 3.3 V.

## Sourcing

### Suppliers

| What | Where |
|---|---|
| U4, TPS63802 buck-boost module | eBay, seller **PCB-Tronic24** (Cuxhaven), item no. 376238468668 – <https://www.ebay.de/itm/376238468668> – €4.79 (€4.31 from 4 pcs) |
| all other parts | reichelt.de, part numbers below |

The TPS63802 is also available on AliExpress (diymore, 10.4 × 7.9 mm,
€2.59), but ~€3 import fee per item is added there and delivery takes ~10
days. The eBay offer is therefore cheaper, faster and returnable.

### Reichelt part numbers

| Ref | Qty | Part no. | Description |
|---|---|---|---|
| C3–C7 | 5 | `X7R-5 100N` | multilayer ceramic 100 nF, 50/100 V, X7R ±10 %, P5.0 |
| C1 | 1 | `WL1C477M0811M` | electrolytic 470 µF/16 V, 105 °C, low ESR, P3.5, 8 × 11.5 mm |
| C8 | 1 | `RD1A108M1012M128` | electrolytic 1000 µF/**10 V**, 105 °C, P5, D10 × **12.5 mm** |
| R2, R3, R4, R6 | 4 | `METALL 10,0K` | 10.0 kΩ, 0207, 0.6 W, 1 % |
| R5 | 1 | `METALL 2,40K` | 2.40 kΩ, 0207, 0.6 W, 1 % |
| D1, D2 | 2 | `1N 4148` | switching diode 100 V / 150 mA, DO-35 |
| SW1, SW2 | 2 | `DIP DTS-61K` | tactile switch 6 × 6 × 4.3 mm, PCB |
| U2 | 1 | `CD 4050BE TEX` | CMOS hex non-inverting buffer, 3–18 V, DIP-16 |
| U3 | 1 | `74HCT 08` | AND gate, quad, 4.5–5.5 V, DIP-14 |
| U2 (socket) | 1 | `GS 16` | IC socket 16-pin, 7.62 mm |
| U3 (socket) | 1 | `GS 14` | IC socket 14-pin, 7.62 mm |
| J2, J4, J5 | 1 | `ECON SL40G1` | pin header 1 × 40, straight, 2.54 mm (11 pins needed) |
| J5 | 1 | `JUMPER 2,54 SW` | jumper, 2.54 mm pitch |
| tool | 1 | `ANMAS MG81000N` | head-band magnifier with LED, 5 lenses |

Since rev. 0.3, D3/D4 (1N4148) and R7/R8 (10 kΩ) are needed in addition –
the spare parts of the order above cover them.

**J1, J3 and J6 are deliberately not in the list** – they are wired
directly, which only needs stranded wire (see "Placement").

A few notes on the selection so they don't get lost again:

* **`X7R-5 100N`, not `KERKO 100N`.** The latter is a cent cheaper but Y5V
  with −20/+80 % – the capacitance collapses with temperature and voltage.
* **C8 rated 10 V, not 16 V.** All 1000 µF types rated 16 V at Reichelt are
  at least 16 mm tall and break the 14 mm limit. The 10 V variant comes in
  D10 × 12.5 mm, and 10 V on a 4.9 V rail is double margin.
* **R5 at 2.40 kΩ.** Until 2026-09-07 the schematic said 2.5 kΩ – not a
  standard value, the E24 series has 2.4 k and 2.7 k. For a pull-down on the
  scan bus the value is uncritical (it comes empirically from the 2017 blog
  post). Schematic and board are updated to **2.4 kΩ**.
* **Reichelt doesn't carry M2 screws** – the search only returns M.2 SSD
  accessories.

### Ordered on 2026-09-07

| Part no. | Qty | Note |
|---|---|---|
| `X7R-5 100N` | 10 | 5 needed |
| `WL1C477M0811M` | 2 | 1 needed |
| `RD1A108M1012M128` | 4 | 1 needed |
| `METALL 10,0K` | 10 | 4 needed |
| `METALL 2,40K` | 10 | 1 needed |
| `1N 4148` | 5 | 2 needed |
| `DIP DTS-61K` | 2 | 2 needed, **no spare** |
| `GS 16` | 2 | 1 needed |
| `GS 14` | 2 | 1 needed |
| `CD 4050BE TEX` | 1 | spare, U2 on hand |
| `74HCT 08` | 1 | spare, U3 on hand |
| `ANMAS MG81000N` | 1 | tool |
| TPS63802 module (eBay) | 1 | |

Not ordered because **on hand**: 2.54 mm pin header (for J2, J4, J5),
jumper for J5 and the four M2 screws. The order is thus complete – nothing
is missing.

## GPIO assignment (from src/main.cpp)

| GPIO | Function | Via which level shifter |
|---|---|---|
| GPIO13 | hood scan input (interrupt, RISING) | U2 (4050), gate C (pin 7 → 6) |
| GPIO5  | read LED state (J1/1), firmware `ledPin2` | U2 (4050), gate B (pin 5 → 4) |
| GPIO4  | read LED state (J1/2), firmware `ledPin1` | U2 (4050), gate A (pin 3 → 2) |
| GPIO12 | key emulation (J1/3), firmware `buttonPin2` | U3 (HCT08), gate A (pins 1+2 → 3) → D3 |
| GPIO14 | key emulation (J1/4), firmware `buttonPin1` | U3 (HCT08), gate B (pins 4+5 → 6) → D4 |
| GPIO2  | on-board status LED (active LOW) | – (internal to the ESP-12 module, no-connect) |
| GPIO0  | flash button / Wi-Fi config reset at boot | – (local, SW2 + R2 via board-J5) |
| CH_PD  | enable, must be HIGH at boot | – (local, R4) |
| GPIO15 | boot strapping, must be LOW at boot | – (local, R3) |
| GPIO16 | unused (no deep-sleep wake-up planned) | – |
| GPIO9/GPIO10 | **do not use** – reserved on the ESP-12 for flash access (QIO) | – |

> **Channel mapping (bring-up 2026-09-24):** both the two LED channels and
> the two key channels arrive swapped compared to the perfboard firmware –
> fan 1 was detected as "timer", "light on" switched fan level 2. Corrected
> in the firmware by swapping `ledPin1/2` and `buttonPin1/2`; the hardware
> is unchanged.

## Power and ground

Neither comes through the 8-wire control cable, but through the separate
service connector of the hood electronics (marked 3 = ground, 4 = supply in
the blog post), tapped with a shortened ISA bus slot:

* **about 4.8 V, able to supply about 500 mA** (determined experimentally
  with a load resistor)
* The RJ45 cable between hood and front panel carries **no reference
  ground**. The front panel alone doesn't need it, measuring and actively
  driving do. Using the enclosure as ground reference does not work.

It follows for J1/J3: **neither 5V_IN nor GND is on these connectors** –
that is correct and not an oversight.

### Supply nets

* `5V_IN` → J6/1, U4/VIN, U3/VCC (74HCT08 runs on 5 V), C5, C8
* `VCC` (3.3 V) → U4/VOUT, U1, U2/VDD, R2, R4, R6, J2/4, C1, C3, C4, C6
* `GND` → J6/2, common ground
* Each net carries a `PWR_FLAG` (`#FLG01` on GND, `#FLG02` on 5V_IN); VCC is
  driven by the `power_out` pin of the converter module.

### Sizing the buffering

When transmitting over Wi-Fi, the ESP briefly draws ~350 mA from 3.3 V,
which means ~270 mA from 4.9 V through the converter. The source delivers
500 mA – **so current was never the bottleneck.** The bottleneck was the
*lower limit*: if the input voltage sags below the minimum input voltage of
a buck module (4.5 – 6 V depending on type), it drops out of regulation.
That is the brownout boot loop of the perfboard version.

**With the TPS63802 (1.5 – 5.5 V) this edge no longer exists**, so the
buffering was allowed to shrink from 3300 µF to **1000 µF** on 2026-09-07.
The capacitor now only bridges the response time of the hood supply, no
longer a dropout risk. Side effect: C8 stands upright again (12.5 mm instead
of 35 mm lying down) and the 13.5 × 39 mm strip in the layout is free.

The buffering is therefore split across both sides (decided 2026-09-06):

| Net | Capacitors |
|---|---|
| `5V_IN` | C5 (100 nF) + **C8, 1000 µF/10 V** – buffers the 500 mA source |
| `VCC` | C3/C4/C6 (100 nF) + **C1, 470 µF/10 V** – bridges the ~100 µs response time |

The large capacitance deliberately does **not** go on the output: 330–470 µF
is enough there, and very large values can trip the converter's current
limit at power-up.

## Reset and flash circuitry

| Net | Parts |
|---|---|
| `/RST` | U1/1, R6 (pull-up), C7 (coupling from FTDI-DTR), SW1, J4 |
| `Net-(U1-EN)` | U1/3, R4 (own pull-up) |
| `/GPIO0` | U1/18, SW2, board-J5 pin 2 |
| `/FTDI_DTR` | J2/1, C7 |

**EN and RST deliberately have separate pull-ups.** A shared resistor would
merge both pins into one net: SW1 and J4 would then switch the chip off
instead of resetting it, and a later RC network on EN would render the
auto-reset useless through the capacitive divider with C7.

### Auto-reset via J2

On connecting, esptool runs the sequence `DTR=0/RTS=1 → 100 ms → DTR=1/RTS=0`.
J2 only brings out **DTR** (pin 5 is CTS on the adapter used, an input of
the adapter, and can't drive anything). Therefore:

* **Auto-reset works** – the DTR edge is coupled to RST via C7.
* **Auto-flash does not work** – GPIO0 is not remote-controlled.
* After flashing, the ESP stays in the bootloader because esptool's
  `hard_reset` only toggles RTS. Press SW1 (or hope for the DTR edge when
  the port closes).

Full auto-flash would require an adapter with RTS **and** the cross-coupled
NPN pair of the NodeMCU circuit. Deliberately not implemented: serial
flashing is only used for the first flash and for recovery; day to day OTA
is used (`[env:nodemcuv2-ota]`).

### Board-J5 jumper (flash/run)

| Position | Effect |
|---|---|
| **1–2 (normal operation)** | R2 pulls GPIO0 to 3.3 V – the ESP boots the firmware |
| 2–3 | GPIO0 tied to GND – the ESP boots into flash mode |

In normal operation the jumper is on **1–2**. The pull-up R2 therefore only
acts when the jumper is fitted; without the jumper GPIO0 floats and boot is
unreliable. That is a deliberate choice (one part instead of two, flash mode
without button acrobatics) – the jumper must always be fitted.

## Unused gates (important!)

CMOS and HCT inputs must **not float** (oscillation, shoot-through current,
noise). Therefore:

* U2 (4050): the three unused buffers D/E/F – **inputs (pins 9, 11, 14) tied
  to GND**, outputs (pins 10, 12, 15) open with no-connect flag.
* U3 (74HCT08): the two unused gates C/D – **both inputs of each gate
  (pins 9+10 and 12+13) tied to GND**, outputs (pins 8, 11) open with
  no-connect.

## J1 / J3 pinout (8 pins each)

The original used an old network cable. Wire colours per **T568B**. The
board taps into the existing hood ↔ front panel connection.

| Pin | Net | J1 (hood) | J3 (front panel) | T568B |
|---|---|---|---|---|
| 1 | `LED_CH_2` | LED state hood-J7 → U2/5 → GPIO5 | via D1 (anode) onto the same net | orange-white |
| 2 | `LED_CH_1` | LED state hood-J8 → U2/3 → GPIO4 | via D2 (anode) onto the same net | orange |
| 3 | `BTN_CH_1` | key emulation, driven by U3/3 via D3 | connected through | green-white |
| 4 | `BTN_CH_2` | key emulation, driven by U3/6 via D4 | connected through | blue |
| 5 | – | ↔ J3/5 | ↔ J1/5 | blue-white |
| 6 | – | ↔ J3/6 | ↔ J1/6 | green |
| 7 | – | ↔ J3/7 | ↔ J1/7 | brown-white |
| 8 | `SCAN` | scan bus → R5 (pull-down) → U2/7 → GPIO13 | connected through | brown |

Pins 5–7 are pure pass-throughs between hood and front panel; the board does
not tap them. **Neither 5V nor GND is on this connector** (see "Power and
ground").

D1/D2 (1N4148, anode at J3, cathode at the respective LED net) already sat
exactly like this on the LED lines of the perfboard version (confirmed from
the Fritzing file `fritzing/abzugshaube.fzz`, 2026-09-24). They have nothing
to do with the key emulation.

### Key emulation: U3 only with output diodes (D3/D4) and pull-downs (R7/R8)

The key lines are part of the key matrix between hood and front panel. The
board may only **pull them up** (press a key) or **leave them completely
alone** – never hold them LOW.

* **U3 directly on J1/3+4 (rev. 0.2) does not work:** the 74HCT08 has
  push-pull outputs and always drives the line HIGH or LOW; with floating
  inputs (ESP boot) randomly. Result: ESP boot loop, phantom keys on the
  hood (fan level 3 + filter = both channels in scan row 2), hood stops
  responding.
* **A direct wire or series resistor (1–2.4 kΩ) instead of U3 doesn't work
  either:** the front panel glows faintly, the ESP hangs on hood power, with
  a resistor the hood goes into an error code at start-up. While the 3.3 V
  rail is still at 0 V at power-up, the ESP protection diodes clamp the
  lines; conversely the 5 V pulses feed back into VCC. (The perfboard had
  GPIO12/14 wired directly and ran that way for years – outside the
  specification.)
* **Solution (rev. 0.3):** D3/D4 (1N4148) in series after U3 pins 3 and 6,
  anode at U3, cathode at BTN_CH. HIGH ≈ 4.3 V presses, LOW blocks the
  diode. R7/R8 (10 kΩ) pull the U3 inputs to GND until the firmware runs.
  The firmware always drives GPIO12/14 actively: LOW = release, HIGH =
  press, set LOW right at the start of `setup()`.
* **On the fabricated rev. 0.2 boards** this is retrofitted: bend pins 3
  and 6 of the 74HCT08 out of the socket, insert a diode from the bent pin
  (anode) into the socket contact (cathode), and solder 10 kΩ on the bottom
  side between socket pins 2↔7 and 4↔7.

The hood light turning on at power-up is behaviour of the hood itself (it
also happens without the board).

## Known pitfalls of the perfboard version (avoided in the redesign)

1. **GPIO0 without pull-up** was the main cause of sporadic boot failures.
   → R2 added, active via the board-J5 jumper in position 1–2.
2. **GPIO12/14 directly on the 5 V key lines** – outside the ESP
   specification. → U3 with D3/D4 and R7/R8, see "Key emulation".
   (The earlier assumption that D1/D2 protected the key lines was wrong:
   they sit on the LED lines.)
3. **The reset button was originally wired to GPIO16 instead of RST** – in
   the redesign SW1 goes directly to RST/EXT_RST.
4. **No defined power-on reset** – converter start-up behaviour could lead
   to unstable boot. → Solved by the buck-boost converter U4 (regulates from
   1.5 V) and the split buffering C8/C1. A supervisor IC (U5, MCP130) that
   was considered for a while was therefore not fitted; bring-up in 2026-09
   ran without brownout resets.
5. **Bulk capacitor too small / unclearly sized and on the wrong side**
   (original value in the post: 800 µF, later measured 3.88 mF) → split in
   the redesign: C8 (1000 µF) on `5V_IN`, C1 (470 µF) on `VCC`. See "Sizing
   the buffering".
6. **RST auto-reset capacitor:** now C7, 100 nF ceramic (non-polarised). The
   former 4.7 µF electrolytic (C2) was wrong twice over – too large a time
   constant for esptool's 100 ms reset pulse, and a coupling capacitor sees
   voltage in both directions, so a polarised one is unsuitable. Together
   with R6 (10 kΩ) this gives τ = 1 ms.

## Mechanics: enclosure and board dimensions

Requirements from the enclosure (as of 2026-09-07):

| Item | Value |
|---|---|
| Maximum board size | **55 × 80 mm** |
| Mounting holes | 4, grid **50 × 50 mm** (same vertically and horizontally) |
| Standoff inner diameter (thread) | 1.8 mm |
| Standoff outer diameter | 5.0 mm |
| First hole from the top edge | 3.5 mm from hole centre (= 1 mm from standoff edge to board edge) |

### Orientation on the board

The outline in KiCad lies **landscape**: `(0,0) → (80,0) → (80,55) → (0,55)`,
so the 80 mm edge runs along x. This is the same board as "55 × 80
portrait", just rotated by 90°. This gives the hole centres:

```
   x = 3.5 and 53.5 mm      (long side 80 mm, 1 mm margin to the standoff)
   y = 2.5 and 52.5 mm      (short side 55 mm, standoff flush with the edge)
```

**The 1 mm margin only applies to the long side** (confirmed 2026-09-07). On
the short side the standoff sits flush with the board edge:

| Direction | Hole centre | Standoff (⌀5) | Margin to edge |
|---|---|---|---|
| long side (80 mm, x) | 3.5 / 53.5 | 1.0…6.0 and 51.0…56.0 | **1 mm** |
| short side (55 mm, y) | 2.5 / 52.5 | 0…5 and 50…55 | **0 mm, flush** |

That is intentional, not a mistake.

### Component height (requirement 2026-09-07)

The inner height of the enclosure is **max. 20 mm including board and
standoffs**. Rule of thumb: **14 mm component height above the board is
safe, above that it gets risky.**

| Part | Height above PCB | |
|---|---|---|
| C8 1000 µF/10 V, D10 upright | 12.5 mm | ok |
| U4, TPS63802 module soldered flat | 3.5 mm | ok |
| U4 on pin headers | ~6–7 mm | ok |
| Mating connector (Dupont socket) on a vertical pin header | ~14.7 mm | **marginal** |
| C1 470 µF/10 V, D8 | ~11.5 mm | ok |
| D3/D4, R7/R8 upright | ~9–11 mm | ok |
| U2/U3, DIP in socket | ~9 mm | ok |
| Vertical 2.54 mm pin header, no mating connector | 8.5 mm | ok |
| Jumper on J5 | ~7.5 mm | ok |
| Ceramic disc capacitor 100 nF | 5–7 mm | ok |
| SW1/SW2 (6 mm buttons) | ~5 mm | ok |
| U1 (ESP-12E) | ~3 mm | ok |
| Resistors, diodes lying flat | 2.5 mm | ok |

So height is a **selection criterion for U4**: the converter module must
not exceed 14 mm including pin header. Together with the already known
requirements (min. Vin ≤ 4.5 V, ≥ 500 mA, three-pin on 2.54 mm grid), the
whole OKI-78SR and R-78E families are ruled out – their minimum input
voltage is 4.75 V and 7 V respectively.

### Screws and hole diameter (decided 2026-09-07)

| Item | Value |
|---|---|
| Screw | **M2** |
| Head diameter | **4.0 mm** |
| Board hole | **2.2 mm** |
| Footprint | `MountingHole:MountingHole_2.2mm_M2` |

The footprint is a pure `np_thru_hole` – 2.2 mm hole, **no copper**, no net.
It comes with two circles documenting the clearance: ⌀4.40 mm on
`Cmts.User` (screw head) and ⌀4.90 mm as `F.CrtYd` (courtyard). An extra
keep-out is therefore not needed – the courtyard keeps parts away
automatically and is checked by DRC.

The ⌀5 standoff is *irrelevant on the component side*: it sits under the
board, and there are only solder pads there with this pure THT assembly. On
top, the screw head of ⌀4 mm is the limit, and the ⌀4.9 courtyard covers it
with 0.45 mm margin.

On the short side the hole spans y = 1.4 to 3.6 mm; a 1.4 mm wide web
remains to the board edge. With the 3.2 mm (M3) holes used earlier it would
have been only 0.9 mm.

## Placement and routing

The old board (90 × 70 mm, net names from a long-gone revision) could not
be saved and was rebuilt from scratch. The board is fully routed; on
**B.Cu** there is the filled GND back plane `GND_BACKPLANE` (0.6 mm edge
clearance, 0.3 mm copper clearance, thermal relief with 0.6 mm spokes).
After filling the plane, 22 conflict-free signal and supply segments were
moved from B.Cu to F.Cu, including the direct J1↔J3 connections for
`Net-(J1-Pin_5)` to `Net-(J1-Pin_7)`. B.Cu remains reserved for unavoidable
crossings and the GND plane; further layer changes are not forced blindly if
they would cross existing F.Cu nets.
Redundant GND tracks between plated GND pads were then removed. The only
remaining F.Cu GND connection is the necessary route from SMD GND pad 15 of
U1 to the GND via; all other GND pads connect directly to `GND_BACKPLANE`.

The third placement pass (2026-09-11) was needed because choosing the
TPS63802 shrank C8 from 3300 µF lying down to 1000 µF upright, while U4 grew
from 82 to 325 mm².

### Two decisions that drive the placement

* **J1, J3 and J6 are not plugged but soldered directly.** A Dupont socket
  alone is ~14.7 mm tall, with a cable bend on top really 18–20 mm; that
  doesn't fit under the lid (limit 14 mm). The pads of the vertical pin
  headers remain in the layout, the headers are just not fitted – the wires
  go straight into the holes. J2 (FTDI) and J4 (reset) remain pluggable;
  they are only used with the enclosure open anyway.
* **U4 occupies 25.8 × 13.0 mm.** Since 2026-09-11 the real footprint is
  there (portrait, 270°: **VIN at the top, VOUT at the bottom**), the
  placeholder on `Cmts.User` is gone – see section U4.

### Zones

As of rev. 0.3 (2026-09-26), looking at the component side:

| Area | Contents |
|---|---|
| left middle | **U1** (ESP-12E), antenna at the left short edge with keep-out zone |
| top left | J2 (FTDI), C7, SW1 (reset), J4, R3 |
| bottom left | R4, R6, R2, SW2 (flash), J5, TP2 (GND), TP4 (3.3 V) |
| top middle | **U2** (4050), C4, TP5 (SCAN), TP6 (GND), C8 |
| middle | C3, C1, below them **U4** (converter module) and C6 |
| top right | R5, **J3**, **J1**, D1, D2; cable-tie holes H1/H2 |
| bottom right | J6, TP3 (5 V), TP1 (GND), D3/D4/R7/R8 upright, **U3** (74HCT08), C5; H3/H4 |

The cable routing at H1–H4 is marked as a footprint keep-out
`Kabelmontage` / `Kabelmontage_1` ("cable mounting").

Distances of the decoupling capacitors to their supply pin: C3 → U1/8 =
3.4 mm, C4 → U2/1 = 3.5 mm, C5 → U3/14 = 3.4 mm.

With the real footprint (2026-09-11) the connections are fixed: VIN at
(60.10 | 62.64 / 43.22), VOUT at (60.10 | 62.64 / 66.08), GND twice each at
x 52.48 / 55.02 in both rows. **C8 therefore sits 7.7 mm above the converter
input** – good enough for the 1000 µF buffer, especially as the module
brings its own 100 µF input capacitor and C8 is pure energy storage on the
millisecond scale.

The **output side** remains open: after the move on 2026-09-11, C6 (100 nF)
is only 3.6 mm from the VOUT pads and thus done; C1 (470 µF) is still
19.5 mm away. That is the announced follow-up of the U4 surroundings – it is
still pending and remains an optimisation item for a later placement
revision.

To make the 13.0 mm wide body fit between C6 and TP1 at all, **C6 moved
1.2 mm and TP2/TP4 1.0 mm each to the left** on 2026-09-11; C8's reference
text had to move out of the module outline. Afterwards: courtyards clear,
silkscreen clear.

### How the placement was done

Anchored by hand: U1 (antenna at the edge), U2/U3, J1/J3 (pin facing pin so
the six pass-throughs become straight tracks) and U4. The rest by script,
with cost = sum of **pad-to-pad distances** per net; GND, VCC and 5V_IN are
excluded because global nets give no locality signal. Decoupling and buffer
capacitors are instead tied with a high weight to their supply pin. Then
improvement passes until convergence.

### Checks

```
kicad-cli sch erc  --severity-all            → 0 violations
kicad-cli pcb drc  --schematic-parity
  → 0 violations                    (as of 2026-09-26, rev. 0.3;
                                     4 cable-mounting keep-outs H1–H4 are exclusions)
  → schematic_parity: 8 notes       (8 × "extra footprint" for the holes
                                     MH1–MH4 / H1–H4 without symbol – intended)
  → 0 unconnected items
  → 185 track segments, 10 vias; B.Cu GND plane filled
  → minimum copper clearance 0.200 mm, minimum hole 0.30 mm
```

Exact positions are in `hood-control.kicad_pcb`, a view in
[`board-layout.pdf`](board-layout.pdf).

## Status / next steps in KiCad

* The schematic is fully wired, **ERC: 0 errors, 0 warnings** (also with
  `--severity-all`).
* **Every part has a footprint.** Corrected on 2026-09-06: R5 (had
  `Package_DIP:DIP-8_W7.62mm` – a DIP-8 package for a two-pin resistor),
  J4/J5/J6 (had none), U4 (had the Murata land pattern), and the C1/C8
  redistribution.
* The ERC rule `footprint_filter` is set to `ignore` in the project
  settings. It would have reported the R5 error – candidate for re-enabling.
* **Warning about the KiCad MCP tools:** `move_schematic_component`,
  `delete_schematic_wire`, `add_schematic_component` and
  `add_schematic_wire` delete junctions when rewriting the file – even at
  places unrelated to the operation. Reproduced twice (30 → 23 and 30 → 18
  junctions), each time resulting in ~14 ERC errors from silently split
  nets. Make changes either in the GUI or by script directly on the file,
  and afterwards **always** count junctions and diff the netlist against the
  previous state.
* **Board rebuilt on 2026-09-07** (see "Placement"): outline 80 × 55 mm, all
  parts placed, MH1–MH4 as `MountingHole_2.2mm_M2`.
* The dead clearance exception for U5 was removed from
  `hood-control.kicad_dru` – U5 no longer exists in the circuit.
* **U4 module is here, measured and drawn** (2026-09-11): footprint in
  `hood-control.pretty`, placed on the board.
* **Fabrication files are in `kicad/fab/`**: seven Gerber layers (F.Cu, B.Cu,
  F/B.Mask, F/B.Silkscreen, Edge.Cuts), PTH and NPTH drill files, Gerber job
  file and `hood-control-gerber.zip` as upload package. Generation and order
  parameters are in [`kicad/fab/FAB.md`](fab/FAB.md). Important: fabrication
  class **6/6 mil** is mandatory – the minimum copper clearance of 0.200 mm
  is just below the 0.2032 mm that 8/8 mil would require.
* **Later placement optimisation:** the surroundings of U4 could still be
  improved – especially C1 and C6 could move closer to the VOUT side. The
  board is already routed; such a revision would require another routing /
  layer clean-up pass.
* **Rev. 0.3 (2026-09-26):** D3/D4/R7/R8 upright (2.54 mm pitch) in the row
  above U3, outside the keep-out `Kabelmontage_1`. U4 is now linked to its
  symbol; as a result `duplicate_pad_numbers_are_jumpers` is `no` –
  uncritical, all 8 U4 holes are connected on the board.
* DRC exclusions for H1–H4 (cable-tie holes inside the keep-outs) are stored
  in `hood-control.kicad_pro`.
