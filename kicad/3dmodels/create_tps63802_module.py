import FreeCAD as App
import Part
import Draft
from FreeCAD import Vector
import os
import re


OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT_STEP = os.path.join(OUTPUT_DIR, "TPS63802_Module_25.8x13.0mm.step")
export_colors = []


def feature(doc, name, label, shape, color):
    obj = doc.addObject("Part::Feature", name)
    obj.Label = label
    obj.Shape = shape
    solid_count = len(shape.Solids) or 1
    export_colors.extend([tuple(color)] * solid_count)
    if obj.ViewObject is not None:
        obj.ViewObject.ShapeColor = color
    return obj


def box_centered(x_size, y_size, z_size, x, y, z):
    return Part.makeBox(
        x_size,
        y_size,
        z_size,
        Vector(x - x_size / 2.0, y - y_size / 2.0, z),
    )


def rounded_box(x_size, y_size, z_size, x, y, z, radius):
    shape = box_centered(x_size, y_size, z_size, x, y, z)
    return shape.makeFillet(radius, shape.Edges)


doc = App.newDocument("TPS63802_Module_25.8x13.0mm")

# The module dimensions and hole pattern are taken from the project footprint.
board_width = 25.8
board_depth = 13.0
board_thickness = 1.6
hole_diameter = 1.0
hole_positions = [
    (-11.43, -5.08),
    (-11.43, -2.54),
    (-11.43, 2.54),
    (-11.43, 5.08),
    (11.43, -5.08),
    (11.43, -2.54),
    (11.43, 2.54),
    (11.43, 5.08),
]

board = Part.makeBox(
    board_width,
    board_depth,
    board_thickness,
    Vector(-board_width / 2.0, -board_depth / 2.0, 0),
)
for x, y in hole_positions:
    drill = Part.makeCylinder(
        hole_diameter / 2.0,
        board_thickness,
        Vector(x, y, 0),
    )
    board = board.cut(drill)

feature(doc, "ModulePcb", "Module PCB (25.8 x 13.0 mm)", board, (0.015, 0.025, 0.02))

# The photos show four broad copper terminal areas, each carrying two holes.
# Cut the holes back out of each bar so the hole pattern remains visible.
for index, (x, y) in enumerate(
    (
        (-11.43, -3.81),
        (-11.43, 3.81),
        (11.43, -3.81),
        (11.43, 3.81),
    ),
    start=1,
):
    pad = box_centered(2.54, 4.57, 0.06, x, y, board_thickness)
    for hole_x, hole_y in hole_positions:
        if abs(hole_x - x) < 0.001 and abs(hole_y - y) <= 1.3:
            pad = pad.cut(
                Part.makeCylinder(
                    hole_diameter / 2.0,
                    0.06,
                    Vector(hole_x, hole_y, board_thickness),
                )
            )
    feature(doc, "TerminalPad%02d" % index, "Copper terminal pad %02d" % index, pad, (0.72, 0.46, 0.12))

# Copper-plated through-hole barrels provide additional detail below the
# terminal pads while keeping the holes open in the exported model.
for index, (x, y) in enumerate(hole_positions, start=1):
    outer = Part.makeCylinder(0.7, board_thickness + 0.08, Vector(x, y, 0))
    inner = Part.makeCylinder(hole_diameter / 2.0, board_thickness + 0.08, Vector(x, y, 0))
    barrel = outer.cut(inner)
    feature(doc, "PlatedHole%02d" % index, "Plated hole %02d" % index, barrel, (0.72, 0.46, 0.12))

# Simplified component solids guided by IMG_2575 and IMG_2576. The photos are
# oblique, so their positions are visual estimates rather than measurements.
top_z = board_thickness

# The reference photos have the inductor at the VIN end, the IC immediately
# below it, two input capacitors on the left, and four output capacitors in a
# row to the right.
feature(
    doc,
    "Inductor",
    "Shielded 4R7 inductor",
    rounded_box(5.2, 4.8, 1.85, -5.6, -4.0, top_z, 0.35),
    (0.45, 0.45, 0.45),
)
for side in (-1, 1):
    feature(
        doc,
        "InductorTerminal%d" % side,
        "Inductor metal termination",
        rounded_box(0.35, 3.8, 1.15, -5.6 + side * 2.45, -4.0, top_z + 0.22, 0.12),
        (0.68, 0.68, 0.68),
    )

