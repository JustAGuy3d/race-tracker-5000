// Enclosure for Elecrow CrowPanel 7.0" HMI ESP32 Display (DIS08070H, PCB V3.0)
//
// Board dimensions come from Elecrow's published V3.0 Eagle .brd and STEP files.
// Honeycomb back pattern and overall style follow the ACOS CrowPanel Advance 7
// enclosure by Alley Cat / Majestic AV, LLC (CC BY-SA 4.0) - this derivative
// is therefore CC BY-SA 4.0 as well.
//
// Coordinate frame: looking at the component side of the PCB (the back),
// origin at the lower-left PCB corner with the USB-C port on the right edge.
// z = 0 is the component face of the PCB, +z goes towards the rear cover,
// -z goes towards the screen.

// The optional front label (part = "bezel_label" / "bezel_text") needs a font
// installed on your PC; set label_font to its name. It is off by default.

part = "assembly"; // "bezel" | "bezel_label" | "rear_plain" | "bezel_text" | "gasket" | "rear" | "rear_2c_body" | "rear_2c_hex" | "gauge" | "assembly" | "mock"

/* [Board - from Elecrow V3.0 files] */
pcb      = [181.0, 104.0];
pcb_t    = 1.6;
pcb_r    = 2.5;
hole_in  = 3.5;                 // mounting holes: 174 x 97 mm pattern, dia 3.2
stack    = 5.48;                // touch glass front face to PCB front face
active   = [15.07, 11.43, 168.91, 97.06]; // visible LCD area x0,y0,x1,y1
usb_y    = 51.29;               // USB-C centre along the right edge
usb_zc   = 1.6;                 // USB-C centre above the PCB
sd_x     = 148.6;               // TF card centre along the bottom edge
sd_zc    = 1.4;
bat_y    = 23.0;                // BAT socket centre (side entry, right edge)
buttons  = [[175.36, 84.72, "BOOT"], [175.48, 70.33, "RST"]];
btn_h    = 2.0;
comp_h   = 6.0;                 // tallest part on the back is 5.5 mm (edge connectors)
under_batt_h = 4.5;             // tallest parts under the battery (CD54 inductors); board-side foam rests on these

/* [Battery bay] */
batt     = [102, 60, 11.5];     // 1160100 10000mAh: pocket is 102 x 60 inside the wall
batt_c   = [95, 52];            // battery centre (kept clear of the antenna)
batt_fit = 0;
batt_slot = 4;                  // gap in the battery wall for the leads
batt_slot_y = 27;               // slot centre, on the wall facing the BAT socket
foam_board = 3.0;               // foam tape between battery and board
foam_shell = 3.0;               // foam tape between battery and rear shell

/* [Case] */
clr      = 0.5;                 // gap around the PCB
wall     = 2.4;
corner_r = 7;                   // outer corner radius seen from the front
face_t   = 1.6;                 // bezel thickness over the glass
front_cham = 2.2;               // chamfer on the front outer edge
glass_gap= 0.4;                 // air gap between bezel and glass
win_margin = 1.0;               // bezel window margin around the active area
win_cham = 1.2;
back_t   = 2.4;
bevel_z  = 7.5;                 // 45 degree back bevel starts this far above the PCB (outside)
lip_h    = 3.0;                 // overlap joint between the two halves
lip_w    = 1.2;
fit      = 0.2;
tongue_relief = 0.4;            // inner tongue stops short so only the visible outer seam touches
// TPU gasket: sits in a channel on the bezel rim, between the lip and the PCB edge
gasket_ch   = 0.6;              // channel depth below the PCB face
gasket_base = 0.5;              // flat base thickness
gasket_fin  = [0.4, 0.7];       // soft sealing fin on top: width, height (crushed 0.2 by the rear rim)
// snap ridges along the joint, between the corner screws: [wall, position along it, length]
// walls: "b" bottom, "t" top, "l" left, "r" right (looking at the back)
snaps    = [["b", 40, 28], ["b", 90, 28], ["b", 122, 28],
            ["t", 45, 28], ["t", 85, 28], ["t", 150, 28],
            ["l", 30, 22], ["l", 75, 22], ["r", 78, 22]];
