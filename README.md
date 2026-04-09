# e_iwd — Enlightenment Wi-Fi module (iwd backend)

A native [Enlightenment](https://www.enlightenment.org/) gadget that
manages wireless connections through [iwd](https://iwd.wiki.kernel.org/),
the Intel Wireless Daemon. No NetworkManager, no ConnMan, no shelling
out to `iwctl` — everything goes over iwd's D‑Bus API.

It is roughly the iwd-only equivalent of `econnman`.

## Features

- **Shelf gadget** with a signal-tier icon (off / acquiring / weak…excellent)
  and a tooltip showing the current SSID, security type, and signal level.
- **Popup network browser** (left-click the gadget):
  - status line: disabled / disconnected / scanning / connecting / connected
  - sorted network list — connected first, then known networks, then by
    signal strength; long SSIDs are truncated to keep the popup tidy
  - per-row signal bars and security tag (`open` / `WPA` / `WEP` / `802.1X`)
  - **Connect** by clicking a row, **Forget** (`✕`) on known networks
  - **Rescan**, **Enable / Disable** Wi‑Fi
  - **Disconnect** button visible while connected
  - **Hidden…** button to join a non-broadcasting SSID
- **Authentication agent** registered with iwd:
  - passphrase prompt for new protected networks (modal dialog window)
  - cancel-on-`Agent.Cancel` so iwd-initiated cancellations close the
    open prompt cleanly
  - polite stubs for `RequestUserNameAndPassword`, `RequestUserPassword`
    and `RequestPrivateKeyPassphrase` so iwd doesn't unregister us when
    it tries them on EAP networks
- **Settings dialog** (right-click the gadget → Settings):
  - auto-connect to known networks
  - show hidden networks
  - signal refresh interval
  - preferred wireless adapter
- **Robust to iwd lifecycle**: tracks `net.connman.iwd` name owner,
  re-binds objects on restart, clears state on departure.

## Architecture

```
e_iwd/
├── src/
│   ├── e_mod_main.c           module init/shutdown
│   ├── e_mod_gadget.c         gadcon provider, icon + tooltip + menu
│   ├── e_mod_popup.c          network list popup
│   ├── e_mod_config.c         persistent settings + E_Config_Dialog
│   ├── iwd/
│   │   ├── iwd_dbus.c         system bus, name owner, ObjectManager
│   │   ├── iwd_manager.c      top-level state aggregator + listeners
│   │   ├── iwd_adapter.c      net.connman.iwd.Adapter (Powered)
│   │   ├── iwd_device.c       Device + Station, scan/connect/disconnect,
│   │   │                      GetOrderedNetworks → signal strength
│   │   ├── iwd_network.c      Network.Connect, KnownNetwork.Forget
│   │   ├── iwd_agent.c        net.connman.iwd.Agent (passphrase, cancel)
│   │   └── iwd_props.c        a{sv} parsing helpers
│   └── ui/
│       ├── wifi_auth.c        passphrase dialog (floating elm_win)
│       └── wifi_hidden.c      hidden-network SSID + passphrase dialog
└── meson.build
```

Data flow:

```
iwd (D-Bus) ──► iwd_dbus ──► iwd_manager ──► iwd_device / iwd_network
                                 │
                                 ├──► listeners (gadget, popup)
                                 └──► Iwd_Agent ──► UI passphrase prompt
```

The module uses **Eldbus** for all bus traffic and **Elementary** for
its widgets. Everything is async — no blocking calls on the UI thread.

## Building

Dependencies (development headers):

- Enlightenment ≥ 0.25 (tested against 0.27)
- EFL ≥ 1.26 (Eldbus, Elementary, Edje, Ecore, Eina)
- meson + ninja
- a running `iwd` ≥ 1.0 (runtime, not build-time)

Build and install:

```sh
meson setup build
ninja -C build
sudo ninja -C build install
```

The module is installed to
`<libdir>/enlightenment/modules/iwd/<module_arch>/module.so`. The
`module_arch` and `libdir` are pulled from Enlightenment's pkg-config
file, so the install path matches whatever your distro packages.

Once installed, enable it from **Settings → Modules → Extensions →
iwd**, then add the gadget to a shelf or the desktop via
**Settings → Gadgets**.

## Runtime requirements

- `iwd` running as a system service (`systemctl enable --now iwd`).
- Your user must be allowed to talk to `net.connman.iwd` on the system
  bus. On most distros this means being in the `network` group, or
  having a polkit rule for the `net.connman.iwd` interfaces. The module
  degrades gracefully when permissions are missing — you'll just see an
  empty list.
- A wireless adapter managed by iwd (i.e. not claimed by
  NetworkManager / wpa_supplicant).

## Usage

| Action | How |
|---|---|
| Open the network list | Left-click the gadget |
| Open settings | Right-click the gadget → Settings |
| Connect to a known network | Click its row in the list |
| Connect to a new protected network | Click its row, enter the passphrase in the dialog |
| Forget a known network | Click the `✕` button on its row |
| Disconnect | Click **Disconnect** in the popup (visible while connected) |
| Join a hidden SSID | Click **Hidden…**, enter SSID and (optional) passphrase |
| Rescan | Click **Rescan** |
| Disable / enable Wi-Fi | Click **Disable** / **Enable** in the popup |

Passphrases are sent straight to iwd over D-Bus. They are never logged,
never written to the module config, and are zeroed in memory once the
dialog closes.

## Configuration

Settings are persisted via Enlightenment's standard config system as
`module.iwd` (an `eet` file under your E config profile, e.g.
`~/.e/e/config/<profile>/module.iwd.cfg`). Fields:

| Field | Default | Meaning |
|---|---|---|
| `auto_connect`       | on  | Let iwd auto-connect to known networks |
| `show_hidden`        | off | Reveal hidden networks in the list |
| `refresh_interval`   | 5   | Signal-strength refresh interval (seconds) |
| `preferred_adapter`  | —   | Preferred wireless adapter (blank = auto) |

## Status

Phases 0–4 of the project are complete and the module builds cleanly
against EFL 1.28 / Enlightenment 0.27. Phase 5 (robustness) and Phase 6
(packaging) are partially landed.

Known gaps:

- No custom theme `edj` — the gadget uses freedesktop icon names from
  the active icon theme.
- No suspend / resume integration.
- No EAP UI (RequestUserName / RequestPrivateKey are stubbed to
  refuse).
- Multi-adapter UX is auto-select-first; the preferred-adapter setting
  is plumbed but not yet honored by the manager.
- Not yet tested by valgrind for leaks.

## License

MIT-style, matching Enlightenment and EFL. See `LICENSE`.