# TPS63802 QFN-style package with five leads along each long edge.
ic_x = -3.7
ic_y = 0.35
feature(
    doc,
    "PowerIc",
    "TPS63802 QFN package",
    rounded_box(3.35, 2.95, 0.82, ic_x, ic_y, top_z, 0.16),
    (0.08, 0.08, 0.08),
)
for side in (-1, 1):
    for index in range(5):
        lead_x = ic_x - 1.04 + index * 0.52
        lead = box_centered(0.34, 0.62, 0.12, lead_x, ic_y + side * 1.69, top_z)
        feature(
            doc,
            "IcLead%d_%d" % (side, index),
            "TPS63802 lead",
            lead,
            (0.68, 0.68, 0.68),
        )

def ceramic_capacitor(name, x, y, rotation=0):
    body = rounded_box(2.2, 1.7, 0.82, x, y, top_z, 0.12)
    if rotation:
        body.rotate(Vector(x, y, top_z), Vector(0, 0, 1), rotation)
    feature(doc, name + "Body", "Ceramic capacitor body", body, (0.72, 0.58, 0.38))
    for side in (-1, 1):
        end = box_centered(0.28, 1.82, 0.9, x + side * 0.99, y, top_z)
        if rotation:
            end.rotate(Vector(x, y, top_z), Vector(0, 0, 1), rotation)
        feature(doc, name + "End%d" % side, "Ceramic capacitor termination", end, (0.72, 0.72, 0.72))


# Six large MLCCs are visible in the two reference photos. They stand upright
# in the photos, so their long axis is rotated across the module width.
for index, (x, y, rotation) in enumerate(
    (
        (-8.6, 0.55, 90),
        (-6.5, 0.55, 90),
        (0.7, -0.15, 90),
        (2.7, -0.15, 90),
        (4.7, -0.15, 90),
        (6.7, -0.15, 90),
    ),
    start=1,
):
    ceramic_capacitor("CeramicCap%02d" % index, x, y, rotation)

# Small SMD passives form the three-column array below the output capacitors.
for index, (x, y, angle) in enumerate(
    (
        (-0.2, 2.4, 0),
        (2.1, 2.4, 0),
        (4.4, 2.4, 0),
        (-0.2, 3.55, 0),
        (2.1, 3.55, 0),
        (4.4, 3.55, 0),
        (-0.2, 4.7, 0),
        (2.1, 4.7, 0),
        (4.4, 4.7, 0),
    ),
    start=1,
):
    resistor = rounded_box(1.35, 0.72, 0.42, x, y, top_z, 0.08)
    if angle:
        resistor.rotate(Vector(x, y, top_z), Vector(0, 0, 1), angle)
    feature(doc, "Passive%02d" % index, "Passive component %02d" % index, resistor, (0.06, 0.06, 0.05))
    for side in (-1, 1):
        end = box_centered(0.24, 0.78, 0.48, x + side * 0.56, y, top_z)
        if angle:
            end.rotate(Vector(x, y, top_z), Vector(0, 0, 1), angle)
        feature(
            doc,
            "Passive%02dEnd%d" % (index, side),
            "Passive component termination",
            end,
            (0.72, 0.72, 0.72),
        )

# The lower-left corner contains the selectable-output solder bridge.
for index, x in enumerate((-8.35, -7.15), start=1):
    feature(
        doc,
        "OutputBridgePad%02d" % index,
        "3V3 solder bridge pad %02d" % index,
        box_centered(0.78, 1.7, 0.06, x, 5.15, top_z),
        (0.78, 0.52, 0.10),
    )

# A small extra passive is visible beside the bridge in both photos.
feature(
    doc,
    "BridgePassive",
    "Output bridge passive",
    rounded_box(1.35, 0.72, 0.42, -5.55, 5.05, top_z, 0.08),
    (0.06, 0.06, 0.05),
)

# Small status LED beside the output-side passive array.
led = rounded_box(1.0, 0.95, 0.55, 8.4, 4.1, top_z, 0.12)
feature(doc, "StatusLed", "Status LED", led, (0.08, 0.08, 0.07))