snap_h   = 0.5;                 // ridge height; 0.3 of it hooks behind the bezel lip
snap_z   = 1.9;                 // ridge centre above the PCB
sw_notch = 24;                  // the bezel lip is cut away around the switch

/* [Front label, on the bezel strip below the screen] */
label      = "Race Tracker 5000";
label_font = "Liberation Sans:style=Bold";
label_size = 5;
label_depth = 0.6;              // debossed; the separate text part fills it flush for two-colour prints

/* [Battery plug pod on the USB side] */
ear_d    = 3;                   // inside clearance beyond the case wall for the PH plug
ear_len  = 14;
ear_cav_h = 5.8;                // pocket height above the PCB (also capped by the sloped roof)
ear_roof = 1.6;                 // pod roof thickness; the roof continues the 45 degree back bevel

/* [Power switch: SS12F16 panel-mount slide switch, top edge] */
sw_x      = 118.5;              // centre along the top edge (between UART0 socket and touch FPC)
sw_z      = 3.9;                // centre above the PCB
sw_flange = [19.6, 5.6];        // mounting plate
sw_holes  = 15;                 // M2 threaded holes in the plate
sw_slot   = [7, 3.6];           // 3 mm knob + 3 mm travel
sw_body   = [11, 5.6, 8];       // body incl. solder lugs behind the plate

/* [BOOT / RESET: flush flexure buttons cut into the bevel above the switches] */
btn_tab   = [8, 1.0];           // tab width, tab thickness (thinned from inside)
btn_hinge_x = 169.5;            // hinge line, upper end of the tab (outer surface x)
btn_end_x = 180.5;              // free end of the tab (outer surface x)
btn_slot  = 0.6;                // cut around the tab
btn_gap   = 0.3;                // post tip to switch top, at rest
btn_post  = 5;                  // square post
btn_tip   = 3;                  // square contact face on the switch (switch body is 3.2 x 5.2)

/* [Fasteners: M3 heat-set inserts + M3x8 screws] */
insert_d     = 4.5;             // hole for M3 x 4 mm long x 5 mm OD inserts
insert_depth = 5.0;
screw_d      = 3.4;
head_d       = 6.4;
seat_t       = 3.6;             // plastic under the screw head

/* [Honeycomb - as on the ACOS rear shell] */
hex_r     = 5.31;
hex_depth = 1.4;
hex_inlay = 0.6;                // thickness of the separate colour inlay under each pocket
hex_edge  = 1.5;                // keep whole hexagons this far inside the flat back

$fn = 40;
eps = 0.01;

depth = max(comp_h + 0.5, under_batt_h + foam_board + batt.z + foam_shell); // rear cavity depth above PCB
zr    = depth + back_t;                        // outer back face
zf    = -(pcb_t + stack + glass_gap + face_t); // outer front face
bevel = zr - bevel_z;                          // bevel size, horizontal and vertical
bevel_zi = bevel_z - 0.5;                      // inside of the bevel starts here
holes = [for (x = [hole_in, pcb.x - hole_in], y = [hole_in, pcb.y - hole_in]) [x, y]];
ctr   = [pcb.x / 2, pcb.y / 2];
out0  = [-clr - wall, -clr - wall];
out1  = pcb + [clr + wall, clr + wall];
flat0 = out0 + [bevel, bevel];                 // flat part of the back
flat1 = out1 - [bevel, bevel];

// ---------------------------------------------------------------- outlines
module rrect(p0, p1, r) {
    translate(p0) offset(r = r) offset(delta = -r) square(p1 - p0);
}
// rounded rectangle shrunk by d on every side, corner radius shrinking with it
module inset_rect(p0, p1, r, d) {
    rr = max(r - d, 0.05);
    hull() for (x = [p0.x + d + rr, p1.x - d - rr], y = [p0.y + d + rr, p1.y - d - rr])
        translate([x, y]) circle(r = rr);
}
module main_cav(d = 0) { inset_rect([-clr, -clr], pcb + [clr, clr], pcb_r + clr, d); }
module main_out(d = 0) { inset_rect(out0, out1, corner_r, d); }
ear0 = [pcb.x - 6, bat_y - ear_len / 2];
ear1 = [pcb.x + clr + ear_d, bat_y + ear_len / 2];
module ear_cav() { inset_rect(ear0, ear1, 3, 0); }
module ear_out(d = 0) { inset_rect(ear0 - [wall, wall], ear1 + [wall, wall], 3 + wall, d); }
module cav2d() { main_cav(); ear_cav(); }
module outer2d() { main_out(); ear_out(); }

