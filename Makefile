include config.mk

all: compile

proto:
	$(WAYLAND_SCANNER) server-header $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h
	$(WAYLAND_SCANNER) server-header ./protocols/wlr-layer-shell-unstable-v1.xml wlr-layer-shell-unstable-v1-protocol.h

compile: proto
	cc -o absinthe src/* $(CFLAGS) $(LDFLAGS)

format:
	find src/ -type f -print0 | xargs -0 clang-format -i
	find include/ -type f -print0 | xargs -0 clang-format -i
