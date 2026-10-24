#!/usr/bin/env python3
"""Check that the sd2iec builds stay inside their flash and RAM limits.

Usage: check-size.py obj-<suffix> [obj-<suffix> ...]
"""

import argparse
import os
import re
import subprocess
import sys

# Lowering any of these limits must be a deliberate decision.
# flash_margin: minimum free flash in bytes (BINARY_LENGTH - Program)
# ram_free: minimum free RAM in bytes (RAM size - RAM use), the room left for the stack
LIMITS = {
    "atmega644p":  {"flash_margin": 128, "ram_free": 688},
    "atmega1284p": {"flash_margin": 128, "ram_free": 896},
    "atmega1281":  {"flash_margin": 128, "ram_free": 1248},
}

# internal RAM size in bytes
RAM_SIZE = {
    "atmega644p": 4096,
    "atmega1284p": 16384,
    "atmega1281": 8192,
}

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VARIABLES_MK = os.path.join(ROOT, "scripts", "avr", "variables.mk")


def flash_limits():
    """Return {mcu: BINARY_LENGTH} parsed from scripts/avr/variables.mk."""
    result = {}
    mcu = None
    with open(VARIABLES_MK) as f:
        for line in f:
            m = re.match(r"\s*(?:else\s+)?ifeq\s*\(\$\(MCU\),\s*(\w+)\)", line)
            if m:
                mcu = m.group(1)
                continue
            m = re.match(r"\s*BINARY_LENGTH\s*=\s*(0x[0-9a-fA-F]+|\d+)", line)
            if m and mcu:
                result[mcu] = int(m.group(1), 0)
    return result


def build_mcu(objdir):
    """Return CONFIG_MCU of a build from its make.inc."""
    with open(os.path.join(objdir, "make.inc")) as f:
        for line in f:
            m = re.match(r"CONFIG_MCU\s*=\s*(\w+)", line)
            if m:
                return m.group(1)
    raise SystemExit("%s: CONFIG_MCU not found in make.inc" % objdir)


def section_sizes(elf):
    """Return {section: size} from avr-size -A."""
    out = subprocess.run(["avr-size", "-A", elf], check=True,
                         stdout=subprocess.PIPE, universal_newlines=True).stdout
    sizes = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1].isdigit():
            sizes[parts[0]] = int(parts[1])
    return sizes


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("objdirs", nargs="+", help="obj-* build directories")
    ap.add_argument("--extra-flash-margin", type=int, default=0, metavar="N",
                    help="add N bytes to the minimum free flash (to test the check)")
    ap.add_argument("--extra-ram-margin", type=int, default=0, metavar="N",
                    help="add N bytes to the minimum free RAM (to test the check)")
    args = ap.parse_args()

    flash = flash_limits()
    rows = []
    violations = []
    for objdir in args.objdirs:
        name = os.path.basename(os.path.normpath(objdir))
        mcu = build_mcu(objdir)
        if mcu not in flash or mcu not in RAM_SIZE or mcu not in LIMITS:
            violations.append("%s: no limits for %s" % (name, mcu))
            continue
        sec = section_sizes(os.path.join(objdir, "sd2iec.elf"))
        program = sec.get(".text", 0) + sec.get(".data", 0)
        ram = sec.get(".data", 0) + sec.get(".bss", 0) + sec.get(".noinit", 0)
        free_flash = flash[mcu] - program
        free_ram = RAM_SIZE[mcu] - ram
        min_flash = LIMITS[mcu]["flash_margin"] + args.extra_flash_margin
        min_ram = LIMITS[mcu]["ram_free"] + args.extra_ram_margin
        rows.append((name, "%d / %d" % (program, flash[mcu]), free_flash, free_ram))
        if program > flash[mcu]:
            violations.append("%s: program %d exceeds flash limit %d by %d bytes"
                              % (name, program, flash[mcu], program - flash[mcu]))
        elif free_flash < min_flash:
            violations.append("%s: free flash %d is below minimum %d"
                              % (name, free_flash, min_flash))
        if free_ram < min_ram:
            violations.append("%s: free RAM %d is below minimum %d"
                              % (name, free_ram, min_ram))

    header = ("build", "program / limit", "free flash", "free RAM")
    table = [header] + [(r[0], r[1], str(r[2]), str(r[3])) for r in rows]
    width = [max(len(r[i]) for r in table) for i in range(4)]
    for r in table:
        print("%-*s  %*s  %*s  %*s" % (width[0], r[0], width[1], r[1],
                                       width[2], r[2], width[3], r[3]))
    for v in violations:
        print("FAIL " + v)
    return 1 if violations else 0


if __name__ == "__main__":
    sys.exit(main())