// extrude with a stepped 45 degree chamfer on the bottom edge
module cham_extrude(h, c) {
    n = max(1, round(c / 0.2));
    s = c / n;
    for (i = [0 : n - 1])
        translate([0, 0, i * s]) linear_extrude(s + eps) offset(delta = -(c - i * s)) children();
    translate([0, 0, c]) linear_extrude(h - c) children();
}
module slice(z) { translate([0, 0, z]) linear_extrude(eps) children(); }

// slot through a side wall: axis "x" cuts the right wall, "y" the bottom wall
module wall_slot(axis, pos, zc, w, h) {
    r = h / 2;
    if (axis == "x")
        hull() for (s = [-1, 1])
            translate([pcb.x - 0.3, pos + s * (w / 2 - r), zc])
                rotate([0, 90, 0]) cylinder(r = r, h = clr + wall + 2);
    else
        hull() for (s = [-1, 1])
            translate([pos + s * (w / 2 - r), 0.3, zc])
                rotate([90, 0, 0]) cylinder(r = r, h = clr + wall + 2);
}
module openings() {
    wall_slot("x", usb_y, usb_zc, 13.5, 8);   // USB-C plug overmould
    wall_slot("y", sd_x, sd_zc, 15, 6);       // TF card + fingernail access
}

// slide switch: bolts to the inside of the top wall with two M2 x 5 screws
// from outside; the knob comes out through the slot
module switch_cut() {
    y0 = pcb.y + clr;           // inner face of the top wall
    translate([sw_x, y0 - 1, sw_z]) {
        translate([-sw_slot.x / 2, 0, -sw_slot.y / 2]) cube([sw_slot.x, wall + 2, sw_slot.y]);
        for (s = [-1, 1]) translate([s * sw_holes / 2, 0, 0]) rotate([-90, 0, 0]) cylinder(d = 2.3, h = wall + 2);
        // finger recess so the knob stands proud
        translate([-5.5, 1 + wall - 0.9, -2.2]) cube([11, 2, 4.4]);
    }
}

// local frame on a wall: x runs along the wall, -y points out of the case,
// origin on the cavity face
module on_wall(side, pos) {
    if (side == "b") translate([pos, -clr, 0]) children();
    else if (side == "t") translate([pos, pcb.y + clr, 0]) rotate(180) children();
    else if (side == "l") translate([-clr, pos, 0]) rotate(-90) children();
    else translate([pcb.x + clr, pos, 0]) rotate(90) children();
}
// triangular ridge / groove profile at distance y0 outside the cavity face
module snap_profile(len, y0, h, zc) {
    translate([-len / 2, 0, 0]) rotate([90, 0, 90]) linear_extrude(len)
        polygon([[-y0 + 0.1, zc - h - 0.1], [-y0 - h, zc], [-y0 + 0.1, zc + h + 0.1]]);
}
module snap_ridges() {
    for (sn = snaps) on_wall(sn[0], sn[1]) snap_profile(sn[2], wall - lip_w - fit, snap_h, snap_z);
}
module snap_grooves() {
    for (sn = snaps) on_wall(sn[0], sn[1]) snap_profile(sn[2] + 2, wall - lip_w, snap_h - 0.05, snap_z - 0.15);
}
// where the switch sits, the rear wall runs full thickness down to the bezel
module switch_wall2d(g) {
    intersection() {
        main_out();
        translate([sw_x - sw_notch / 2 + g, pcb.y + clr]) square([sw_notch - 2 * g, 10]);
    }
}

