# sd2iec with LCD and OLED display support

sd2iec firmware with support for parallel HD44780 and ST7036 character LCDs and WS0010 OLED displays. The default branch `lcd` combines these repositories:

- [thierer/sd2iec](https://github.com/thierer/sd2iec): current unofficial sd2iec firmware, the base of this branch (`master` follows it)
- [SvOlli/sd2iec-lcd](https://github.com/SvOlli/sd2iec-lcd): LCD support, last updated in 2023 on an older sd2iec
- [markusC64/sd2iec](https://github.com/markusC64/sd2iec): fork of thierer/sd2iec with extension mode 5, attribute and lock commands, fast format and fixes
- [sd2iec.de](https://www.sd2iec.de): the original sd2iec by Ingo Korb, `git clone http://www.sd2iec.de/sd2iec.git`. Newer changes from its master are added to this branch

LCD code copied from SvOlli/sd2iec-lcd and adapted to current thierer/sd2iec, with small improvements and cleanup.

## What is added

- Support for WS0010 OLED displays: configs `larsp-oled` (MMC2IEC) and `evo2-oled` (evo2) for a module such as the Winstar WEH001602A, dimming after inactivity
- Scrolling of names longer than the display, the long file name of a mounted image
- Extension mode 5 (`XE5`): any PETSCII name on FAT, written as `{HH}` escapes where FAT cannot store a character. File names with more than one dot
- Commands `A:`, `L`/`EL`/`EU`, `EH`, `R-H` and `D:` for attributes, locking and hiding of files and the header, `EL:$` locks a whole disk image
- `XN+`/`XN-` fast format, `XET+`/`XET-` strict file type check, `XEL`/`XEU` lock of the extension settings, `XH+`/`XH-` hidden files in images
- Partition listing filtered by image type, `$=P:*=N`
- `N:name.dnp,nnn` creates a DNP image (CMD native partition with subdirectories) with nnn tracks, 001 to 255
- Fixes for `COPY` errors, REL record length 13, locked files in images, read-only images, DOS device names, names with shifted spaces, one-letter DNP directories and error codes
- Boards `petSD+` and `cbmSD-mini` (configs `petSD+` and `cbmSDmini`) with an on-board display and menu, the bus (IEC or IEEE-488) is selected at runtime
- D80 and D82 disk images (8050 and 8250) in all builds
- Fastloaders on `petSD+` and `cbmSD-mini`, and the loaders of Another World, Wings of Fury (needs a 1541 ROM image set with `XR`) and Ultima IV Remastered (D64, D71, D81 images)

## Display

Line 1 shows the directory or the mounted image, line 2 the loaded or saved file, the error message or `READY:` with the device address. Names longer than 14 characters scroll, a mounted image shows its long file name.

| | LCD (HD44780, ST7036) | OLED (WS0010) |
|---|---|---|
| Configs | `larsp-lcd`, `evo2-lcd` | `larsp-oled`, `evo2-oled` |
| Scroll step | 700 ms | 300 ms |
| Dimming | no | after 45 seconds without a change, `XOn` sets the time |

A dimmed OLED lights up on a change of content. The first press of Next, Prev or Home only wakes it, the next press acts. Pins: `doc/lcd-pinout.txt`.

## Commands

Sent on the command channel, for example `OPEN15,8,15,"N:TEST,01":CLOSE15`, or `@"N:TEST,01"` with JiffyDOS. A command that is not listed is not supported, `README` has the details.

### DOS commands

| Command | Action |
|---|---|
| `$` | directory, `$=B` or `$=D` lists only directories, the filter `*=H` includes hidden files, `$=T` is the CMD style listing with time stamps |
| `$=P` | partition directory with name and image type filters, `$=P:*=8` lists the D81 images, `$=P:A*=N` the NAT partitions starting with A (`=N` NAT, `=4` D41, `=7` D71, `=8` D81) |
| `CD:name`, `CD:_` | change directory, `_` (left arrow) goes up, an image file as the last component is mounted and `CD:_` leaves it |
| `MD:name`, `RD:name` | make or remove a directory, `MD/foo/:bar` creates `bar` in `foo`, `RD` works in the current directory only |
| `CP`, `G-P` | change the partition, partition info |
| `N:name,id` | format the mounted image, without an image `N:NAME.D64,ID` creates a D64, D41, D71 or D81 image (`.D64` if no extension), an id followed by `+` or `-` turns fast format on or off for this command |
| `N:name.dnp,nnn` | create an empty DNP image with nnn tracks, 001 to 255, mount it and use `N` again to format it |
| `S:name,name` | scratch files |
| `R:new=old` | rename, on FAT the case of a name can be changed |
| `C:target=source,source` | copy, several sources are appended to the target |
| `A:R=foo`, `A:H=foo`, `A:=foo` | set read-only, hidden or no attributes of a file |
| `L:foo`, `EL:foo`, `EU:foo` | toggle, set or clear the read-only flag, `EL:$` and `EU:$` lock and unlock a mounted image, `XL` and `XU` are the same as `EL` and `EU` |
| `EH/:foo` | toggle the hidden flag of a file |
| `D:name,id`, `R-H:name` | change the header name and id of a mounted image, `EH:name,id` and `XH:name,id` are the same as `D:` |
| `P` | position in a REL file or in a regular file |
| `U1`, `U2`, `B-R`, `B-W`, `B-P` | block access while a D64 image is mounted |
| `DI`, `DR`, `DW` | direct sector access, needs a buffer of 512 bytes or more |
| `T-R`, `T-W` | read or set the time, boards with an RTC |
| `M-R`, `M-W`, `M-E` | memory access for drive detection and the fastloaders, `XR` sets a ROM file |
| `U0>` + address | device address, saved with `XW`, used only while the hardware address of the board is the same as when it was saved |
| `UI+`, `UI-` | faster bus protocol for the VC20 |
| `UI`, `UJ`, `U` + Shift-J | soft reset, close all buffers, restart of the processor |

### Extended commands

X commands need the quotes with JiffyDOS: `@"XE5"`. A change lasts until the next reset. `XW` writes the settings marked in the XW column to the EEPROM and the firmware reads them at every start.

| Command | Action | XW | Default |
|---|---|---|---|
| `X` | show the current settings in the error channel | | |
| `X?` | extended version: version, processor and suffix of the config | | |
| `XW` | write the settings to the EEPROM | | |
| `XEn` | extension mode, see below | yes | 1 |
| `XE+` / `XE-` | hide the type extension of FAT files, modes 3 to 5 turn it on | yes | off |
| `XET+` / `XET-` | refuse an explicit file type that differs from the type of the file | yes | off |
| `XEL` / `XEU` | lock / unlock `XEn`, `XE+` and `XE-` | yes | unlocked |
| `XIn` | disk images in the directory: 0 with their normal type, 1 as directories, 2 both | yes | 0 |
| `XN+` / `XN-` | fast format | yes | off |
| `XH+` / `XH-` | honor the hidden flag of files in images | yes | off |
| `X*+` / `X*-` | 1581 style `*` matching | yes | on |
| `XDdrv=val` | drive slots, only in the `uIEC` build (SD and ATA) | yes | board default |
| `XR:name` / `XR` | file for M-R emulation / disable it | yes | none |
| `XS:name` / `XS` | set up a swap list / disable it | | none |
| `XT1text`, `XT2text` | show `text` on line 1 or 2 of the display | | |
| `XTC` + command byte + data | send a command byte and data bytes to the display controller | | |
| `XA` | show the version and the author of the LCD code | | |
| `XG` | show the credits | | |
| `XCnn` | set the contrast 0 to 63, ST7036 only | | |
| `XOn` | seconds until the OLED dims, 1 to 254, 0 always off, 255 always on, OLED builds only | yes | 45 |

## Configuration

### File names and extension modes

FAT has no file type and does not take every C64 name. `XEn` selects how a file is stored.

| Mode | File on the card |
|---|---|
| 0 | plain file, no header, no type |
| 1 (default) | PRG plain, SEQ, USR and REL as x00: `NAME.S00`, `NAME.U00`, `NAME.R00` |
| 2 | every type as x00, PRG as `NAME.P00` |
| 3 | SEQ, USR and REL as `NAME.SEQ`, `NAME.USR`, `NAME.REL`, PRG plain |
| 4 | like 3, PRG as `NAME.PRG` |
| 5 | like 4, characters FAT cannot hold are escaped |

An x00 file keeps the type and the C64 name in a header. With `XE+` the directory takes the type from a `.SEQ`, `.USR`, `.REL` or `.PRG` extension and hides it, modes 3 to 5 turn `XE+` on. Without it every file with another extension, or without one, lists as PRG.

In modes 0 to 4 a name with a character other than letters, digits, space and `! # $ % & ' ( ) - .` is stored as x00, whatever the mode says. The file name has `_` for every character that is not accepted, the C64 name is in the header. `A/B` as PRG becomes `A_B.P00`.

Mode 5 writes these characters as the PETSCII code in hex between braces: codes below 32 and from 96 up, `: / \ * " < > ?`, a dot at the start and a dot or space at the end. Following characters of this kind share one pair of braces.

| C64 name | File on the card |
|---|---|
| `A/B` | `A{2F}B` |
| `GAME*` | `GAME{2A}` |
| shifted A, code `C1` | `{C1}` |
| `CON` | `{}CON` |
| directory `DIR.PRG` | `DIR.PRG{}` |

`CON`, `PRN`, `AUX`, `NUL`, `COM1` to `COM9` and `LPT1` to `LPT9` get `{}` in front. A directory, or a file stored without type suffix such as a disk image, whose name ends in `.PRG`, `.SEQ`, `.USR` or `.REL` gets `{}` appended. The directory shows the decoded name, in mode 5 names are matched with case, trailing shifted spaces are dropped.

### Emergency start

Hold NEXT while powering on to skip the settings saved in the EEPROM. The firmware starts with the defaults and takes the device address from the board. The EEPROM keeps its content until `XW` overwrites it. The first button change after the start is ignored, so releasing NEXT does not switch the disk image.

## Builds

The builds are for the ATmega1284P (128 KB flash), `uIEC` and `uIEC3` for the ATmega1281.

The `uIEC` build mounts 15 partitions at most, the other builds keep the number of their config.

The boards `petSD+` and `cbmSD-mini` have an ATmega1284P that runs at 8 MHz on the IEC bus and at 16 MHz on the IEEE-488 bus. A menu on the on-board display selects the bus, the device address, the disk image and the settings. All fastloaders are built except the burst and parallel ones, which need an SRQ interrupt and a parallel port these boards do not have. A long key press leaves a running loader.

## Programming

ATmega1284P with a USBasp programmer, `larsp-oled` as the example. Fuses (from `scripts/avr/variables.mk`): efuse 0xFD, hfuse 0xD2, lfuse 0xE7. Check them with:

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
  -U flash:w:dist/sd2iec-1.186.5-jpu-m1284p-larsp-oled.bin:r \
  -U efuse:w:0xFD:m \
  -U hfuse:w:0xD2:m \
  -U lfuse:w:0xE7:m
```

Fuses already set, firmware only:

```
avrdude -c usbasp -P usb -p m1284p \
  -U flash:w:dist/sd2iec-1.186.5-jpu-m1284p-larsp-oled.bin:r
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
  -U flash:w:dist/sd2iec-1.186.5-jpu-m1284p-larsp-oled.bin:r
```

## Build

Needs `avr-gcc` and `avr-libc`. One config:

```
make CONFIG=configs/config-larsp-oled
```

The binary is `obj-m1284p-larsp-oled/sd2iec.bin`. All configs, into `dist/` as `sd2iec-<version>-<mcu>-<config>.bin`, for example `sd2iec-1.186.5-jpu-m1284p-larsp-oled.bin`:

```
make dist
```

`DISTCONFIGS="larsp-lcd evo2-lcd"` builds only some configs.

`make check-size` builds everything and fails when a build outgrows flash or leaves too little RAM for the stack. Limits: `scripts/check-size.py`.

## Version

The version format is `<major>.<build>[.<revision>]-JPU[+LCD|+OLED]`, for example `1.186.5-JPU`, `1.186.5-JPU+LCD` and `1.186.5-JPU+OLED`.

| Part | Meaning |
|---|---|
| `<major>` | 1 |
| `<build>` | build number of the upstream tag, `v1.0.0atentdead0-186-g069555f1` gives 186 |
| `<revision>` | revision, left out when there is none |
| `-JPU` | this fork |
| `+LCD` | builds with LCD support |
| `+OLED` | builds with WS0010 OLED support |

| Where | Example |
|---|---|
| Error channel (`UI`) | `SD2IEC V1.186.5-JPU+OLED` |
| LCD line 1 | `SD2IEC 1.186.5` |
| File name | `sd2iec-1.186.5-jpu-m1284p-larsp-oled.bin` |

The version is set in `version.mk`. The boot loader flashes every build.

## Credits

LCD code by Draco, Seanser, Sascha Bader, andi6510, CapFuture1975 and Sven Oliver Moll, based on the LCD library by Peter Fleury. sd2iec by Ingo Korb and contributors, see `README`.

FAT file system: [FatFs](https://elm-chan.org/fsw/ff/) by ChaN, adapted for sd2iec (`src/fatfs`).

Free software under GPL version 2 only, see `COPYING`.
