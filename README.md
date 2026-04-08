# e_iwd

Enlightenment module for Wi-Fi management via [iwd](https://iwd.wiki.kernel.org/),
a native replacement for the ConnMan-based econnman gadget.

See `CLAUDE.md` for the full PRD and implementation plan.

## Status

Phase 0 — scaffolding only. Nothing connects to D-Bus yet.

## Build

    meson setup build
    ninja -C build
    sudo ninja -C build install

Requires: `enlightenment`, `elementary`, `eldbus` (pkg-config).

## Layout

    src/
      e_mod_main.c       module entry points
      e_mod_gadget.c     shelf gadget
      e_mod_popup.c      popup UI
      e_mod_config.c     persistent settings
      iwd/               D-Bus client to net.connman.iwd
      ui/                reusable EFL widgets
    data/
      module.desktop
