# Rover hardware setup and wiring

This guide covers the current ESP32 rover firmware and the 3.5-inch Inland
Raspberry Pi-style display shown in the MadHacks project. Disconnect motor
power while wiring. Do not use wire colors as a pinout; follow the signal
labels and header pin numbers.

## Recommended board arrangement

Use the DOIT ESP32 DevKit V1 as the rover's main controller. The current
firmware runs on it, drives the motors, reads the INA260, and creates the
`Rover-ESP32` Wi-Fi access point.

An Arduino UNO R4 is not a drop-in replacement for this firmware:

- The UNO R4 Minima has no built-in Wi-Fi.
- The UNO R4 WiFi has an RA4M1 main MCU and a separate ESP32-S3 connectivity
  coprocessor. The existing rover firmware uses ESP32 Arduino Wi-Fi APIs and
  would need to be ported to the UNO R4 environment. See the official
  [UNO R4 WiFi](https://docs.arduino.cc/hardware/uno-r4-wifi/) and
  [UNO R4 Minima](https://docs.arduino.cc/hardware/uno-r4-minima/) specs.
- Both UNO R4 variants use 5 V GPIO. The MadHacks display wiring guide
  specifies 3.3 V display logic, so a 5 V R4 signal must not be connected
  directly unless the exact display board is documented as 5 V tolerant.

The R4 could be used as a separate display controller, which would avoid
changing the ESP32 motor pin map. That requires a level-safe ESP32-to-R4
communication link and R4 display firmware; neither is implemented in this
repository yet. The full MadHacks sketch uses UNO Q-specific APIs and is not
drop-in R4 firmware. Its [TFT test sketch](https://github.com/ReeeVax/madhacks2026/blob/main/board/tft_test/tft_test.ino)
is a useful reference, but it has not been validated here on an R4.

Keeping the ESP32 for motor control and Wi-Fi is the simplest choice for the
current code. Adding the screen directly to that ESP32 would require a new
pin plan: the existing motor drivers use 13 GPIO signals, the INA260 uses
GPIO21/22, and the screen/touch interface needs additional SPI and control
signals. Do not connect the display to the current motor pins.

## Parts and assumptions

- DOIT ESP32 DevKit V1.
- Two dual-channel motor-driver modules with `PWMA`, `AIN1`, `AIN2`, `PWMB`,
  `BIN1`, `BIN2`, and active-high `STBY` inputs. The exact driver is unknown;
  verify 3.3 V logic compatibility, standby behavior, and current/voltage
  ratings in its datasheet.
- Four motors and a battery that meet the driver ratings. Their specifications
  are not identified in this project.
- INA260 breakout that supports the battery voltage and motor current.
- Inland 3.5-inch Raspberry Pi-style touchscreen. The MadHacks guide describes
  its display/touch controllers as ILI9486/XPT2046 at 480 x 320 with a 40-pin
  Raspberry Pi header. Verify your exact board and pin-1 mark before using
  those header numbers; see its
  [hardware guide](https://github.com/ReeeVax/madhacks2026/blob/main/board/WIRING.md).
- Arduino UNO R4 only if testing or later implementing a separate screen
  controller.
- Regulated supplies and wiring rated for the actual motor and screen loads.

## ESP32 and motor-driver wiring

The numbers in the ESP32 column are GPIO numbers, not ESP32 board header
positions.

### Left-side motor driver

| Driver signal | ESP32 connection |
| --- | --- |
| PWMA | GPIO13 |
| AIN1 | GPIO14 |
| AIN2 | GPIO4 |
| PWMB | GPIO18 |
| BIN1 | GPIO26 |
| BIN2 | GPIO27 |
| STBY | GPIO17 (shared by both drivers) |
| Logic VCC | ESP32 3V3 only if the driver permits it |
| GND | Common ground |
| VM / motor supply | INA260 load-side positive |
| A01/A02 | One left motor |
| B01/B02 | The other left motor |

### Right-side motor driver

| Driver signal | ESP32 connection |
| --- | --- |
| PWMA | GPIO33 |
| AIN1 | GPIO16 |
| AIN2 | GPIO32 |
| PWMB | GPIO19 |
| BIN1 | GPIO25 |
| BIN2 | GPIO23 |
| STBY | GPIO17 (shared by both drivers) |
| Logic VCC | ESP32 3V3 only if the driver permits it |
| GND | Common ground |
| VM / motor supply | INA260 load-side positive |
| A01/A02 | One right motor |
| B01/B02 | The other right motor |

Connect both `STBY` inputs together to GPIO17. The firmware holds standby low
at startup and enables it for a nonzero drive command. Install an external
pull-down (for example 10 kOhm) from the shared standby line to ground if the
driver datasheet permits it, so the drivers remain disabled while the ESP32
resets.

The motor channel-to-wheel mapping and polarity depend on the chassis. Raise
the wheels for the first test; use low command values and correct direction
only after checking the driver documentation.

## INA260 and battery wiring

| INA260 connection | Connect to |
| --- | --- |
| Logic VCC | ESP32 3V3 only if the breakout supports 3.3 V |
| GND | Common ground |
| SDA | ESP32 GPIO21 |
| SCL | ESP32 GPIO22 |
| VIN+ | Battery positive / source side |
| VIN- | Both motor drivers' VM / load side |

Place the INA260 in series with the positive battery feed to the motor
drivers: **battery positive -> VIN+; VIN- -> both driver VM terminals**.
Connect battery negative, both driver grounds, INA260 ground, and ESP32 ground
together.

Do not connect an unidentified battery directly to ESP32 3V3 or VIN. Power the
ESP32 from USB or a suitable regulated supply. Do not power motors from the
ESP32 3V3 pin. Check the INA260 breakout, battery, drivers, motors, connectors,
and wire ratings before applying motor power. The INA260 is only an
initialization interlock in this firmware; there is no low-battery cutoff.

## Display and Arduino UNO R4

The MadHacks hardware guide describes the 3.5-inch screen as a Raspberry
Pi-style 40-pin module. Pin numbers below are **screen header pin numbers**,
counted from the pin-1 mark on the display board. The MadHacks guide's signal
assignments are listed here with Arduino UNO R4 header pins; the R4's standard
SPI pins are D11/MOSI, D12/MISO, and D13/SCK.

| Screen header pin | Screen signal | UNO R4 pin |
| ---: | --- | --- |
| 1 | 3.3 V logic supply | 3V3, if the display current is within the board rating |
| 2 | 5 V backlight supply | 5V, if the USB/supply current budget permits |
| 6 | GND | GND |
| 19 | MOSI (data to display/touch) | D11 / MOSI |
| 21 | MISO (touch data to controller) | D12 / MISO |
| 23 | SPI clock | D13 / SCK |
| 24 | LCD chip select | D10 |
| 18 | LCD DC/RS | D9 |
| 22 | LCD reset | D8 |
| 26 | Touch chip select | D7 |
| 11 | Touch IRQ (optional) | D2 |

The screen and touch controller share SPI clock/data; they have separate chip
selects. The MadHacks guide's [screen wiring details](https://github.com/ReeeVax/madhacks2026/blob/main/board/WIRING.md#5-display--touch-wiring)
and [test sketch](https://github.com/ReeeVax/madhacks2026/blob/main/board/tft_test/tft_test.ino)
show the original setup.

**Voltage warning:** the UNO R4 GPIO operates at 5 V, while the MadHacks
display guide specifies 3.3 V logic. The photos do not establish whether the
display board has 5 V-tolerant level shifting. Before connecting an R4 signal,
verify the exact board's schematic/datasheet. If it is not explicitly
5 V-tolerant, put an SPI-capable 5 V-to-3.3 V level translator on the R4-to-
display signals and a suitable 3.3 V-to-5 V translator on MISO/IRQ. A common
slow I2C level-shifter board may not work reliably for SPI. Do not guess based
only on the board having several chips.

The wiring above only connects the screen to an R4 for a separate display
test. The current ESP32 rover firmware does not send data to the R4 or drive
the screen. Do not connect ESP32 and R4 UART pins together yet: a serial
protocol and safe 5 V/3.3 V level conversion must be implemented first. If you
want a single-controller screen, the motor GPIO assignment must be redesigned
and tested before wiring.

## Power and ground checklist

- Keep motor battery power disconnected while making signal connections.
- Power the ESP32 from USB or a suitable regulator.
- Power the Arduino R4 from its USB/regulator if it is being used for a screen
  test; do not connect the R4 5 V output to the ESP32 5 V output.
- Use the screen's 3.3 V logic and 5 V backlight supplies only within the
  ratings of the board and supply. Grounds must be common when boards exchange
  signals.
- Join the ESP32, motor-driver, INA260, and battery-negative grounds.
- If an R4 is later linked to the ESP32, join their grounds and level-shift
  signals as required.
- Keep the motor-current path separate from low-voltage signal wiring and
  within all component ratings.

## Bring-up and operation

1. With motor power disconnected and wheels raised, inspect every connection
   against the tables and the component pin labels.
2. Connect the ESP32 to USB and upload this repository's PlatformIO firmware.
   Open the USB serial monitor at 115200 baud.
3. Confirm the INA260 initializes. If it is missing, the firmware refuses
   drive commands.
4. Read the generated Wi-Fi password from serial, connect a phone/computer to
   SSID `Rover-ESP32`, stay connected despite any "no internet" warning, and
   open `http://192.168.4.1/`.
5. Test `STOP`, then briefly hold a direction button at low speed. The browser
   controls are hold-to-drive; the drive stops on release or after one second
   without a valid command. This is not a safety-rated emergency stop.
6. Only after the low-power test passes, reconnect motor power and repeat with
   the chassis restrained and the wheels clear.
7. Test the screen separately on the R4 only after confirming voltage
   compatibility. Screen/R4-to-ESP32 integration requires additional firmware
   not included in the current rover project.

USB-serial drive commands remain available: `D <left> <right>` with each side
from -255 to 255, or `S` to stop. Positive values are the firmware's logical
forward direction; actual wheel direction depends on wiring.
