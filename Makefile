include config.mk

TARGET = absinthe

SRC_FILES != find src/ -type f
OBJ_FILES = $(SRC_FILES:src/%.c=build/%.o)

all: $(TARGET)

proto:
	$(WAYLAND_SCANNER) server-header $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h
	$(WAYLAND_SCANNER) server-header ./protocols/wlr-layer-shell-unstable-v1.xml wlr-layer-shell-unstable-v1-protocol.h

$(OBJ_FILES):
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $(@:build/%.o=src/%.c)

$(TARGET): proto $(OBJ_FILES)
	$(CC) -o absinthe $(OBJ_FILES) $(LDFLAGS)

clean:
	rm -rf build/
	rm absinthe
	rm xdg-shell-protocol.h
	rm wlr-layer-shell-unstable-v1-protocol.h

format:
	find src/ -type f | xargs $(CLANG_FORMAT) -i
	find include/ -type f | xargs $(CLANG_FORMAT) -i
