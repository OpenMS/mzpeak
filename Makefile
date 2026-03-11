BUILD_DIR ?= build

.PHONEY: all clean test

all: $(BUILD_DIR)/compile_commands.json
	cd build && meson compile

clean::
	rm -rf "$(BUILD_DIR)"

test:: all
	cd build && meson test

$(BUILD_DIR)/compile_commands.json:
	meson setup $(BUILD_DIR)
