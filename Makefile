BUILD_DIR ?= build

.PHONEY: all clean

all: $(BUILD_DIR)/compile_commands.json
	cd build && meson compile

clean::
	rm -rf "$(BUILD_DIR)"

$(BUILD_DIR)/compile_commands.json:
	meson setup $(BUILD_DIR)
