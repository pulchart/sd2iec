# sd2iec with LCD and OLED display support

sd2iec firmware with support for parallel HD44780 and ST7036 character LCDs and WS0010 OLED displays. The default branch `lcd` combines these repositories:

- [thierer/sd2iec](https://github.com/thierer/sd2iec): current unofficial sd2iec firmware, the base of this branch (`master` follows it)
- [SvOlli/sd2iec-lcd](https://github.com/SvOlli/sd2iec-lcd): LCD support, last updated in 2023 on an older sd2iec
- [markusC64/sd2iec](https://github.com/markusC64/sd2iec): fork of thierer/sd2iec with extension mode 5, attribute and lock commands, fast format and fixes
- [sd2iec.de](https://www.sd2iec.de): the original sd2iec by Ingo Korb, `git clone http://www.sd2iec.de/sd2iec.git`. Newer changes from its master are added to this branch

LCD code copied from SvOlli/sd2iec-lcd and adapted to current thierer/sd2iec, with small improvements and cleanup.

## What's new

All together:

### Display

- Parallel HD44780 and ST7036 LCDs and WS0010 OLED displays, the OLED dims after inactivity, `XOn` sets the time
- Scrolling of names and long error messages, the long file name of a mounted image, no flashing of the directory or an error when the image is changed with Next and Prev
- Line 2 shows statuses below 20 for a few seconds with the prefix `I:`: files scratched, selected partition, answers of the X commands

### Base

- FatFs updated to R0.16

### Boards and images

- Boards `petSD+` and `cbmSD-mini` (configs `petSD+` and `cbmSDmini`) with an on-board display and menu, the bus (IEC or IEEE-488) is selected at runtime
- D80 and D82 disk images (8050 and 8250) in all builds
- `N:name.dnp,nnn` creates a DNP image (CMD native partition with subdirectories) with nnn tracks, 001 to 255
- Partition listing filtered by image type, `$=P:*=N`

### Commands and names

- Extension mode 5 (`XE5`): any PETSCII name on FAT, written as `{HH}` escapes where FAT cannot store a character. File names with more than one dot, x00 names longer than 8 characters, directories starting with a dot are listed
- Commands `A:`, `L`/`EL`/`EU`, `EH`, `R-H` and `D:` for attributes, locking and hiding of files and the header, `EL:$` locks a whole disk image
- `XN+`/`XN-` fast format, `XET+`/`XET-` strict file type check, `XEL`/`XEU` lock of the extension settings, `XH+`/`XH-` hidden files in images
- `ET:name,t` changes the file type, `EC` converts a file to a temporary name and back to switch the extension mode of a directory tree, `XEB` marks mode 5 names with `{}` so the other modes read them (builds with `CONFIG_XEB`)

### Fastloaders

- New: ULoad Model 4 (Ultima IV Remastered, D64, D71 and D81 images), Another World and Wings of Fury (need a 1541 ROM image set with `XR`)
- New: GMA, the fastloader of the disk versions of Elite (tried with Elite 64, Elite 128 v2 and Elite Unbound), cracks with their own loader are not supported
- New: TCS, the fastloader of the TCS version of Elite 64 (`elite64tcs.d64`)
- ULoad Model 3 (Ultima 3 Gold): the D71 and D81 versions are detected, the display keeps scrolling while the loader waits
- A long key press leaves a fastloader on boards with a display, `M-R $FEA0` answers as a 1581 on D81 images

### Fixes

- `COPY` checks all sources before it creates the target, `N` refuses `*`, `?`, `=` and `"` in a name, wildcards in new names give 33
- REL record length 13, locked files in images, read-only images, DOS device names, names with shifted spaces, one-letter DNP directories, error codes
- An overwrite keeps the case of the typed name, GEOS and Wheels sector jobs clear the error first
- The software I2C no longer waits for ever for a slave that holds SCL, a device address outside 4 to 30 in the EEPROM is ignored, a build with only Hypra-Load works

## Display

Line 1 shows the directory or the mounted image, line 2 the loaded or saved file, the error message or `READY:` with the device address. Names longer than 14 characters scroll, a mounted image shows its long file name.

Statuses below 20 show on line 2 with the prefix `I:` for about 5 seconds, then `READY:` returns. `S` shows `I:SCRATCHED nn` (files scratched), `CP` shows `I:PARTITION nn`, the X commands that answer with their settings and `X?` show the text, a long text scrolls twice. A new text on line 2 (LOAD, SAVE, error) or closing the last channel brings `READY:` sooner. `00, OK` is not shown, `READY:` stays.

| | LCD (HD44780, ST7036) | OLED (WS0010) |
|---|---|---|
| Configs | `larsp-lcd`, `evo2-lcd` | `larsp-oled`, `evo2-oled` |
| Scroll step | 700 ms | 300 ms |
| Dimming | no | after 45 seconds without a change, `XOn` sets the time |

A dimmed OLED lights up on a change of content. The first press of Next, Prev or Home only wakes it, the next press acts. Pins: `doc/lcd-pinout.txt`.

## Commands

Sent on the command channel, for example `OPEN15,8,15,"N:TEST,01":CLOSE15`, or `@N:TEST,01` with JiffyDOS. "Dxx image" means D64, D71, D81 and DNP.

### DOS commands

| Command | Action |
|---|---|
| `$` | Directory.<br>`$=B` or `$=D` lists only directories.<br>`$=T` is the CMD style listing with time stamps and timestamp filters.<br>`*=H` includes hidden files, they are marked with an `H` after the lock mark: `PRG<H` or `PRG H`. |
| `$=P` | CMD style partition directory with name and type filters, `$=P:S*`.<br>All partitions are listed as `NAT`, mounted images with their actual type.<br>Type filters: `=N` NAT, `=4` D41, `=7` D71, `=8` D81. `$=P:*=8` lists all D81 images, `$=P:A*=N` the NAT partitions starting with A.<br>The SYSTEM line is listed only if the name filter matches it and no type filter is given.<br>`$!` lists the EEPROM file system, `$1` to `$9` the partitions 1 to 9. |
| `CD:name`, `CD:_` | Change directory, CMD syntax, drive and partition numbers are ignored.<br>`CD:_` or `CD_` goes up (`_` is the left arrow), `CD:foo` or `CD/foo` enters foo, `CD//foo` enters `\foo`, `CD/foo/:bar` or `CD/foo/bar` enters `foo\bar`.<br>Wildcards work anywhere in the path.<br>An image file as the last component, after a slash or a colon, is mounted, `CD:_` leaves it. Images are recognized by extension and size, a broken image is not detected. |
| `MD:name`, `RD:name` | `MD` creates the directory after the colon relative to the directories before it: `MD/foo/:bar` creates `bar` in `foo`, `MD//foo/:bar` creates `bar` in `\foo`.<br>`RD:foo` removes a subdirectory of the current directory only. |
| `CP`, `G-P` | `CP<num>` selects the partition, `num` is an ASCII number from 1 to 255, `C<Shift-P>` (`$42 $D0`) takes a binary number as the third character. Partition 0 means the selected partition, 1 by default. `!:` in place of a partition number addresses the EEPROM file system.<br>`G-P` returns the partition info as for CMD FD/HD, partly faked. |
| `N:name,id` | Formats the mounted image. A name with `*`, `?`, `=` or `"` returns `30,SYNTAX ERROR`. In a DNP image only in its root directory, otherwise ignored. An id followed by `+` or `-` turns fast format on or off for this command, see `XN+`.<br>Without a mounted image and without an existing file of that name, a name with the extension `.D64`, `.D41`, `.D71` or `.D81` creates an image of that type (`.D41` is a D64), the label is the name without the extension, the id is required. An existing file is formatted like a mounted image.<br>A name without a known extension gets `.D64`, an existing file of that name is not formatted and the command ends with `63,FILE EXISTS`.<br>Paths work: `N//SUBDIR:D64TEST.D64,XX` creates the image in `/SUBDIR`.<br>On the EEPROM file system `N` formats it, name and id are ignored. |
| `N:name.dnp,nnn` | Creates an empty DNP image with nnn tracks, 001 to 255, `N:TEST.DNP,100`. An existing file ends the command with `63,FILE EXISTS`. The image is not formatted, mount it and use `N` again. |
| `S:name,name` | Scratch. Wildcards work, directories are skipped. The number of names is limited by the command length only, 100 to 120 characters. |
| `R:new=old` | Rename, as on CMD drives, the error for an invalid character may differ. Names that FAT cannot store return `33,SYNTAX ERROR`. On FAT the case of a name can be changed. |
| `C:target=source,source` | Copy, CMD syntax `C[partition][path]:target=[[partition][path]:]source[,...]`.<br>Several sources are appended to one target. Every source is relative to the current directory. Wildcards work, only the first match is copied. All sources are checked before the target is created.<br>REL files work but are little tested, REL and non-REL files cannot be mixed in one append. |
| `A:R=foo`, `A:H=foo`, `A:=foo` | Set the attributes of a file and remove all others: `A:=foo` none, `A:R=foo` read-only, `A:H=foo` hidden, `A:RH=foo` both.<br>Works on FAT and in Dxx images. In Dxx images hidden counts only with `XH+`, without it setting hidden returns `31,SYNTAX ERROR`. Not on M2I images and not on the EEPROM file system. |
| `L:foo`, `EL:foo`, `EU:foo` | Read-only flag, a locked file cannot be scratched or overwritten.<br>`L:foo` toggles it on the first match, `EL:foo` sets it and `EU:foo` clears it on all matches. `XL` and `XU` are the same as `EL` and `EU`. Several names separated by commas, directories are skipped.<br>`EL:$` locks a mounted Dxx image, `EU:$` unlocks it. Writing to a locked image returns `26,WRITE PROTECT ON`. The lock is the DOS version byte of the root header, 1 to 63 counts as locked. `N` never creates a locked image. |
| `EH/:foo` | Toggles the hidden flag of the first match. The path must be present, otherwise it is a header change. In Dxx images needs `XH+`. |
| `ET:name,t` | Change the file type of the first match to `t`: `P`, `S` or `U`, anything else returns `31,SYNTAX ERROR`. A path is allowed.<br>In a Dxx image the type in the directory entry changes, the closed and locked flags stay.<br>On FAT the name is built for the new type in the current `XEn` mode. An x00 file stays an x00 file with the new extension, `FOO.P00` becomes `FOO.S00`.<br>Not on M2I images and not on the EEPROM file system, they return `31,SYNTAX ERROR`. |
| `EC:name`, `EC:name=TMPn` | Convert a file to a temporary name and back, for programs that convert a directory tree between extension modes, see [Converting between extension modes](#converting-between-extension-modes).<br>`EC:name` renames the first match to `TMPn`, `n` is the smallest free number, and reports the name in the error channel: `80,TMP1,0,0`. An x00 file loses its header, it is copied and the source is deleted. Returns `31,SYNTAX ERROR` in `XE1` and `XE2`, the temporary file would get a header again.<br>`EC:name=TMPn` renames `TMPn` back to `name` in the form the current mode requires. If that form is an x00 file (SEQ, USR and REL in `XE1`, every type in `XE2`), the file is copied with a new header and the source is deleted. `name` must not exist, `63,FILE EXISTS`. An x00 file as `TMPn` returns `31,SYNTAX ERROR`.<br>A directory is renamed, never copied. FAT only, Dxx images, M2I images and the EEPROM file system return `31,SYNTAX ERROR`. |
| `D:name,id`, `R-H:name` | Change the header name and id of a mounted Dxx image, in a DNP image of the current directory. The id is optional, two characters.<br>`D:foo,ba` sets name and id, `R-H:foo` only the name, `EH:foo,ba` and `XH:foo,ba` are the same as `D:`, `EH:` must not have a path.<br>Not on FAT, not on M2I images, not on the EEPROM file system. |
| `P` | Position in a REL file, or in a regular file on FAT: `"P"+chr$(channel)+chr$(lo)+chr$(midlo)+chr$(midhi)+chr$(hi)` seeks to the 0-based offset `hi*2^24+midhi*65536+256*midlo+lo`, missing bytes are zero.<br>The file then is in random access mode and is not closed at EOF. The IEC protocol cannot send an empty payload, so reads after EOF time out until the file is positioned again. |
| `U1`, `U2`, `B-R`, `B-W`, `B-P` | Block access while a Dxx image is mounted. `B-P` is not checked against the original ROM and takes a third parameter, the high byte of the position in a large buffer: `B-P 9 4 1` positions to byte 260 of channel 9. |
| `DI`, `DR`, `DW` | Direct sector access of the storage device, needs a large buffer of 512 bytes or more (`OPEN9,8,9,"##2"`), a smaller one returns `78,BUFFER TOO SMALL`. `bufchan` is the secondary address of the buffer.<br>`"DI"+chr$(bufchan)+chr$(device)+chr$(page)` reads info page 0 into the buffer: valid bytes of the structure, highest page (0), disk type (0 IDE, 2 SD), sector size divided by 256, number of sectors (4 bytes, little endian). A missing device gives `DRIVE NOT READY` and 0 valid bytes. Device numbers are not stable between releases, scan at least 0 to 7. Page 0 fits in a standard buffer.<br>`"DR"+chr$(bufchan)+chr$(device)+` sector as 4 bytes, little endian, reads a sector into the buffer, `DW` writes it. Errors: `20,READ ERROR`, `25,WRITE ERROR`, `26,WRITE PROTECT ON`. |
| `T-R`, `T-W` | Read or set the time, boards with an RTC. Without an RTC both return `30,SYNTAX ERROR`, with an RTC that is not set `T-R` returns `31,SYNTAX ERROR`.<br>The fourth character selects the format, `T-W` takes the new time right after it:<br>`A` ASCII, `SUN. 01/20/08 01:23:45 PM`+CR, day of week `SUN.`, `MON.`, `TUES`, `WED.`, `THUR`, `FRI.`, `SAT.`, year modulo 100.<br>`B` BCD and `D` decimal, 9 bytes: day of week (0 Sunday), year (BCD modulo 100, decimal minus 1900), month, day, hour 1 to 12, minute, second, AM/PM (0 AM), CR. A year below 80 is read as 20xx.<br>`I` ISO 8601 subset, `2008-01-20T13:23:45 SUN`+CR, `T-W` ignores everything after the seconds and calculates the day of week, the year has the century. |
| `M-R`, `M-W`, `M-E` | Memory access for drive detection and the fastloaders.<br>`M-R` without `XR` answers from a table of common detection addresses and sets most fastloaders to a compatible mode (1541 mode for Dreamload and ULoad Model 3, no fastloader for Action Replay 6), unknown addresses return arbitrary data. The mounted image type gives the drive type: 1541 for D41, 1571 for D71, 1581 for D81.<br>`M-W` to the address of the device number changes the address of sd2iec, and the written data are compared with the known fastloaders, so `M-E` runs the right emulation.<br>Programs that need to detect sd2iec must use `UI` and look for `sd2iec` or `uiec`, not `M-R`. |
| `U0>` + address | Changes the device address, `"U0>"+chr$(address)`. Other `U0` commands are not implemented. Saved with `XW`, used only while the hardware address of the board is the same as when it was saved. |
| `UI+`, `UI-` | Slightly faster bus protocol for the VC20 on or off, little tested. |
| `UI`, `UJ`, `U` + Shift-J | `UI` sets the `73,...` message. `UJ` closes all buffers and keeps the directory, mounted image and swap list. `U` + Shift-J (code 202) restarts the processor and skips the bootloader. |

### Extended commands

With JiffyDOS X commands need quotes, `@"XE5"`: `@X` is a JiffyDOS command (destination drive), unquoted the drive gets nothing. A change lasts until the next reset, `XW` saves the settings marked in the XW column to the EEPROM.

| Command | Action | XW | Default |
|---|---|---|---|
| `X` | Shows the current settings, the ones `XW` saves, in the error channel: `03,E01-:*+:ET-:I00:N-:H-:O45:R,08,00`.<br>Fixed order, a field ends with `:`: `E` extension mode and hiding flag (`<` in place of the `:` when `XEL` locked them), `*` 1581 style matching, `ET` strict file type, `EB` XEB write and read flag (`EB00-`, only with `CONFIG_XEB`), `I` image display, `N` fast format, `H` hidden flag in images, `O` OLED dim time (OLED builds only), `R` file name of `XR` (empty without one). Flags are `+` or `-`, numbers have two digits. The track is the device address, the sector 0. | | |
| `X?` | Extended version: version, processor type of the build and suffix of the config file, usually the short name of the board. | | |
| `XW` | Writes the settings to the EEPROM: the ones marked in the XW column, the device address and, on boards with an on-board display, the bus, the menu setting, brightness and contrast. With a device address changed by software, sd2iec starts with it unless the address jumpers differ from their setting at the time of the save. Hardware overrides software, so a board stays reachable when its saved address is taken by another device on the bus. | | |
| `XEn` | Extension mode, how files on FAT are written:<br>`0` never x00 files<br>`1` x00 for SEQ, USR and REL, not for PRG<br>`2` always x00<br>`3` file extension SEQ, USR, REL, no x00 header<br>`4` as 3, also for PRG<br>`5` as 4, characters FAT cannot store are escaped, only with `CONFIG_XE5`.<br>Modes 3 to 5 turn extension hiding on.<br>Mode 5 writes characters FAT cannot store as hex in curly braces: codes below 32 or above 95 (shifted letters too), `:/\*"<>?`, a leading `.`, a trailing `.` or space. A run of them shares one pair of braces, `A/B` is `A{2F}B`. Listings show the decoded name, in upper case if FAT stores lower case. Names are case sensitive, trailing shifted spaces are dropped. DOS device names (`CON`, `PRN`, `AUX`, `NUL`, `COM1-9`, `LPT1-9`) get an empty `{}` in front. A directory, or a file without type extension, whose name ends in `.PRG`, `.SEQ`, `.USR` or `.REL` gets an empty `{}` appended.<br>PRG files with the extension D64, D41, D71, D81, DNP or M2I are always written without x00 header and without `.PRG`. | yes | 1 |
| `XE+` / `XE-` | Hide the type extension of FAT files: `APPLICATION.PRG` becomes a PRG file `APPLICATION`, `README.SEQ` a SEQ file `README`. | yes | off |
| `XET+` / `XET-` | Refuse an explicit file type that differs from the type of an existing file: `"NAME,S"` on a PRG file returns `64,FILE TYPE MISMATCH`. | yes | off |
| `XEL` / `XEU` | Lock / unlock `XEn`, `XE+` and `XE-`, they are ignored without a message while locked. The lock shows as `<` after the extension hiding flag in `X`. | yes | unlocked |
| `XEB+` / `XEB-` | Only with `CONFIG_XEB`. In modes 0 to 4 a FAT name that contains `{` is read as a mode 5 name: the escapes are decoded and a type extension is evaluated and removed, with `XE+` off too. Other names are read as before. Modes 0 to 4 never write `{`. Mode 5 always decodes. Not locked by `XEL`. | yes | off |
| `XEB0` / `XEB1` | Only with `CONFIG_XEB`. In mode 5 `XEB1` puts an empty `{}` in front of every name that has no `{`, `FOO` is written as `{}FOO.PRG`, so `XEB+` recognizes every file written in mode 5. This includes directories. `XEB1` sets `XEB+` as well. Mode 5 reads names with and without `{}` the same way.<br>Files written before `XEB1` have no marker, `EC:name` followed by `EC:name=TMPn` adds it. Not locked by `XEL`. | yes | 0 |
| `XIn` | Disk images and M2I files in the directory: `0` with their normal type, `1` as directories (still readable as files), `2` both, twice. `1` or `2` helps software written for CMD drives that does not recognize images as mountable, but its `CD` may still fail to mount them. | yes | 0 |
| `XN+` / `XN-` | Fast format: only the directory track of an image is wiped by `N`. | yes | off |
| `XH+` / `XH-` | Honor the hidden flag (bit 5 of the file type) in Dxx images, ignored when off. | yes | off |
| `X*+` / `X*-` | 1581 style `*`: characters after a `*` are matched against the end of the name. When off they are ignored. | yes | on |
| `XDdrv=val` | Drive slots, only for boards with several drive types (the `uIEC` build, SD and ATA). `drv` is the slot 0 to 7, `val`: `0` master ATA, `1` slave ATA, `4` primary SD/MMC, `5` secondary SD/MMC, `6` reserved, `15` no device. Only devices of the board can be set, a device cannot be in two slots. Reordering ATA devices is discouraged, use the master/slave jumpers. Setting all slots to `15` resets the configuration.<br>`XD?` shows it, `03,D:00=04:01=00:02=01,10,01`: track is the device address, sector is status information. | yes | board default |
| `XR:name` / `XR` | File for `M-R` emulation / disable it. A ROM image of a 1541, 1571 or 1581, headers are skipped, name up to 16 characters. It is searched in the current directory, in the root of the current partition and in the root of the first partition. Answers `M-R` from `$8000` to `$FFFF`, the internal table is used if the file is missing or unreadable. Needed where detection reads large parts of the ROM (GEOS), lowers fastloader compatibility, most fastloaders accept only the 1541 variant. | yes | none |
| `XS:name` / `XS` | Set up a swap list / disable it. Text file with one image or directory per line (anything `CD` accepts), `;` starts a comment, empty lines are ignored, ASCII, or PETSCII if the first line is `#PETSCII`. The NEXT and PREV buttons walk through it and wrap around, both together go to the first entry (HOME). Without an active list `AUTOSWAP.LST` in the current directory is used and drops out on `CD`. HOME without it generates `AUTOSWAP.GEN` with all images of the directory, rename it to `AUTOSWAP.LST` to keep it. | | none |
| `XT1text`, `XT2text` | Show `text` on line 1 or 2 of the LCD, the line is cleared first. Builds with `CONFIG_LCD_DISPLAY`. | | |
| `XTC` + command byte + data | Send a command byte and data bytes to the display controller, `OPEN15,8,15,"XTC"+CHR$(1)` clears the display. | | |
| `XA` | Show the version and the author of the LCD code. | | |
| `XG` | Show the credits. | | |
| `XCnn` | Contrast 0 to 63, ST7036 only. | | |
| `XOn` | Seconds until the WS0010 OLED dims, 1 to 254, `0` always off, `255` never dims, OLED builds only. | yes | 45 |

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

### Converting between extension modes

Files keep their FAT names when `XEn` changes, so files written in mode 5 do not show their PETSCII names in mode 2. A program on the C64 converts a directory tree with `EC`: every file gets a temporary name in a mode that reads it correctly, the mode is switched, and the file gets its name back in the form of the target mode. `TMPn` looks the same in all modes.

For one directory:

1. Switch to the source mode: `XE5` for files written in mode 5, `XE4` for files written in modes 0 to 4, it reads x00 files and type extensions. `EC:name` does not work in `XE1` and `XE2`.
2. Read the directory including hidden files (`*=H`), note the subdirectories and convert each one the same way, `CD` into it and `CD:_` back.
3. Read the directory again and send `EC:name` for every file and subdirectory. Note the temporary name from the error channel, `80,TMP1,0,0`, with the name.
4. Switch to the target mode. For modes 0 to 2 turn `XE+` on so the temporary files are found with their type, and restore it afterwards. After mode 5 with `XEB1` send `XEB+` as well, the temporary names were written as `{}TMPn`.
5. Send `EC:name=TMPn` for every noted pair. Switch back to the source mode before the parent directory continues.

Subdirectories are converted after their contents, so the names for `CD` stay valid. Disk images are files, their content is not changed.

### Emergency start

Hold NEXT while powering on to skip the settings saved in the EEPROM. The firmware starts with the defaults and takes the device address from the board. The EEPROM keeps its content until `XW` overwrites it. The first button change after the start is ignored, so releasing NEXT does not switch the disk image.

## Builds

The builds are for the ATmega1284P (128 KB flash), `uIEC` and `uIEC3` for the ATmega1281.

The `uIEC` build mounts 15 partitions at most, the other builds keep the number of their config.

`CONFIG_XEB` is on in the configs `uIEC`, `uIEC3`, `larsp`, `sw1`, `sw2`, `petSD`, `XS-1541`, `arm2iec1`, `mbed` and `example`. `ET` and `EC` are in every build.

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
  -U flash:w:dist/sd2iec-1.208.1-jpu-m1284p-larsp-oled.bin:r \
  -U efuse:w:0xFD:m \
  -U hfuse:w:0xD2:m \
  -U lfuse:w:0xE7:m
```

Fuses already set, firmware only:

```
avrdude -c usbasp -P usb -p m1284p \
  -U flash:w:dist/sd2iec-1.208.1-jpu-m1284p-larsp-oled.bin:r
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
  -U flash:w:dist/sd2iec-1.208.1-jpu-m1284p-larsp-oled.bin:r
```

## Build

Needs `avr-gcc` and `avr-libc`. One config:

```
make CONFIG=configs/config-larsp-oled
```

The binary is `obj-m1284p-larsp-oled/sd2iec.bin`. All configs, into `dist/` as `sd2iec-<version>-<mcu>-<config>.bin`, for example `sd2iec-1.208.1-jpu-m1284p-larsp-oled.bin`:

```
make dist
```

`DISTCONFIGS="larsp-lcd evo2-lcd"` builds only some configs.

`make check-size` builds everything and fails when a build outgrows flash or leaves too little RAM for the stack. Limits: `scripts/check-size.py`.

## Version

The version format is `<major>.<build>[.<revision>]-JPU[+LCD|+OLED]`, for example `1.208.1-JPU`, `1.208.1-JPU+LCD` and `1.208.1-JPU+OLED`.

| Part | Meaning |
|---|---|
| `<major>` | 1 |
| `<build>` | build number of the upstream tag, `v1.0.0atentdead0-208-g799296af` gives 208 |
| `<revision>` | revision, left out when there is none |
| `-JPU` | this fork |
| `+LCD` | builds with LCD support |
| `+OLED` | builds with WS0010 OLED support |

| Where | Example |
|---|---|
| Error channel (`UI`) | `SD2IEC V1.208.1-JPU+OLED` |
| LCD line 1 | `SD2IEC 1.208.1` |
| File name | `sd2iec-1.208.1-jpu-m1284p-larsp-oled.bin` |

The version is set in `version.mk`. The boot loader flashes every build.

## Credits

LCD code by Draco, Seanser, Sascha Bader, andi6510, CapFuture1975 and Sven Oliver Moll, based on the LCD library by Peter Fleury. sd2iec by Ingo Korb and contributors, see `README`. `ET`, `EC` and `XEB` by MarcusC64.

FAT file system: [FatFs](https://elm-chan.org/fsw/ff/) by ChaN, adapted for sd2iec (`src/fatfs`).

Free software under GPL version 2 only, see `COPYING`.
