# Rover CAD starter

`rover_base.scad` is an original, parametric OpenSCAD starting point for a
four-wheel rover base plate. It is not copied from another repository.

## Open and adjust

1. Install OpenSCAD.
2. Open `rover_base.scad`.
3. Use Preview (F5) to inspect the plate and mounting slots.
4. Edit the parameters at the top of the file using millimeters.
5. Use Render (F6), inspect the result, then export STL if it fits your
   measured parts.

## Starter geometry

Default dimensions are **200 x 150 x 4 mm**. The plate includes:

- Four pairs of adjustable slots near the wheel positions for separate motor
  brackets.
- Four adjustable electronics mounting slots.
- Four strap slots for two hook-and-loop straps.
- Rounded plate corners.

The default wheelbase, track width, slot lengths, widths, and hole pattern are
illustrative placeholders. They were not measured from your motors, brackets,
ESP32, drivers, INA260, battery, or display. The plate does not include wheels,
motor brackets, enclosure walls, a screen bezel, or verified mounting holes.
Measure your parts and revise the parameters before printing; check clearances,
strength, fasteners, and print orientation for your material and printer.

OpenSCAD is a source model, not a proof of fit or strength. The screen and
Arduino R4 are intentionally not assigned physical mounting holes yet because
their exact board and connector dimensions have not been verified.
