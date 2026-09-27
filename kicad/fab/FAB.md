🇬🇧 **English** · 🇩🇪 [Deutsch](FAB.de.md)

# Fabrication files hood-control

Generated on 2026-09-26 (rev. 0.3, 185 track segments / 10 vias) from
`../hood-control.kicad_pcb` with KiCad 10.0.6:

```
kicad-cli pcb export gerbers \
  --layers F.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,Edge.Cuts \
  --subtract-soldermask --no-x2 --no-netlist -o kicad/fab/ kicad/hood-control.kicad_pcb
kicad-cli pcb export drill --format excellon --excellon-units mm \
  --excellon-zeros-format decimal --excellon-separate-th -o kicad/fab/ kicad/hood-control.kicad_pcb
```

| File | Contents |
|---|---|
| `*-F_Cu.gtl` / `*-B_Cu.gbl` | copper top / bottom |
| `*-F_Mask.gts` / `*-B_Mask.gbs` | solder mask top / bottom |
| `*-F_Silkscreen.gto` | silkscreen top |
| `*-B_Silkscreen.gbo` | silkscreen bottom – **empty**, nothing is on B.SilkS |
| `*-Edge_Cuts.gm1` | outline, closed rectangle 80.00 × 55.00 mm |
| `*-PTH.drl` | 127 plated holes |
| `*-NPTH.drl` | 8 non-plated holes (MH1–4, H1–4) |
| `*-job.gbrjob` | Gerber job file (layer assignment + stackup) |

Gerber and drill files share the same absolute origin. Upload
`hood-control-gerber.zip` to any PCB manufacturer.

## Order parameters

| | |
|---|---|
| Size | 80 × 55 mm, 1 design |
| Layers | 2 |
| Material / thickness | FR-4, 1.6 mm |
| Copper | 1 oz (35 µm) |
| Minimum clearance | 0.200 mm → class 6/6 mil (8/8 mil is **not** enough) |
| Minimum hole | 0.30 mm (vias) |
| Minimum annular ring | 0.35 mm |
| Copper to edge | 0.50 mm |
| Vias | tented (solder mask over vias, set in the board) |
| Surface finish | lead-free HASL (or leaded – both fine for hand-soldered THT) |
| Solder mask / silkscreen | green / white |
