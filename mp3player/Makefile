CC := clang
OBJC := clang

UNAME_S := $(shell uname -s)

CFLAGS := -std=c11 -Wall -Wextra -O2 -Ithird_party $(shell pkg-config --cflags sdl2 SDL2_image)
LDFLAGS := $(shell pkg-config --libs sdl2 SDL2_image) -lm

# miniaudio needs a thread + dynamic-loading backend on Linux; on macOS it
# talks to CoreAudio/AudioToolbox directly instead.
ifeq ($(UNAME_S),Linux)
	LDFLAGS += -lpthread -ldl
endif

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := mp3player

ifeq ($(UNAME_S),Darwin)
	# cocoa_shim.m is the only Objective-C
	OBJCSRC := src/cocoa_shim.m
	OBJ += $(OBJCSRC:.m=.o)
	OBJCFLAGS := -std=c11 -Wall -Wextra -O2 -fobjc-arc -Ithird_party $(shell pkg-config --cflags sdl2)
	LDFLAGS += -framework Cocoa -framework QuartzCore -framework CoreAudio -framework AudioToolbox -framework CoreFoundation
endif

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.m
	$(OBJC) $(OBJCFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	rm -f $(OBJ) $(BIN)
	rm -rf $(APP_DIR)

# ---------------------------------------------------------------------------
# macOS .app bundle
#
# main.c's chdir_to_executable_dir() already resolves the binary's own
# directory and chdir()s there before touching any relative asset path, so
# bundling is just: put the binary and a copy of assets/ side-by-side inside
# Contents/MacOS, and give it an Info.plist.
# ---------------------------------------------------------------------------
APP_NAME    := MP3Player
BUNDLE_ID   := com.example.mp3player
VERSION     := 1.0.0
APP_DIR     := $(APP_NAME).app
CONTENTS    := $(APP_DIR)/Contents
MACOS_DIR   := $(CONTENTS)/MacOS
RESOURCES   := $(CONTENTS)/Resources
ICON_SRC    := assets/reference/composited.png
ICONSET     := $(RESOURCES)/AppIcon.iconset
ICNS        := $(RESOURCES)/AppIcon.icns

define INFO_PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleName</key>
	<string>$(APP_NAME)</string>
	<key>CFBundleDisplayName</key>
	<string>$(APP_NAME)</string>
	<key>CFBundleIdentifier</key>
	<string>$(BUNDLE_ID)</string>
	<key>CFBundleVersion</key>
	<string>$(VERSION)</string>
	<key>CFBundleShortVersionString</key>
	<string>$(VERSION)</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleExecutable</key>
	<string>$(APP_NAME)</string>
	<key>CFBundleIconFile</key>
	<string>AppIcon</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>LSMinimumSystemVersion</key>
	<string>10.13</string>
	<key>NSHighResolutionCapable</key>
	<true/>
	<key>NSHumanReadableCopyright</key>
	<string>Copyright (C) 2026</string>
</dict>
</plist>
endef
export INFO_PLIST

.PHONY: app app-run app-icon app-clean

# make app only really makes sense on macOS since it packages the Darwin
# build (Cocoa window, CoreAudio, etc). Building it elsewhere would just
# zip up a Linux ELF inside a .app, which won't launch, so guard it.
app:
ifneq ($(UNAME_S),Darwin)
	$(error "make app" only produces a working bundle on macOS -- build/run this target there)
endif
	$(MAKE) all
	rm -rf $(APP_DIR)
	mkdir -p $(MACOS_DIR) $(RESOURCES)
	cp $(BIN) $(MACOS_DIR)/$(APP_NAME)
	cp -R assets $(MACOS_DIR)/assets
	@echo "$$INFO_PLIST" > $(CONTENTS)/Info.plist
	$(MAKE) app-icon
	# Ad-hoc sign so Gatekeeper doesn't flag it as "damaged" on this Mac,
	# and clear any quarantine bit picked up from the copied assets.
	xattr -cr $(APP_DIR)
	codesign --force --deep --sign - $(APP_DIR)
	@echo "Built $(APP_DIR)"

# Builds Resources/AppIcon.icns from assets/reference/composited.png.
# Best-effort: skipped quietly if the source image or the macOS icon tools
# (sips/iconutil) aren't available, since the bundle still works without
# a custom icon (macOS just shows the generic app icon).
app-icon:
ifeq ($(UNAME_S),Darwin)
	@if [ -f "$(ICON_SRC)" ] && command -v sips >/dev/null && command -v iconutil >/dev/null; then \
		mkdir -p "$(ICONSET)"; \
		SIDE=$$(sips -g pixelWidth -g pixelHeight "$(ICON_SRC)" | awk '/pixelWidth|pixelHeight/{print $$2}' | sort -n | tail -1); \
		sips -padToHeightWidth $$SIDE $$SIDE "$(ICON_SRC)" --out "$(RESOURCES)/_icon_square.png" >/dev/null; \
		for sz in 16 32 128 256 512; do \
			sips -z $$sz $$sz "$(RESOURCES)/_icon_square.png" --out "$(ICONSET)/icon_$${sz}x$${sz}.png" >/dev/null; \
			dz=$$((sz*2)); \
			sips -z $$dz $$dz "$(RESOURCES)/_icon_square.png" --out "$(ICONSET)/icon_$${sz}x$${sz}@2x.png" >/dev/null; \
		done; \
		iconutil -c icns "$(ICONSET)" -o "$(ICNS)"; \
		rm -rf "$(ICONSET)" "$(RESOURCES)/_icon_square.png"; \
		echo "Generated $(ICNS) from $(ICON_SRC)"; \
	else \
		echo "Skipping app icon (no $(ICON_SRC) or sips/iconutil unavailable) -- bundle will use the generic icon."; \
	fi
endif

app-run: app
	open $(APP_DIR)

app-clean:
	rm -rf $(APP_DIR)
