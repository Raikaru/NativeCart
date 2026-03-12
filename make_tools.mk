# This controls building executables in the `tools` folder.
# Can be invoked through the `Makefile` or standalone.

MAKEFLAGS += --no-print-directory

# Inclusive list. If you don't want a tool to be built, don't add it here.
TOOLS_DIR := tools
TOOL_NAMES := bin2c gbafix gbagfx jsonproc mapjson mid2agb preproc ramscrgen rsfont scaninc wav2agb

TOOLDIRS := $(TOOL_NAMES:%=$(TOOLS_DIR)/%)

# Tool making doesnt require a pokefirered dependency scan.
RULES_NO_SCAN += tools check-tools check-tools-deps clean-tools $(TOOLDIRS)
.PHONY: $(RULES_NO_SCAN)

tools:
	@$(MAKE) check-tools-deps
	@for tooldir in $(TOOLDIRS); do \
		$(MAKE) -C $$tooldir || exit $$?; \
	done

check-tools-deps:
	@status=0; \
	$(MAKE) -C $(TOOLS_DIR)/gbagfx check-libpng || status=1; \
	$(MAKE) -C $(TOOLS_DIR)/rsfont check-libpng || status=1; \
	exit $$status

$(TOOLDIRS):
	@$(MAKE) -C $@

clean-tools:
	@$(foreach tooldir,$(TOOLDIRS),$(MAKE) clean -C $(tooldir);)
