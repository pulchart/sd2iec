# Hey Emacs, this is a -*- makefile -*-

ifndef CONFIG
  CONFIG = config
endif

CONFDATA     := $(shell scripts/configparser.pl --confdata $(CONFIG))
CONFIGSUFFIX := $(word 1,$(CONFDATA))
OBJDIR       := obj-$(CONFIGSUFFIX)
CONFFILES    := $(wordlist 2,99,$(CONFDATA))

ifndef CONFFILES
.PHONY: missing_conf
missing_conf:
	@echo "ERROR: Please use $(MAKE) CONFIG=... to specify at least one config file."
	@echo "   (if you did, at least one file seems to be missing)"
endif

export CONFIGSUFFIX CONFIG OBJDIR

# Enable verbose compilation with "make V=1"
ifdef V
 Q :=
 E := @:
else
 Q := @
 E := @echo
endif

all: $(OBJDIR) $(OBJDIR)/make.inc
	$(Q)$(MAKE) --no-print-directory -f scripts/Makefile.main

$(OBJDIR)/make.inc: $(CONFFILES) | $(OBJDIR)
	$(E) "  CONFIG $(CONFFILES)"
	$(Q)scripts/configparser.pl --genfiles --makeinc $(OBJDIR)/make.inc --header $(OBJDIR)/autoconf.h $(CONFIG)

$(OBJDIR):
	$(E) "  MKDIR  $(OBJDIR)"
	-$(Q)mkdir $(OBJDIR)

copy clean fuses program delete-eeprom: FORCE | $(OBJDIR) $(OBJDIR)/make.inc
	$(Q)$(MAKE) --no-print-directory -f scripts/Makefile.main $@

FORCE: ;

# "make dist" builds every config in DISTCONFIGS into dist/ as
# sd2iec-<version>-<mcu>-<config>.bin, the version is set in version.mk.
# The configs in DISTMINI are built again with configs/addconfig-64k for the
# ATmega644P, as sd2iec-<version>-m644p-<config>-mini.bin
include version.mk

DISTCONFIGS ?= uIEC uIEC3 larsp plus sw1 sw2 evo2 larsp-lcd evo2-lcd larsp-oled evo2-oled petSD+ cbmSDmini
DISTMINI    ?= larsp sw1 sw2 larsp-lcd larsp-oled
DISTNAME    := sd2iec-$(MAJOR).$(MINOR)$(if $(PATCHLEVEL),.$(PATCHLEVEL))$(shell echo '$(PRERELEASE)' | tr A-Z a-z)

.PHONY: dist
dist:
	$(Q)mkdir -p dist
	$(Q)for c in $(DISTCONFIGS); do \
	  cfg=configs/config-$$c; \
	  suffix=$$(scripts/configparser.pl --confdata $$cfg | cut -d' ' -f1); \
	  rm -rf obj-$$suffix; \
	  $(MAKE) --no-print-directory CONFIG=$$cfg || exit 1; \
	  cp obj-$$suffix/sd2iec.bin dist/$(DISTNAME)-$$suffix.bin || exit 1; \
	  echo "  DIST   dist/$(DISTNAME)-$$suffix.bin"; \
	done
	$(Q)for c in $(DISTMINI); do \
	  cfg="configs/config-$$c configs/addconfig-64k"; \
	  suffix=$$(scripts/configparser.pl --confdata $$cfg | cut -d' ' -f1); \
	  rm -rf obj-$$suffix; \
	  $(MAKE) --no-print-directory CONFIG="$$cfg" || exit 1; \
	  cp obj-$$suffix/sd2iec.bin dist/$(DISTNAME)-$$suffix.bin || exit 1; \
	  echo "  DIST   dist/$(DISTNAME)-$$suffix.bin"; \
	done

# "make check-size" builds dist and fails when a build outgrows its flash or
# leaves too little RAM for the stack, the limits are in scripts/check-size.py
.PHONY: check-size
check-size: dist
	$(Q)dirs=""; \
	for c in $(DISTCONFIGS); do \
	  dirs="$$dirs obj-$$(scripts/configparser.pl --confdata configs/config-$$c | cut -d' ' -f1)"; \
	done; \
	for c in $(DISTMINI); do \
	  dirs="$$dirs obj-$$(scripts/configparser.pl --confdata configs/config-$$c configs/addconfig-64k | cut -d' ' -f1)"; \
	done; \
	scripts/check-size.py $$dirs

# "make dist-bootloader" downloads the AVR bootloader hex files into dist/
BOOTLOADER_VERSION ?= 0.4.1
BOOTLOADER_URL     ?= https://sd2iec.de/bootloader/newboot-$(BOOTLOADER_VERSION)-binaries.zip

.PHONY: dist-bootloader
dist-bootloader:
	$(Q)mkdir -p dist
	$(E) "  GET    $(BOOTLOADER_URL)"
	$(Q)curl -fsSL -o dist/newboot.zip $(BOOTLOADER_URL)
	$(Q)unzip -o -q -j dist/newboot.zip '*.hex' -d dist
	$(Q)rm -f dist/newboot.zip
