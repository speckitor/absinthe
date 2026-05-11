include config.mk

TARGET = absinthe

SRC_FILES != find src/ -type f
OBJ_FILES = $(SRC_FILES:src/%.c=build/%.o)

all: proto $(TARGET)

xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml ${.TARGET}

wlr-layer-shell-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header ./protocols/wlr-layer-shell-unstable-v1.xml ${.TARGET}

proto: xdg-shell-protocol.h wlr-layer-shell-unstable-v1-protocol.h

.for _src in ${SRC_FILES}
build/${_src:T:R}.o: ${_src}
	@mkdir -p build
	$(CC) $(CFLAGS) -c ${.ALLSRC} -o ${.TARGET}
.endfor

$(TARGET): $(OBJ_FILES)
	$(CC) -o absinthe $(OBJ_FILES) $(LDFLAGS)

clean:
	rm -rf build/
	rm absinthe
	rm xdg-shell-protocol.h
	rm wlr-layer-shell-unstable-v1-protocol.h

format:
	find src/ -type f | xargs $(CLANG_FORMAT) -i
	find include/ -type f | xargs $(CLANG_FORMAT) -i
