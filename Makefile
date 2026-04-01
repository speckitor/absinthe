all: compile

proto:
	wayland-scanner server-header $(shell pkg-config --variable=pkgdatadir wayland-protocols)/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h

compile: proto
	gcc -o absinthe src/* \
		-I./ -I./include -DWLR_USE_UNSTABLE $(shell pkg-config wlroots-0.20 --libs --cflags) -lwayland-server -lxkbcommon \
		-ggdb
