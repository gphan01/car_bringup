// Electronics plate for the LiDAR car (Traxxas Slash 2WD) — print in PETG.  v3
//
// Replaces the body: the two stock body-latch BARS are screwed to its underside, so the plate
// clicks onto the chassis towers exactly like the body does.
//
// Open in OpenSCAD: F5 preview (grey ghosts = parts, for clearance checks), F6 render,
// File > Export > STL. Set `part` below to choose what gets exported.
//
// Units mm. Frame matches ROS: origin = midpoint of the four tower latch centers,
// +X forward, +Y = car's left, +Z up. Plate underside at z = 0; the end strips hang 2 mm lower and
// rest on the bars, so the plate's underside is ~137 mm above the floor.
//
// HOLES ARE NOT PRINTED for the latch bars or the electronics — mark-and-drill (see README):
//   * latch bars: clip both bars onto the car, dab marker on the bars' screw bosses, lower
//     the plate onto them, drill 3.2 mm at the marks, M3 screw from below + washer + nut on top.
//   * electronics: place each part inside its engraved outline, mark through its own holes,
//     drill (2.7 mm for M2.5, 3.2 mm for M3).
//
// All measured values come from docs/hardware.md ("Measured: body mounts and clearances").

part = "plate";      // "plate" | "corner_test_front" | "corner_test_rear" (small fit-check pieces)

$fn = 48;

// ---------------------------------------------------------------- measured chassis geometry
latch_x       = 157;   // front latch centers at +157, rear at -157 (314 mm apart)
latch_y_front = 60;    // front pair 120 mm apart
latch_y_rear  = 69;    // rear pair 138 mm apart (trapezoid, not a rectangle)
tire_gap      = 176;   // inside tire-to-tire (296 overall − 2 × 60 tire width)
antenna_xy    = [31, 50];   // receiver antenna tube, pokes through the plate
antenna_hole  = 16;
under_clear   = 22;    // free space under the plate over receiver/servo (135 − ~113)

// latch-bar screw holes (top face: 6.46 counterbore over a 3.06 through hole; the counterbores
// take the stock body pad's 6.30 posts). Offsets [dx, dy] from each latch-window center for the
// LEFT side (+dy = outward, toward the tire); the right side mirrors.
//   front: one ear toward the tire, one toward the REAR of the car (holes 23.40 apart c-c)
//   rear:  one ear forward, one rearward, both on the window's lengthwise center line
//          (far-to-far 40.79 − 3.06 = 37.73 c-c)
bar_holes_front = [[0, 13], [-19.5, 6]];     // from a photo + the 23.40 c-c; mark-and-drill
bar_holes_rear  = [[18.87, 0], [-18.87, 0]];
boss_r = 5.4;                                // ear boss radius (front 10.62, rear 10.8 at its widest)

// ---------------------------------------------------------------- plate
plate_t     = 3.5;
// The plate is NOT centered on the latches: the rear bar's holes reach ±18.5 fore/aft of the latch,
// the front bar's only reach backward, so the plate is shifted rearward.
// 171 + 183 = 354 mm long (Prusa XL bed ~360).
front_end   = 171;      // front edge at X = +171 (front latch frame ends ~+171)
rear_end    = 183;      // rear edge at X = −183
strip_inner = 125;      // the thickened end strips (which sit on the bars) span |X| = 125 … edge
end_half_w  = 88;       // end strips 176 wide = tire gap; tires are 29 mm BELOW the plate
mid_half_w  = 75;       // 150 wide between the strips
corner_r    = 8;

// the end strips are thickened on the underside (strength for the drilled bar screw holes)
pad_h     = 2;          // 5.5 mm total at the ends

// underside stiffening ribs (kept between the two strips so they never hit the bars)
rib_h = 6;              // < under_clear with lots of margin
rib_w = 3;
rib_x = 120;            // ribs run from −120 to +120
rib_y = [-62, 0, 62];   // two side ribs + one center rib

// ---------------------------------------------------------------- component layout (top side)
// [name, x, y, size_x, size_y]   size = footprint; engraved as a placement outline
parts = [
  ["LIDAR",  105,   0, 56, 56],    // front, nothing ahead of it on the plate
  ["IMU",     20,   0, 20, 27],    // near the middle, rigid, X axis forward
  ["NUCLEO", -35,   0, 70, 82.5],
  ["PI5",   -105,   0, 56, 85],    // rotated: long side across the car
  ["UBEC",    60, -45, 39, 21],
];
engrave_d = 0.4;   // outline depth
engrave_w = 1.0;   // outline line width
lidar_cable_slot = [14, 8];      // just behind the LiDAR

// zip-tie slot pairs [x, y, gap]: a tie loops through both slots
tie_slot  = [4, 2];
tie_pairs = [
  [ 60, -45, 14],    // UBEC
  [ 60,  45, 14],    // fuse holder
  [-70,  55,  6],    // harness / cable run
  [-70, -55,  6],
];

// ---------------------------------------------------------------- derived
latches = [[ latch_x,  latch_y_front], [ latch_x, -latch_y_front],
           [-latch_x,  latch_y_rear],  [-latch_x, -latch_y_rear]];

// absolute bar-hole positions, all four latches
function holes_at(lx, ly, offs) = [for (o = offs) [lx + o[0], ly + sign(ly) * o[1]]];
bar_holes = concat(
  holes_at( latch_x,  latch_y_front, bar_holes_front),
  holes_at( latch_x, -latch_y_front, bar_holes_front),
  holes_at(-latch_x,  latch_y_rear,  bar_holes_rear),
  holes_at(-latch_x, -latch_y_rear,  bar_holes_rear));

