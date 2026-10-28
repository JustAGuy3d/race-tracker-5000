# Enclosure

A two-part case for the Elecrow CrowPanel 7.0" (DIS08070H, board V3.0) with a
battery bay, a 45° bevelled back, BOOT and RESET buttons, a power switch, a
TPU gasket, and a built-in seat for zip-tying it to a 1-2" square tube. About
190 × 110 × 33.5 mm.

The print-ready 3MF is on Printables:
[Just-A-7" CrowPanel Case](https://www.printables.com/model/1871230-just-a-7-crowpanel-case-for-the-elecrow-crowpanel).

![The final case, back view](../docs/images/render-back-plain.png)

## Files

| File | What it is |
|---|---|
| `stl/front-bezel.stl` | Front bezel. Holds the screen and the four heat-set inserts. |
| `stl/rear-shell.stl` | Rear shell: battery pocket, button guide tubes, switch slot and tube seat. |
| `stl/button-plungers.stl` | The two BOOT / RESET plungers. |
| `stl/gasket-tpu.stl` | Gasket, one piece. Print in TPU. |
| `stl/fit-gauge.stl` | Optional quick test print to check the outline and screw holes. |
| `crowpanel7_case.scad` | OpenSCAD source for all of the above. |

The STLs are already in print orientation.

## Features

- **Battery pocket:** a walled 102 × 60 mm pocket for a 1160100 LiPo, with
  room for 3 mm foam tape on both faces and a 4 mm slot for the leads.
- **BOOT and RESET buttons:** two printed plungers slide straight down guide
  tubes onto the board's buttons. Their caps stand about 1.5 mm out of the
  bevel.
- **Power switch:** a slot in the top edge for an SS12F16 panel-mount slide
  switch, held by two M2 × 5 mm screws.
- **Tube seat:** leaned back 45°, the bevel along the bottom edge is level,
  so the case sits on top of a horizontal 1-2" square tube. Two zip-tie
  tunnels run through that bevel (ties up to 7.6 mm wide, about 300 mm long
  for 2" tube).
- **Seams:** snap ridges along the edges and a TPU gasket keep the long edges
  closed between the corner screws.
- **Openings:** USB-C only. There is no TF card slot; take any card out of the
  board before closing the case, since it sticks out past the board's edge.

## Printing

- **Front bezel:** face down. No supports.
- **Rear shell:** back down. No supports: every overhang, including the bevel,
  is 45° or less. The screw-head pockets leave a 0.2 mm skin under each seat so
  it bridges cleanly; push the screw through it the first time.
- **Plungers:** lying flat, as exported. No supports.
- **Gasket:** flat, as exported, in TPU at 0.2 mm layers. The sealing fin is a
  single 0.4 mm line, so use a 0.4 mm nozzle and make sure the slicer doesn't
  drop thin walls.
- **Material:** PETG (better outdoors and in the sun) or PLA for the shell.
- I printed mine on a Snapmaker U1.

## Assembly

1. Melt the four M3 heat-set inserts into the bosses in the front bezel.
2. Put the board in the bezel, screen down.
3. Press the gasket into the channel on the bezel rim, between the lip and the
   board's edge: flat base down, thin fin up, with its gap at the USB-C port.
4. In the rear shell: drop the two plungers into their guide tubes from the
   inside, cap first. Fit the slide switch to the inside of the top wall with
   the two M2 screws, wired into the battery's positive lead.
5. Put the battery in its pocket with 3 mm foam tape on both faces, leads out
   through the slot, and plug it into the BAT socket. Check the red and black
   wires against the + and − marks on the board first.
6. Lower the rear shell straight down onto the bezel. The snap ridges click
   into the bezel lip. Fit the four M3 × 8 mm screws.

To open it again, take out the screws, then flex the bezel lip outward at the
USB-C opening to release the snap ridges.

To mount it, sit the bottom bevel on top of the tube with the screen leaning
back, thread two zip ties through the tunnels and around the tube, and pull
them tight.

## Changing the design

Everything is parameters at the top of `crowpanel7_case.scad`: battery size,
foam thickness, wall thickness, chamfers, button gap and so on. Pick the part
to export with `part`, for example:

```
openscad -o front-bezel.stl -D 'part="bezel"' crowpanel7_case.scad
```

Options you can switch:

- `sw_style`: `"slide"` (the SS12F16 in the top edge, default) or `"rocker"`
  (a KCD1 snap-in rocker on the flat back; set `batt_slot_y = 78` with it).
- `btn_style`: `"plunger"` (default) or `"flex"` (flexing tabs cut into the
  bevel, an earlier version).
- `sd_slot`: `true` adds a TF card opening in the bottom wall.
- `tie_seat`: `false` removes the zip-tie tunnels.
- Honeycomb back: `part = "rear"`, or `"rear_2c_body"` + `"rear_2c_hex"` for
  two colours.

Parts: `bezel`, `bezel_label` (front with a text label; needs a font installed
on your PC, see `label_font`), `bezel_text` (just the label, for a second
colour), `rear_plain`, `rear`, `rear_2c_body`, `rear_2c_hex`, `plungers`,
`gasket`, `gauge`, and `assembly` (exploded view over a model of the board).

The board dimensions come from Elecrow's V3.0 design files. Earlier boards
(V1.0, V2.0) may differ.

## Licence

CC BY-SA 4.0, as a derivative of the ACOS Elecrow CrowPanel Advance 7
enclosure by Alley Cat / Majestic AV, LLC. See [LICENSE.md](LICENSE.md).
