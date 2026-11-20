# ATLAS-R rover roadmap

This roadmap separates what is already in the repository from work that needs
hardware measurements or a Raspberry Pi. Do not treat planned features as
implemented.

## Current baseline

- ESP32 DOIT DevKit V1 firmware with open-loop PWM/direction control for four
  motor channels.
- Protected ESP32 Wi-Fi access point and hold-to-drive browser controls.
- USB-serial drive commands and battery telemetry.
- INA260 initialization interlock and a one-second command timeout.
- Wiring and power assumptions documented in [HARDWARE_SETUP.md](./HARDWARE_SETUP.md).
- Parametric chassis-plate starting model in [cad/](./cad/).

The firmware compiles, but it has not been hardware-tested. Verify motor-driver
logic compatibility, motor stall current, battery voltage, and the INA260
breakout ratings before powered tests.

## Before adding vision

1. **Verify the electrical system.** Identify the exact H-bridge modules,
   motors, battery, and INA260 breakout. Check stall current and all supply
   limits; add a correctly rated fuse, master switch, and physical motor-power
   disconnect.
2. **Prove manual drive safely.** Start with wheels raised, confirm motor
   polarity and STOP behavior, test loss of Wi-Fi/serial, and document measured
   current and voltage. The software STOP is not a hardware emergency stop.
3. **Add wheel feedback.** If the motors have encoders, read them with
   timestamped counts, measure wheel circumference and track width, and add
   bounded speed control only after the motor wiring and encoder voltage levels
   are known.
4. **Add sensing deliberately.** Choose an IMU and short-range obstacle sensor
   only after selecting exact parts and voltage interfaces. Add sensor
   validity/fault behavior before allowing autonomous motion.
5. **Add regression checks.** Test command parsing, output bounds, command
   timeout, sensor-init failure, and motor-stop behavior; record test results
   and hardware revisions.
6. **Finish the display integration.** The current repository only documents
   a separate UNO R4 display test. Choose whether the ESP32 or R4 owns the
   screen, verify voltage translation, and implement/test the inter-board
   protocol before connecting their signal pins.

## When a Raspberry Pi is available

- Keep the ESP32 responsible for motor outputs, wheel feedback, command
  bounds, and stopping on heartbeat loss.
- Put camera capture and vision on the Pi; start with a controlled, marked
  indoor test area rather than full SLAM.
- Link Pi-to-ESP32 over a documented serial or USB protocol with versioned
  messages, heartbeat, bounded commands, and explicit fault handling.
- Have the ESP32 stop motors independently if the Pi, vision process, or
  communication link stalls.
- Log command, sensor, battery, and timing data before expanding to waypoint
  navigation or inspection classification.

## Mechanical model status

The CAD model is a customizable plate concept only. Its wheel spacing,
mounting slots, and thickness are placeholders until the chassis, motor
brackets, battery, controllers, and display are measured. See
[cad/README.md](./cad/README.md) before printing.
