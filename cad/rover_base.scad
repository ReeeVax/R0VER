// Parametric rover base plate. All dimensions are millimeters.
// Defaults are an unverified layout placeholder; measure hardware before printing.

$fn = 32;

plate_length = 200;
plate_width = 150;
plate_thickness = 4;
corner_radius = 10;

wheelbase = 145;
track_width = 100;
motor_slot_pair_spacing = 18;
motor_slot_length = 24;
motor_slot_width = 4.3;

electronics_pattern_x = 64;
electronics_pattern_y = 42;
electronics_slot_length = 22;
electronics_slot_width = 3.4;

strap_pattern_x = 104;
strap_pattern_y = 24;
strap_slot_length = 18;
strap_slot_width = 5.5;

cut_clearance = 0.15;

module rounded_plate() {
    linear_extrude(height = plate_thickness)
        offset(r = corner_radius)
            square([
                plate_length - 2 * corner_radius,
                plate_width - 2 * corner_radius
            ], center = true);
}

module slot_hole(length, width) {
    hull() {
        translate([-(length - width) / 2, 0, -cut_clearance])
            cylinder(h = plate_thickness + 2 * cut_clearance, d = width);
        translate([(length - width) / 2, 0, -cut_clearance])
            cylinder(h = plate_thickness + 2 * cut_clearance, d = width);
    }
}

difference() {
    rounded_plate();

    // Two adjustable bracket slots at each wheel position.
    for (x = [-wheelbase / 2, wheelbase / 2]) {
        for (side = [-1, 1]) {
            for (offset = [-motor_slot_pair_spacing / 2,
                            motor_slot_pair_spacing / 2]) {
                translate([x, side * track_width / 2 + offset, 0])
                    slot_hole(motor_slot_length, motor_slot_width);
            }
        }
    }

    // Generic mounting pattern for electronics or small standoffs.
    for (x = [-electronics_pattern_x / 2, electronics_pattern_x / 2]) {
        for (y = [-electronics_pattern_y / 2, electronics_pattern_y / 2]) {
            translate([x, y, 0])
                slot_hole(electronics_slot_length, electronics_slot_width);
        }
    }

    // Four slots for two adjustable hook-and-loop straps.
    for (x = [-strap_pattern_x / 2, strap_pattern_x / 2]) {
        for (y = [-strap_pattern_y / 2, strap_pattern_y / 2]) {
            translate([x, y, 0])
                rotate([0, 0, 90])
                    slot_hole(strap_slot_length, strap_slot_width);
        }
    }
}