// gasket channel: the bezel rim inside the lip is lowered; the PCB edge forms the inner side
module gasket_channel() {
    translate([0, 0, -gasket_ch]) linear_extrude(gasket_ch + eps) difference() {
        offset(r = wall - lip_w) cav2d();
        cav2d();
    }
}
// printed flat in TPU; interrupted at the USB-C and TF card slots
module gasket() {
    difference() {
        union() {
            translate([0, 0, -gasket_ch]) linear_extrude(gasket_base) difference() {
                offset(r = wall - lip_w - 0.1) cav2d();
                main_cav(clr - 0.15);
                ear_cav();
            }
            translate([0, 0, -gasket_ch + gasket_base - eps]) linear_extrude(gasket_fin[1] + eps) difference() {
                offset(r = 0.55 + gasket_fin[0]) cav2d();
                offset(r = 0.55) cav2d();
            }
        }
        translate([pcb.x - 3, usb_y - 13.5 / 2 - 0.6, -5]) cube([12, 13.5 + 1.2, 10]);
        translate([sd_x - 15 / 2 - 0.6, -8, -5]) cube([15 + 1.2, 10, 10]);
    }
}

// square pad around a mounting hole, reaching slightly into the wall
module corner_pad(p, inner) {
    sx = p.x < ctr.x ? 1 : -1;
    sy = p.y < ctr.y ? 1 : -1;
    translate(p) hull() {
        translate([-sx * 4.3, -sy * 4.3]) circle(r = 0.1);
        translate([sx * (inner - 1.5), -sy * 4.3]) circle(r = 0.1);
        translate([-sx * 4.3, sy * (inner - 1.5)]) circle(r = 0.1);
        translate([sx * (inner - 1.5), sy * (inner - 1.5)]) circle(r = 1.5);
    }
}

// ---------------------------------------------------------------- bezel
// label centred on the flat strip between the front chamfer and the window;
// mirrored here because this frame looks at the case from the back
module front_label(h) {
    y_mid = (out0.y + front_cham + active[1] - win_margin - win_cham) / 2;
    translate([ctr.x, y_mid, zf]) mirror([1, 0, 0]) linear_extrude(h)
        text(label, size = label_size, font = label_font, halign = "center", valign = "center");
}
module bezel(with_label = false) {
    boss_z0 = zf + face_t - eps;
    difference() {
        union() {
            difference() {
                translate([0, 0, zf]) cham_extrude(-zf, front_cham) outer2d();
                translate([0, 0, zf + face_t]) linear_extrude(-zf) cav2d();
            }
            // outer half of the overlap joint
            translate([0, 0, -0.5]) linear_extrude(lip_h + 0.5) difference() {
                outer2d();
                offset(r = wall - lip_w) cav2d();
            }
            // insert bosses: the PCB front face rests on these
            for (p = holes) translate([0, 0, boss_z0])
                linear_extrude(-pcb_t - boss_z0) corner_pad(p, 3.9);
            // mid-span rests under the PCB side strips (outside the LCD)
            for (r = [[-clr, 46, 7.4, 58], [pcb.x - 7.4, 62, pcb.x + clr, 74]])
                translate([r[0], r[1], boss_z0]) cube([r[2] - r[0], r[3] - r[1], -pcb_t - boss_z0]);
        }
        // display window, chamfered towards the outside
        w0 = [active[0] - win_margin, active[1] - win_margin];
        w1 = [active[2] + win_margin, active[3] + win_margin];
        translate([0, 0, zf - 1]) linear_extrude(face_t + 2) rrect(w0, w1, 1);
        hull() {
            slice(zf - eps) rrect(w0 - [win_cham, win_cham], w1 + [win_cham, win_cham], 1 + win_cham);
            slice(zf + win_cham) rrect(w0, w1, 1);
        }
        for (p = holes) translate([p.x, p.y, -pcb_t - insert_depth]) {
            cylinder(d = insert_d, h = insert_depth + 1);
            translate([0, 0, insert_depth - 0.6]) cylinder(d1 = insert_d, d2 = insert_d + 0.8, h = 0.6 + eps);
        }
        if (with_label) translate([0, 0, -eps]) front_label(label_depth + eps);
        openings();
        snap_grooves();
        gasket_channel();
        translate([0, 0, tongue_relief]) linear_extrude(lip_h + 1) switch_wall2d(0);
    }
}

