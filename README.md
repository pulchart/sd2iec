# sd2iec with LCD display support

sd2iec firmware with support for parallel HD44780 and ST7036 character LCDs. The default branch `lcd` combines these repositories:

- [thierer/sd2iec](https://github.com/thierer/sd2iec): current unofficial sd2iec firmware, the base of this branch (`master` follows it)
- [SvOlli/sd2iec-lcd](https://github.com/SvOlli/sd2iec-lcd): LCD support, last updated in 2023 on an older sd2iec
- [sd2iec.de](https://www.sd2iec.de): the original sd2iec by Ingo Korb, `git clone http://www.sd2iec.de/sd2iec.git`. Newer changes from its master are added to this branch

LCD code copied from SvOlli/sd2iec-lcd and adapted to current thierer/sd2iec, with small improvements and cleanup.

## What is added

- LCD driver and display layer: `src/lcd.c`, `src/display_lcd.c`
- Display shows the version, current directory, loaded and saved names, error channel and the device address. Long names scroll while the bus is idle
- X-commands for text, contrast and credits, see `README`
- Configs `larsp-lcd` (MMC2IEC) and `evo2-lcd` (evo2), plus `evo2` as hardware variant 11
- Display pins per board: `doc/lcd-pinout.txt`

## Status

- Tested on hardware: `larsp-lcd`
- Not tested on hardware: `evo2`, `evo2-lcd`, sw1 and sw2 pin tables

## Programming

ATmega1284P with a USBasp programmer, `larsp-lcd` as the example. Fuses (from `scripts/avr/variables.mk`): efuse 0xFD, hfuse 0xD2, lfuse 0xE7. Check them with:

```
avrdude -c usbasp -P usb -p m1284p \
  -U lfuse:r:-:h \
  -U hfuse:r:-:h \
  -U efuse:r:-:h
```

### Standalone firmware

Every update needs the programmer. avrdude erases the whole chip.

```
avrdude -c usbasp -P usb -p m1284p \
  -U flash:w:dist/sd2iec-1.186.1-jpu-m1284p-larsp-lcd.bin:r \
  -U efuse:w:0xFD:m \
  -U hfuse:w:0xD2:m \
  -U lfuse:w:0xE7:m
```

Fuses already set, firmware only:

```
avrdude -c usbasp -P usb -p m1284p \
  -U flash:w:dist/sd2iec-1.186.1-jpu-m1284p-larsp-lcd.bin:r
```

### With bootloader

1. Program the bootloader once, together with the fuses. `make dist-bootloader` downloads it from [sd2iec.de](https://sd2iec.de/bootloader/) into `dist/`. It exists for larsp, sw1 and sw2.

```
avrdude -c usbasp -P usb -p m1284p \
  -U flash:w:dist/newboot-0.4.1-larsp-m1284p.hex:i \
  -U efuse:w:0xFD:m \
  -U hfuse:w:0xD2:m \
  -U lfuse:w:0xE7:m
```

2. Copy the `.bin` to the root of the SD card and power on. The file name does not matter. Builds of this fork are development versions, the bootloader flashes them whenever the file differs from the chip.
3. Updates: copy the new `.bin` to the card.

To write the firmware with the programmer and keep the bootloader, add `-D`:

```
avrdude -c usbasp -P usb -p m1284p -D \
  -U flash:w:dist/sd2iec-1.186.1-jpu-m1284p-larsp-lcd.bin:r
```

## Build

Needs `avr-gcc` and `avr-libc`. One config:

```
make CONFIG=configs/config-larsp-lcd
```

The binary is `obj-m1284p-larsp-lcd/sd2iec.bin`. All configs, into `dist/` as `sd2iec-<version>-<mcu>-<config>.bin`, for example `sd2iec-1.186.1-jpu-m1284p-larsp-lcd.bin`:

```
make dist
```

`DISTCONFIGS="larsp-lcd evo2-lcd"` builds only some configs.

With GCC 15 both stop on `-Wunterminated-string-initialization` in unmodified upstream files. Add `CSTANDARD` to the command:

```
make dist CSTANDARD="-std=gnu99 -Wno-error=unterminated-string-initialization"
```

## Version

The version format is `<major>.<build>[.<revision>]-JPU[+LCD]`, for example `1.186.1-JPU` and `1.186.1-JPU+LCD`.

| Part | Meaning |
|---|---|
| `<major>` | 1 |
| `<build>` | build number of the upstream tag, `v1.0.0atentdead0-186-g069555f1` gives 186 |
| `<revision>` | revision, left out when there is none |
| `-JPU` | this fork |
| `+LCD` | builds with LCD support |

| Where | Example |
|---|---|
| Error channel (`UI`) | `SD2IEC V1.186.1-JPU+LCD` |
| LCD line 1 | `SD2IEC 1.186.1` |
| File name | `sd2iec-1.186.1-jpu-m1284p-larsp-lcd.bin` |

The version is set in `version.mk`. The boot loader flashes every build.

## Credits

LCD code by Draco, Seanser, Sascha Bader, andi6510, CapFuture1975 and Sven Oliver Moll, based on the LCD library by Peter Fleury. sd2iec by Ingo Korb and contributors, see `README`.

Free software under GPL version 2 only, see `COPYING`.
