# sim/Makefile — Build the dashboard SDL2 simulator on macOS
#
# Usage (run from esp32-touch-ui/ root):
#   make -f sim/Makefile          # build
#   make -f sim/Makefile run      # build + launch
#   make -f sim/Makefile clean

ROOT    := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))..
SIMDIR  := $(ROOT)/sim
SRCDIR  := $(ROOT)/src
UIDIR   := $(ROOT)/ui
LVGL    := $(SIMDIR)/lvgl
BUILD   := $(SIMDIR)/build

CXX     := clang++
CC      := clang
OUT     := $(SIMDIR)/dashboard_sim

SDL2_CFLAGS := $(shell sdl2-config --cflags)
SDL2_LIBS   := $(shell sdl2-config --libs)

# LVGL 8.3 C sources (compile as C, not C++)
LVGL_CSRC := $(shell find \
    $(LVGL)/src/core \
    $(LVGL)/src/draw \
    $(LVGL)/src/extra \
    $(LVGL)/src/font \
    $(LVGL)/src/hal \
    $(LVGL)/src/misc \
    $(LVGL)/src/widgets \
    -name "*.c" 2>/dev/null)

# Project C sources
APP_CSRC := \
    $(UIDIR)/ui.c \
    $(UIDIR)/assets/vehicle_top.c

# Project C++ sources
APP_CXXSRC := \
    $(SIMDIR)/main_sim.cpp \
    $(SRCDIR)/sensors.cpp

# ── Common include flags ───────────────────────────────────────────────────────
INCLUDES := \
    -I$(SIMDIR) \
    -I$(LVGL) \
    -I$(LVGL)/src \
    -I$(SRCDIR) \
    -I$(UIDIR) \
    -I$(ROOT) \
    $(SDL2_CFLAGS)

DEFINES := \
    -DLV_CONF_INCLUDE_SIMPLE \
    -DLV_LVGL_H_INCLUDE_SIMPLE \
    -DSIMULATOR=1

WARN := \
    -Wno-unused-function \
    -Wno-unused-variable \
    -Wno-deprecated-declarations \
    -Wno-sign-compare

# C flags (for LVGL and ui.c — must be C99, NOT C++)
CFLAGS   := -O2 -g -std=c99 $(DEFINES) $(INCLUDES) $(WARN)

# C++ flags (for main_sim.cpp, sensors.cpp)
CXXFLAGS := -O2 -g -std=c++17 $(DEFINES) $(INCLUDES) $(WARN)

LDFLAGS  := $(SDL2_LIBS) -lpthread

# ── Object file lists ─────────────────────────────────────────────────────────
LVGL_OBJS   := $(patsubst $(LVGL)/%.c,    $(BUILD)/lvgl/%.o,  $(LVGL_CSRC))
APP_COBJS   := $(patsubst $(UIDIR)/%.c,   $(BUILD)/ui/%.o,    $(APP_CSRC))
APP_CXXOBJS := $(patsubst $(SIMDIR)/%.cpp,$(BUILD)/sim/%.o,   $(filter $(SIMDIR)/%,$(APP_CXXSRC))) \
               $(patsubst $(SRCDIR)/%.cpp, $(BUILD)/src/%.o,   $(filter $(SRCDIR)/%,$(APP_CXXSRC)))

ALL_OBJS := $(LVGL_OBJS) $(APP_COBJS) $(APP_CXXOBJS)

.PHONY: all run clean fetch

all: fetch $(OUT)

fetch:
	@if [ ! -d "$(LVGL)/.git" ]; then \
		echo "[sim] Cloning LVGL 8.3..."; \
		git clone --depth=1 --branch release/v8.3 \
			https://github.com/lvgl/lvgl.git $(LVGL); \
	else \
		echo "[sim] LVGL already present"; \
	fi

$(OUT): $(ALL_OBJS)
	@echo "[sim] Linking..."
	$(CXX) $(ALL_OBJS) $(LDFLAGS) -o $@
	@echo ""
	@echo "  Build OK → $@"

# LVGL C objects
$(BUILD)/lvgl/%.o: $(LVGL)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ui.c and vehicle_top.c objects
$(BUILD)/ui/%.o: $(UIDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/ui/assets/%.o: $(UIDIR)/assets/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# C++ objects from sim/
$(BUILD)/sim/%.o: $(SIMDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# C++ objects from src/
$(BUILD)/src/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	$(OUT)

clean:
	rm -rf $(BUILD) $(OUT)

distclean: clean
	rm -rf $(LVGL)