// ---------------------------------------------------------------- geometry
module rrect(x0, x1, hw) {   // rounded rectangle spanning x0..x1, ±hw
  translate([(x0 + x1)/2, 0]) offset(r = corner_r)
    square([x1 - x0 - 2*corner_r, 2*hw - 2*corner_r], center = true);
}
module end_strip2d(sgn) {
  if (sgn > 0) rrect(strip_inner, front_end, end_half_w);
  else         rrect(-rear_end, -strip_inner, end_half_w);
}
module outline2d() {
  union() {
    rrect(-rear_end + 5, front_end - 5, mid_half_w);
    end_strip2d(1);
    end_strip2d(-1);
  }
}

module frame_outline(sx, sy) {   // hollow rectangle for engraving
  difference() {
    square([sx, sy], center = true);
    square([sx - 2*engrave_w, sy - 2*engrave_w], center = true);
  }
}

module plate() {
  difference() {
    union() {
      linear_extrude(plate_t) outline2d();
      // thickened end strips on the UNDERSIDE (where the latch bars bolt on). All protrusions
      // are on the underside so the top face is flat: print TOP FACE DOWN, no supports.
      for (sgn = [1, -1]) translate([0, 0, -pad_h]) linear_extrude(pad_h) end_strip2d(sgn);
      // underside ribs
      for (y = rib_y)
        translate([-rib_x, y - rib_w/2, -rib_h]) cube([2*rib_x, rib_w, rib_h]);
    }
    // antenna pass-through
    translate([antenna_xy[0], antenna_xy[1], -rib_h - 1]) cylinder(d = antenna_hole, h = 50);
    // LiDAR cable slot (behind the LiDAR)
    let (l = parts[0])
      translate([l[1] - l[3]/2 - 2 - lidar_cable_slot[0], -lidar_cable_slot[1]/2, -rib_h - 1])
        cube([lidar_cable_slot[0], lidar_cable_slot[1], 50]);
    // zip-tie slots
    for (p = tie_pairs, s = [-1, 1])
      translate([p[0] - tie_slot[0]/2, p[1] + s*p[2]/2 - tie_slot[1]/2, -rib_h - 1])
        cube([tie_slot[0], tie_slot[1], 50]);
    // engraved placement outlines + labels
    for (c = parts) {
      translate([c[1], c[2], plate_t - engrave_d]) linear_extrude(engrave_d + 1) frame_outline(c[3], c[4]);
      translate([c[1], c[2], plate_t - engrave_d]) linear_extrude(engrave_d + 1)
        text(c[0], size = 5, halign = "center", valign = "center");
    }
    // latch-center markers: engraved crosshair + 2 mm sight hole at each latch-window center.
    // Line the sight hole up over the center of the bar's rectangular window to position the plate.
    for (l = latches) {
      translate([l[0], l[1], -rib_h - 1]) cylinder(d = 2, h = 50);
      translate([l[0], l[1], plate_t - engrave_d]) linear_extrude(engrave_d + 1) {
        square([16, engrave_w], center = true);
        square([engrave_w, 16], center = true);
      }
    }
    // FRONT arrow
    translate([front_end - 18, 0, plate_t - engrave_d]) linear_extrude(engrave_d + 1)
      polygon([[8, 0], [-4, 5], [-4, -5]]);
  }
}

// small fit-check pieces: the LEFT end strip around one latch. Print (any bed, ~20 min), line the
// sight hole up over the window center of the clipped-in bar, check both screw bosses land on it.
module corner_test(front) {
  lx = front ? latch_x : -latch_x;
  x0 = front ? strip_inner : -rear_end;
  x1 = front ? front_end : -strip_inner;
  intersection() {
    plate();
    translate([(x0 + x1)/2, 60, -10]) cube([x1 - x0 + 2, 60, 30], center = true);
  }
}

// ---------------------------------------------------------------- ghosts (preview only)
module ghosts() {
  // bar screw bosses (where the marker dots will land)
  for (h = bar_holes) %translate([h[0], h[1], -pad_h - 6]) cylinder(r = boss_r, h = 6);
  // tires (inside faces at ±tire_gap/2)
  for (x = [latch_x, -latch_x], s = [-1, 1])
    %translate([x - 53, s > 0 ? tire_gap/2 : -tire_gap/2 - 60, -29 - 106]) cube([106, 60, 106]);
  // antenna tube
  %translate([antenna_xy[0], antenna_xy[1], -50]) cylinder(d = 6, h = 50 + 50);
  // components
  for (c = parts) %translate([c[1] - c[3]/2, c[2] - c[4]/2, plate_t]) cube([c[3], c[4], 15]);
}

if (part == "plate")             { plate(); ghosts(); }
if (part == "corner_test_front") corner_test(true);
if (part == "corner_test_rear")  corner_test(false);

// ---------------------------------------------------------------- checks
echo(str("Plate size: ", front_end + rear_end, " x ", 2*end_half_w, " mm (XL bed ~360)"));
if (front_end + rear_end > 355) echo("WARNING: plate too long for a 360 mm bed with margin");
// every bar boss must sit fully on a thickened end strip
for (h = bar_holes) {
  ax = abs(h[0]);
  lo = strip_inner;  hi = h[0] > 0 ? front_end : rear_end;
  m  = min(ax - boss_r - lo, hi - (ax + boss_r), end_half_w - (abs(h[1]) + boss_r));
  echo(str("Bar hole ", h, ": boss margin ", m, " mm", m < 1 ? "  <-- TOO TIGHT" : ""));
}