def label_feature(name, text, x, y, size, rotation=0):
    label = Draft.makeShapeString(
        text,
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        size,
    )
    label.Placement = App.Placement(
        Vector(x, y, top_z + 0.04),
        App.Rotation(Vector(0, 0, 1), rotation),
    )
    doc.recompute()
    shape = label.Shape.extrude(Vector(0, 0, 0.03))
    doc.removeObject(label.Name)
    feature(doc, name, "Board label %s" % text, shape, (0.9, 0.9, 0.82))


# The reference photo has white silkscreen identifying the converter and its
# four terminal groups. These labels are raised slightly so they remain visible
# in the 3D rendering above the dark module substrate.
label_feature("LabelPartNumber", "TPS63802", -1.0, -5.45, 1.05)
label_feature("LabelFunction", "BUCK-BOOST", -1.0, -4.1, 0.85)
label_feature("LabelVin", "VIN", -8.4, -5.35, 1.0, 90)
label_feature("LabelVout", "VOUT", 9.5, -5.35, 1.0, 90)
label_feature("LabelGndLeft", "GND", -8.4, 2.0, 1.0, 90)
label_feature("LabelGndRight", "GND", 9.5, 2.0, 1.0, 90)

doc.recompute()
Part.export(doc.Objects, OUTPUT_STEP)


def add_step_colors(filename, colors):
    text = open(filename, "r", encoding="ascii").read()
    shape_match = re.search(
        r"#10 = SHAPE_REPRESENTATION\('',\((.*?)\),\s*#\d+\);",
        text,
        re.DOTALL,
    )
    if shape_match is None:
        raise RuntimeError("Could not find the exported STEP shape representation")

    roots = [int(value) for value in re.findall(r"#(\d+)", shape_match.group(1))]
    solid_roots = roots[1:]
    if len(solid_roots) != len(colors):
        raise RuntimeError(
            "STEP object count does not match the color list: %d != %d"
            % (len(solid_roots), len(colors))
        )

    entity_ids = [int(value) for value in re.findall(r"(?m)^#(\d+)", text)]
    next_id = max(entity_ids) + 1
    style_entities = []
    for root, (red, green, blue) in zip(solid_roots, colors):
        colour_id = next_id
        fill_colour_id = next_id + 1
        fill_style_id = next_id + 2
        surface_fill_id = next_id + 3
        surface_side_id = next_id + 4
        surface_usage_id = next_id + 5
        presentation_id = next_id + 6
        styled_item_id = next_id + 7
        style_entities.extend(
            (
                "#%d = COLOUR_RGB('',%.6f,%.6f,%.6f);"
                % (colour_id, red, green, blue),
                "#%d = FILL_AREA_STYLE_COLOUR('',#%d);"
                % (fill_colour_id, colour_id),
                "#%d = FILL_AREA_STYLE('',(#%d));" % (fill_style_id, fill_colour_id),
                "#%d = SURFACE_STYLE_FILL_AREA(#%d);" % (surface_fill_id, fill_style_id),
                "#%d = SURFACE_SIDE_STYLE('',(#%d));" % (surface_side_id, surface_fill_id),
                "#%d = SURFACE_STYLE_USAGE(.BOTH.,#%d);"
                % (surface_usage_id, surface_side_id),
                "#%d = PRESENTATION_STYLE_ASSIGNMENT((#%d));"
                % (presentation_id, surface_usage_id),
                "#%d = STYLED_ITEM('',(#%d),#%d);"
                % (styled_item_id, presentation_id, root),
            )
        )
        next_id += 8

    endsec = text.rfind("ENDSEC;")
    if endsec < 0:
        raise RuntimeError("Could not find the STEP data section terminator")
    styled_text = text[:endsec] + "\n" + "\n".join(style_entities) + "\n" + text[endsec:]
    open(filename, "w", encoding="ascii").write(styled_text)


add_step_colors(OUTPUT_STEP, export_colors)
doc.saveAs(os.path.join(OUTPUT_DIR, "TPS63802_Module_25.8x13.0mm.FCStd"))
print("Wrote %s" % OUTPUT_STEP)