// ---------------------------------------------------------------- rear shell
hex_all = concat(
    [for (band = [-28, 0, 28], k = [-3 : 3]) [k * 24.25, band]],
    [for (band = [-28, 0, 28], k = [-2.5 : 2.5], s = [-7, 7]) [k * 24.25, band + s]]);
// only whole hexagons that sit on the flat part of the back
hex_fit = [for (h = hex_all)
    if (ctr.x + h.x - hex_r >= flat0.x + hex_edge && ctr.x + h.x + hex_r <= flat1.x - hex_edge &&
        ctr.y + h.y - hex_r * 0.866 >= flat0.y + hex_edge && ctr.y + h.y + hex_r * 0.866 <= flat1.y - hex_edge) h];
module honeycomb() { for (h = hex_fit) translate(h) circle(r = hex_r, $fn = 6); }

// wall all round the battery, with a slot for the leads on the BAT-socket side
module battery_corral() {
    b  = [batt.x + batt_fit, batt.y + batt_fit];
    t  = 1.6;
    rh = foam_shell + min(batt.z - 1, 8);
    translate([0, 0, depth - rh]) linear_extrude(rh + eps) difference() {
        translate(batt_c - b / 2 - [t, t]) square(b + [2 * t, 2 * t]);
        translate(batt_c - b / 2) square(b);
        translate([batt_c.x + b.x / 2 - 1, batt_slot_y - batt_slot / 2]) square([t + 2, batt_slot]);
    }
}
// outside of the rear shell: short vertical band, then a 45 degree bevel to the flat back
module rear_envelope() {
    hull() {
        translate([0, 0, lip_h]) linear_extrude(bevel_z - lip_h) main_out();
        slice(bevel_z + corner_r) main_out(corner_r);
        slice(zr - eps) main_out(bevel);
    }
    // pod over the battery plug: its top is the back bevel carried straight on,
    // so nothing overhangs more than 45 degrees when printed
    intersection() {
        translate([0, 0, lip_h]) linear_extrude(zr) ear_out();
        below_bevel(0);
    }
    // inner half of the overlap joint
    linear_extrude(lip_h + eps) offset(r = wall - lip_w - fit) cav2d();
    translate([0, 0, tongue_relief + 0.1]) linear_extrude(lip_h) switch_wall2d(fit);
}
module rear_cavity() {
    hull() {
        translate([0, 0, -1]) linear_extrude(bevel_zi + 1) main_cav();
        slice(bevel_zi + pcb_r + clr) main_cav(pcb_r + clr);
        slice(depth - eps) main_cav(depth - bevel_zi);
    }
    intersection() {
        translate([0, 0, -1]) linear_extrude(ear_cav_h + 1) ear_cav();
        below_bevel(ear_roof);
    }
}
// everything under the right-hand bevel plane, moved inward by d (measured square to the plane)
module below_bevel(d) {
    multmatrix([[1 / s2, 0, 1 / s2, 0],
                [0, 1, 0, 0],
                [-1 / s2, 0, 1 / s2, bev_k],
                [0, 0, 0, 1]])
        translate([-1000, -1000, -1000]) cube([2000, 2000, 1000 - d]);
}
// right-hand bevel: outer surface is the plane x + z = bev_k
bev_k = out1.x + bevel_z;
s2 = sqrt(2);
// frame on the bevel at a button: X down the slope, Y along the edge, Z out of the case,
// origin on the outer surface at the hinge line
module on_bevel(y) {
    multmatrix([[1 / s2, 0, 1 / s2, btn_hinge_x],
                [0, 1, 0, y],
                [-1 / s2, 0, 1 / s2, bev_k - btn_hinge_x],
                [0, 0, 0, 1]]) children();
}
btn_len = (btn_end_x - btn_hinge_x) * s2;
module button_cuts(b) {
    w = btn_tab[0];
    on_bevel(b.y) {
        // U-shaped slot: the tab stays joined along the hinge
        difference() {
            translate([0, -w / 2 - btn_slot, -4]) cube([btn_len + btn_slot, w + 2 * btn_slot, 5]);
            translate([-1, -w / 2, -5]) cube([btn_len + 1, w, 7]);
        }
        // thin the tab from the inside so it flexes
        translate([0, -w / 2, -4]) cube([btn_len, w, 4 - btn_tab[1]]);
        // engraved ring marking where to press
        translate([(b.x - btn_hinge_x) * s2, 0, -0.3]) difference() {
            cylinder(d = 6, h = 1);
            translate([0, 0, -1]) cylinder(d = 4.8, h = 3);
        }
    }
}
// post hanging from the tab down to the switch
module button_post(b) {
    z_top = bev_k - btn_tab[1] * s2 - b.x;     // tab inner face above the switch
    z0 = btn_h + btn_gap;
    intersection() {
        translate([b.x, b.y, z0]) {
            hull() {
                translate([-btn_tip / 2, -btn_tip / 2, 0]) cube([btn_tip, btn_tip, eps]);
                translate([-btn_post / 2, -btn_post / 2, 0.5]) cube([btn_post, btn_post, eps]);
            }
            translate([-btn_post / 2, -btn_post / 2, 0.5]) cube([btn_post, btn_post, z_top - z0 + 1.5]);
        }
        // stop 0.3 under the outer face so the tab stays flush
        on_bevel(b.y) translate([-20, -20, -40]) cube([60, 40, 40 - 0.3]);
    }
}

