# Fertigungsdaten hood-control

Erzeugt am 2026-09-26 (Rev. 0.3, 185 Segmente / 10 Vias) aus `../hood-control.kicad_pcb` mit KiCad 10.0.6:

```
kicad-cli pcb export gerbers \
  --layers F.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,Edge.Cuts \
  --subtract-soldermask --no-x2 --no-netlist -o kicad/fab/ kicad/hood-control.kicad_pcb
kicad-cli pcb export drill --format excellon --excellon-units mm \
  --excellon-zeros-format decimal --excellon-separate-th -o kicad/fab/ kicad/hood-control.kicad_pcb
```

| Datei | Inhalt |
|---|---|
| `*-F_Cu.gtl` / `*-B_Cu.gbl` | Kupfer oben / unten |
| `*-F_Mask.gts` / `*-B_Mask.gbs` | Lötstopplack oben / unten |
| `*-F_Silkscreen.gto` | Bestückungsdruck oben |
| `*-B_Silkscreen.gbo` | Bestückungsdruck unten – **leer**, es ist nichts auf B.SilkS |
| `*-Edge_Cuts.gm1` | Umriss, geschlossenes Rechteck 80,00 × 55,00 mm |
| `*-PTH.drl` | 127 durchkontaktierte Bohrungen |
| `*-NPTH.drl` | 8 nicht durchkontaktierte (MH1–4, H1–4) |
| `*-job.gbrjob` | Gerber-Job-Datei (Lagenzuordnung + Stackup) |

Gerber und Bohrdaten stehen auf demselben absoluten Ursprung.

## Bestellparameter

| | |
|---|---|
| Maße | 80 × 55 mm, 1 Design |
| Lagen | 2 |
| Material / Dicke | FR-4, 1,6 mm |
| Kupfer | 1 oz (35 µm) |
| kleinster Leiterabstand | 0,200 mm → Klasse 6/6 mil (8/8 mil reicht **nicht**) |
| kleinste Bohrung | 0,30 mm (Vias) |
| kleinster Restring | 0,35 mm |
| Kupfer zur Kante | 0,50 mm |
| Vias | getentet (Lötstopplack über den Vias, im Board so eingestellt) |
| Oberfläche | HASL bleifrei (oder bleihaltig – für THT-Handlötung beides gut) |
| Lötstopplack / Druck | grün / weiß |
