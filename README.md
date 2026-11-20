# ESP32 Rover firmware

This PlatformIO project targets the `esp32doit-devkit-v1` and contains:

- Two-sided PWM/direction drive for four motor channels.
- INA260 battery voltage/current/power readings over I2C.
- A protected ESP32 Wi-Fi access point with a mobile browser drive page.
- USB-serial drive control as a fallback and periodic battery telemetry.

For the full assembly plan, motor-driver and INA260 wiring, display notes,
power precautions, and bring-up steps, see [HARDWARE_SETUP.md](./HARDWARE_SETUP.md).

No obstacle, distance, wheel-encoder, or other sensor hardware is declared in
the source, so no sensor pins or behavior are assumed.

## Planned future direction

Inspired by robotic NASA rovers, the original project description envisions
visual obstacle avoidance, a manipulator arm, multi-terrain capability, and
Raspberry Pi integration. Those features are not implemented by this firmware;
it provides local Wi-Fi browser control and supports the drive and
battery-monitor functions described below.

The MadHacks display is identified as the same 3.5-inch Raspberry Pi LCD, but
its exact model/interface and a safe GPIO/power plan are not established here.
Display wiring and code are deferred; no display pin assignments are made or
changed by this Wi-Fi step.

## Hardware assumptions — verify before wiring

The original motor signal names (`AIN1`, `AIN2`, `BIN1`, `BIN2`, `PWMA`,
`PWMB`, `STBY`) are consistent with two dual-channel, active-high-standby
drivers such as TB6612-compatible modules. The exact driver/module and motors
are not identified in the project. **Use this wiring only if both drivers'
datasheets confirm those signals, 3.3 V logic compatibility, and the proposed
shared standby connection.** Otherwise do not connect or enable the motors;
provide the exact driver model so its interface can be verified.

GPIOs 34 and 35 on the ESP32 are input-only and cannot control motor-driver
inputs. GPIO12 is a boot-strapping pin and can interfere with boot if the
attached driver pulls it high. Firmware therefore keeps the original
assignments except for those three signals: `LEFT_AIN2` is GPIO4,
`RIGHT_AIN1` is GPIO16, and `RIGHT_BIN2` is GPIO23. Both driver `STBY` pins
share GPIO17.

## Wiring

Disconnect motor power while wiring. GPIO numbers below mean ESP32 GPIO
numbers, not header positions.

### Left-side dual-channel driver

| Driver signal | ESP32 connection |
| --- | ---: |
| PWMA | GPIO13 |
| AIN1 | GPIO14 |
| AIN2 | GPIO4 |
| PWMB | GPIO18 |
| BIN1 | GPIO26 |
| BIN2 | GPIO27 |
| STBY | GPIO17 (shared; see below) |
| Logic VCC | ESP32 3V3, only if module datasheet permits |
| GND | Common ground |
| VM / motor supply | INA260 load-side positive (see battery wiring) |
| A01/A02 | One left-side motor |
| B01/B02 | The other left-side motor |

### Right-side dual-channel driver

| Driver signal | ESP32 connection |
| --- | ---: |
| PWMA | GPIO33 |
| AIN1 | GPIO16 |
| AIN2 | GPIO32 |
| PWMB | GPIO19 |
| BIN1 | GPIO25 |
| BIN2 | GPIO23 |
| STBY | GPIO17 (shared; see below) |
| Logic VCC | ESP32 3V3, only if module datasheet permits |
| GND | Common ground |
| VM / motor supply | INA260 load-side positive |
| A01/A02 | One right-side motor |
| B01/B02 | The other right-side motor |

Connect both `STBY` inputs together to GPIO17. Add an external pull-down from
the shared standby line to ground (for example 10 kΩ) so the drivers remain in
standby while the ESP32 is resetting; confirm the driver/module permits this.
The firmware holds standby low during setup and enables it only for nonzero
drive commands. The A/B channel-to-wheel positions and motor polarity are not
specified by the repository. With wheels lifted, test at low speed and correct
the motor terminal orientation only after confirming the driver documentation.

### INA260 battery monitor and bus

| INA260 signal | Connection |
| --- | --- |
| VCC/VIN (logic supply) | ESP32 3V3, if the breakout supports 3.3 V |
| GND | Common ground |
| SDA | ESP32 GPIO21 |
| SCL | ESP32 GPIO22 |
| VIN+ / source side | Battery positive |
| VIN- / load side | Both motor drivers' VM / motor-supply positive |

The INA260 must be in series with the positive battery feed to the motor
drivers for its readings to represent that feed. Confirm the specific INA260
breakout's terminal labels, voltage/current limits, polarity, and connector
ratings from its datasheet before connecting the battery. Do not exceed any
rating; this project cannot establish your battery voltage or motor stall
current. The monitor's measured bus should include the motor supply you intend
to monitor.

Connect battery negative, both driver grounds, INA260 ground, and ESP32 ground
together. Power the ESP32 through USB or a suitable regulated supply; do not
connect an unidentified rover battery directly to ESP32 3V3 or VIN. Motor
power must come from a supply appropriate for the motors and drivers, not from
the ESP32 3V3 pin. Keep motor-current paths and wiring within the ratings of
the battery, driver, sensor breakout, and conductors.

## Build, connect, and operate

Install PlatformIO and run `pio run` in this directory; upload with
`pio run -t upload`.

At startup the ESP32 creates its own protected Wi-Fi access point; it does not
join a router. Connect a phone or computer to SSID **Rover-ESP32** using the
WPA2 password printed to USB serial at **115200 baud**. The password is
generated for each boot and is not stored in the source. Keep the device
connected to this access point even if it reports that internet access is
unavailable, then open **http://192.168.4.1/** in a browser. Reconnect and use
the new serial-printed password after every reboot. If AP startup fails, USB
serial control remains available.

The page's direction buttons are hold-to-drive controls: movement commands
refresh every 250 ms, and releasing/canceling touch or pointer input, changing
browser focus, hiding the page, or navigating away sends a stop request. A
lost command link also stops the drive after one second. The explicit STOP
button sends a stop immediately. These are software failsafes, not a
safety-rated emergency stop.

Commands are newline-terminated:

- `D <left> <right>` — set left/right side speeds, each from `-255` through
  `255`; positive is the firmware's logical forward direction.
- `S` — stop and put both drivers in standby.

Example: `D 100 100` requests equal forward PWM. Web and serial commands share
the same motor-control lease; a command from either interface takes control,
and a stop from either interface stops the rover. Invalid web or serial drive
commands stop the motors; the firmware rejects drive commands if INA260
initialization fails. Battery telemetry is printed once per second as
`voltage` in volts, `current` in amperes, and `power` in watts. Monitor
readings, AP credentials, and serial commands share the serial port.

Before fitting wheels or applying motor power, verify wiring, driver identity
and ratings, battery/sensor ratings, grounds, and motor directions. The INA260
is used as an initialization interlock here, not as a configured low-battery
cutoff; no battery chemistry, cell count, or safe voltage thresholds are
available in the project.
