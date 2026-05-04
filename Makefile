all: compile

proto:
	wayland-scanner server-header $(shell pkg-config --variable=pkgdatadir wayland-protocols)/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h
	wayland-scanner server-header ./protocols/wlr-layer-shell-unstable-v1.xml wlr-layer-shell-unstable-v1-protocol.h

compile: proto
	gcc -o absinthe src/* \
		-Wall -Wextra -Wpedantic \
		-DXWAYLAND \
		-I./ -I./include -DWLR_USE_UNSTABLE $(shell pkg-config wlroots-0.20 --libs --cflags) -lwayland-server -lxkbcommon \
		-O3 \
		-g

format:
	find src/ -type f -print0 | xargs -0 clang-format -i
	find include/ -type f -print0 | xargs -0 clang-format -i
