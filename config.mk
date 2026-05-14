XWAYLAND = -DXWAYLAND
XLIBS = xcb xcb-icccm

# tools
PKG_CONFIG ?= pkg-config
CC ?= clang
CLANG_FORMAT ?= clang-format

PKGS = wayland-server xkbcommon libinput $(XLIBS)

PKG_CFLAGS != $(PKG_CONFIG) --cflags $(PKGS)
PKG_LDFLAGS != $(PKG_CONFIG) --libs $(PKGS)

WAYLAND_SCANNER != $(PKG_CONFIG) --variable=wayland_scanner wayland-scanner
WAYLAND_PROTOCOLS != $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols

WLR_CFLAGS != $(PKG_CONFIG) --cflags wlroots-0.20
WLR_LDFLAGS != $(PKG_CONFIG) --libs wlroots-0.20

DEVFLAGS = -g

CPPFLAGS += $(XWAYLAND) -DWLR_USE_UNSTABLE -I. -I./include

CFLAGS += -O2 -march=native -Wall -Wextra
CFLAGS += $(PKG_CFLAGS) $(WLR_CFLAGS)
CFLAGS += $(CPPFLAGS)
CFLAGS += $(DEVFLAGS)

LDFLAGS += $(PKG_LDFLAGS) $(WLR_LDFLAGS)
