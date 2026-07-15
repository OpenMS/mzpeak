BUILD_DIR ?= build

.PHONEY: all clean test

all: $(BUILD_DIR)/compile_commands.json
	meson compile -C $(BUILD_DIR)

clean::
	rm -rf "$(BUILD_DIR)"

test:: all
	meson test -C $(BUILD_DIR)

$(BUILD_DIR)/compile_commands.json:
	meson setup $(BUILD_DIR) -Dwerror=true -Dbuildtype=debugoptimized
