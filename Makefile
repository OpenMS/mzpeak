BUILD_DIR ?= build

# NOTE:
#
# To build profiling versions of the executable targets:
#
# make clean
# CXXFLAGS='-pg -O0' LDFLAGS='-pg' make

.PHONEY: all clean test

all: $(BUILD_DIR)/compile_commands.json
	meson compile -C $(BUILD_DIR)

clean::
	rm -rf "$(BUILD_DIR)"

test:: all
	meson test -C $(BUILD_DIR)

$(BUILD_DIR)/compile_commands.json:
	meson setup $(BUILD_DIR) -Dwerror=true -Dbuildtype=debugoptimized