// floor of every honeycomb pocket, as its own body for two-colour printing
module hex_inlays() {
    translate([ctr.x, ctr.y, zr - hex_depth - hex_inlay]) linear_extrude(hex_inlay) honeycomb();
}
module rear(split = false, hexes = true) {
    // button posts go on after the cuts so the tab thinning cannot sever them
    for (b = buttons) button_post(b);
    difference() {
        union() {
            difference() { rear_envelope(); rear_cavity(); }
            snap_ridges();
            intersection() {
                rear_envelope();
                union() {
                    // corner posts clamp the PCB against the bezel bosses
                    linear_extrude(zr) for (p = holes) corner_pad(p, 4.5);
                    battery_corral();
                }
            }
        }
        for (p = holes) {
            // 0.2 mm skin is left under the head seat so it bridges cleanly;
            // push the screw through it on first assembly
            translate([p.x, p.y, -1]) cylinder(d = screw_d, h = seat_t - 0.2 + 1);
            translate([p.x, p.y, seat_t]) cylinder(d = head_d, h = zr);
        }
        for (b = buttons) {
            button_cuts(b);
            // label on the flat back, level with its hole on the bevel
            translate([flat1.x - 1.2, b.y, zr - 0.4]) linear_extrude(1)
                text(b[2], size = 2.8, font = "Liberation Sans:style=Bold", halign = "right", valign = "center");
        }
        if (hexes) translate([ctr.x, ctr.y, zr - hex_depth]) linear_extrude(hex_depth + 1) honeycomb();
        openings();
        switch_cut();
        if (split) hex_inlays();
        // relief under the tongue, except at the corner posts that clamp the PCB
        translate([0, 0, -eps]) linear_extrude(tongue_relief) difference() {
            offset(r = wall) cav2d();
            cav2d();
            for (p = holes) corner_pad(p, 4.5);
        }
    }
}

// ---------------------------------------------------------------- fit gauge
// Quick print to confirm outline, hole positions and stack height before the
// long prints. Lay the board in it screen-down: the pegs must drop into all
// four holes and the glass must not touch the table.
module gauge() {
    rest = stack + glass_gap;
    linear_extrude(rest + pcb_t) difference() { offset(r = 1.6) cav2d(); cav2d(); }
    linear_extrude(rest) intersection() {
        union() for (p = holes) corner_pad(p, 3.9);
        offset(r = 0.1) cav2d();
    }
    for (p = holes) translate([p.x, p.y, 0]) cylinder(d = 2.9, h = rest + 2.5);
}

