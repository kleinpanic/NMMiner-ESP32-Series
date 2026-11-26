# Host build of the PanicMiner UI. `make` renders every screen to out-t$(THEME)/*.png; THEME=1 or 2 picks the Mine look.
# LVGL is pinned to the tag the firmware uses and cloned into .deps on first run.
LVGL_TAG := v8.3.11
LVGL     := .deps/lvgl
UI       := ../../src/ui
THEME    ?= 1
BUILD    := build-t$(THEME)
OUT      := out-t$(THEME)

CFLAGS   := -O1 -g -DPM_THEME=$(THEME) -DLV_CONF_INCLUDE_SIMPLE -I$(UI) -I$(LVGL) -I.deps -Wall -Wno-unused-parameter
CXXFLAGS := $(CFLAGS) -Wextra

LVGL_SRC := $(shell find $(LVGL)/src -name '*.c' 2>/dev/null)
UI_SRC   := $(wildcard $(UI)/*.cpp)
FONT_SRC := $(wildcard $(UI)/fonts/*.c)
OBJ      := $(patsubst %,$(BUILD)/%.o,$(notdir $(LVGL_SRC) $(FONT_SRC) main.c $(UI_SRC)))

vpath %.c $(sort $(dir $(LVGL_SRC) $(FONT_SRC))) .
vpath %.cpp $(UI)

.PHONY: render clean
render: $(BUILD)/uisim
	mkdir -p $(OUT)
	./$(BUILD)/uisim $(OUT)

$(LVGL)/lvgl.h:
	git clone -q --depth 1 -b $(LVGL_TAG) https://github.com/lvgl/lvgl.git $(LVGL)

$(BUILD)/uisim: $(OBJ)
	$(CXX) -o $@ $^ -lpng -lm

$(BUILD)/%.c.o: %.c $(UI)/lv_conf.h $(UI)/ui.h | $(BUILD) $(LVGL)/lvgl.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.cpp.o: %.cpp $(wildcard $(UI)/*.h) | $(BUILD) $(LVGL)/lvgl.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD) $(OUT)