// ---------------------------------------------------------------- board mock
module mock(grow = 0) {
    module b(p0, p1) { translate(p0 - [grow, grow, 0]) cube(p1 - p0 + [2 * grow, 2 * grow, grow]); }
    color("red") difference() {
        translate([0, 0, -pcb_t]) linear_extrude(pcb_t) rrect([0, 0], pcb, pcb_r);
        for (p = holes) translate([p.x, p.y, -5]) cylinder(d = 3.2, h = 10);
    }
    color("#222") b([8.07, 2.02, -pcb_t - stack], [172.93, 101.98, -pcb_t]);   // LCD + touch glass
    color("silver") {
        b([173.7, 46.82, 0], [180.8, 55.76, 3.21]);      // USB-C
        b([141.1, 0.2, 0], [158.1, 15.9, 2.4]);          // TF socket
        b([143.1, -2.39, 0.7], [154.1, 12, 2.15]);       // TF card
        b([7.5, 42.0, 0], [26.8, 61.6, 3.22]);           // ESP32-S3 module
        for (bt = buttons) b([bt.x - 1.6, bt.y - 2.6, 0], [bt.x + 1.6, bt.y + 2.6, btn_h]);
    }
    color("white") {
        b([171.5, 19.0, 0], [180.6, 27.0, 5.53]);        // BAT
        b([41.4, 0.2, 0], [49.5, 8.9, 5.53]);            // SPK
        for (x = [31.5, 63.6, 95.6]) b([x, 92.9, 0], [x + 13.6, 103.6, 5.0]); // I2C / GPIO_D / UART0
        b([180.6, 20.0, 0.5], [pcb.x + clr + ear_d - 0.5, 26.0, 4.6]); // PH plug in the BAT socket
    }
    color("#333") {
        b([136.5, 91.3, 0], [155.9, 103.4, 3.0]);        // 8-pin header
        b([124.2, 91.3, 0], [132.2, 97.4, 1.0]);         // touch FPC socket
    }
    color("gray") b([30, 12, 0], [168, 90, under_batt_h]);   // misc. parts keep-out
    y0 = pcb.y + clr;
    color("#222") {                                          // slide switch
        translate([sw_x - sw_flange.x / 2, y0 - 0.5, sw_z - sw_flange.y / 2]) cube([sw_flange.x, 0.5, sw_flange.y]);
        translate([sw_x - sw_body.x / 2, y0 - sw_body.z, sw_z - sw_body.y / 2]) cube([sw_body.x, sw_body.z, sw_body.y]);
        translate([sw_x - 3, y0, sw_z - 1.5]) cube([3, 4, 3]);
    }
}
// battery with its two foam layers
module battery_mock() {
    color("gold") translate([batt_c.x - batt.x / 2, batt_c.y - batt.y / 2, under_batt_h + 0.05])
        cube([batt.x, batt.y, depth - under_batt_h - 0.1]);
}

if (part == "bezel") translate([0, 0, -zf]) bezel();       // prints front face down
else if (part == "bezel_label") translate([0, 0, -zf]) bezel(with_label = true);
else if (part == "bezel_text") translate([0, 0, -zf]) front_label(label_depth);
else if (part == "gasket") translate([0, 0, gasket_ch]) gasket();
else if (part == "rear") translate([0, pcb.y, zr]) rotate([180, 0, 0]) rear(); // prints back face down
else if (part == "rear_plain") translate([0, pcb.y, zr]) rotate([180, 0, 0]) rear(hexes = false);
else if (part == "rear_2c_body") translate([0, pcb.y, zr]) rotate([180, 0, 0]) rear(split = true);
else if (part == "rear_2c_hex") translate([0, pcb.y, zr]) rotate([180, 0, 0]) hex_inlays();
else if (part == "gauge") gauge();
else if (part == "mock") { mock(); battery_mock(); }
else if (part == "clash_bezel") intersection() { bezel(); mock(0.15); }
else if (part == "clash_rear") intersection() { rear(); union() { mock(0.15); battery_mock(); } }
else if (part == "clash_halves") intersection() { bezel(); rear(); }
else {
    color("#4a6fa5") bezel();
    color("orange") gasket();
    mock();
    battery_mock();
    color("#7aa0d4", 0.5) translate([0, 0, 34]) rear();
}
