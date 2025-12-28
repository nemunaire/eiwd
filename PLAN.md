# Implementation Plan: eiwd - Enlightenment Wi-Fi Module (iwd Backend)

## Project Overview

Create a production-ready Enlightenment module that manages Wi-Fi connections using iwd's D-Bus API, providing a gadget + popup UI following Enlightenment conventions.

**Current State**: Fresh workspace with only CLAUDE.md PRD
**Target**: Feature parity with econnman Wi-Fi functionality using iwd instead of ConnMan

## System Context

- **Enlightenment**: 0.27.1 (Module API version 25)
- **iwd daemon**: Running and accessible via D-Bus (`net.connman.iwd`)
- **Build tools**: Meson, GCC, pkg-config available
- **Libraries**: EFL (eldbus, elementary, ecore, evas, edje, eina) + E headers

## Implementation Phases

### Phase 1: Build System & Module Skeleton

**Goal**: Create loadable .so module with proper build infrastructure

**Files to Create**:
- `meson.build` (root) - Project definition, dependencies, installation paths
- `src/meson.build` - Source compilation
- `data/meson.build` - Desktop file and theme compilation
- `data/module.desktop` - Module metadata
- `src/e_mod_main.c` - Module entry point (e_modapi_init/shutdown/save)
- `src/e_mod_main.h` - Module structures and config

**Key Components**:

1. **Meson root build**:
   - Dependencies: enlightenment, eldbus, elementary, ecore, evas, edje, eina
   - Installation path: `/usr/lib64/enlightenment/modules/iwd/linux-gnu-x86_64-0.27/module.so`

2. **Module entry point** (`e_mod_main.c`):
   ```c
   E_API E_Module_Api e_modapi = { E_MODULE_API_VERSION, "IWD" };
   E_API void *e_modapi_init(E_Module *m);
   E_API int e_modapi_shutdown(E_Module *m);
   E_API int e_modapi_save(E_Module *m);
   ```

3. **Config structure** (stored via EET):
   - config_version
   - auto_connect (bool)
   - show_hidden_networks (bool)
   - signal_refresh_interval
   - preferred_adapter

**Verification**: Module loads in Enlightenment without crashing

---

### Phase 2: D-Bus Layer (iwd Backend)

**Goal**: Establish communication with iwd daemon and abstract devices/networks

**Files to Create**:
- `src/iwd/iwd_dbus.c` + `.h` - D-Bus connection management
- `src/iwd/iwd_device.c` + `.h` - Device abstraction (Station interface)
- `src/iwd/iwd_network.c` + `.h` - Network abstraction
- `src/iwd/iwd_agent.c` + `.h` - Agent for passphrase requests

**Key Implementations**:

1. **D-Bus Manager** (`iwd_dbus.c`):
   - Connect to system bus `net.connman.iwd`
   - Subscribe to ObjectManager signals (InterfacesAdded/Removed)
   - Monitor NameOwnerChanged for iwd daemon restart
   - Provide signal subscription helpers

2. **Device Abstraction** (`iwd_device.c`):
   ```c
   typedef struct _IWD_Device {
       char *path, *name, *address;
       Eina_Bool powered, scanning;
       char *state; // "disconnected", "connecting", "connected"
       Eldbus_Proxy *device_proxy, *station_proxy;
   } IWD_Device;
   ```
   - Operations: scan, disconnect, connect_hidden, get_networks

3. **Network Abstraction** (`iwd_network.c`):
   ```c
   typedef struct _IWD_Network {
       char *path, *name, *type; // "open", "psk", "8021x"
       Eina_Bool known;
       int16_t signal_strength; // dBm
       Eldbus_Proxy *network_proxy;
   } IWD_Network;
   ```
   - Operations: connect, forget

4. **Agent Implementation** (`iwd_agent.c`):
   - Register D-Bus service at `/org/enlightenment/eiwd/agent`
   - Implement `RequestPassphrase(network_path)` method
   - Bridge between iwd requests and UI dialogs
   - **Security**: Never log passphrases, clear from memory after sending

**Verification**: Can list devices, trigger scan, receive PropertyChanged signals

---

### Phase 3: Gadget & Basic UI

**Goal**: Create shelf icon and popup interface

**Files to Create**:
- `src/e_mod_gadget.c` - Gadcon provider and icon
- `src/e_mod_popup.c` - Popup window and layout
- `src/ui/wifi_status.c` + `.h` - Current connection widget
- `src/ui/wifi_list.c` + `.h` - Network list widget

**Key Implementations**:

1. **Gadcon Provider** (`e_mod_gadget.c`):
   ```c
   static const E_Gadcon_Client_Class _gc_class = {
       GADCON_CLIENT_CLASS_VERSION, "iwd", { ... }
   };
   ```
   - Icon states via edje: disconnected, connecting, connected (signal tiers), error
   - Click handler: toggle popup
   - Tooltip: SSID, signal strength, security type

2. **Popup Window** (`e_mod_popup.c`):
   - Layout: Current Connection + Available Networks + Actions
   - Current: SSID, signal, IP, disconnect button
   - Networks: sorted by known → signal strength
   - Actions: Rescan, Enable/Disable Wi-Fi

3. **Network List Widget** (`ui/wifi_list.c`):
   - Use elm_genlist or e_widget_ilist
   - Icons: open/WPA2/WPA3 lock icons
   - Sort: known networks first, then by signal
   - Click handler: initiate connection

**Verification**: Gadget appears on shelf, popup opens/closes, networks display

---

### Phase 4: Connection Management

**Goal**: Complete connection flow including authentication

**Files to Create**:
- `src/ui/wifi_auth.c` + `.h` - Passphrase dialog
- `src/iwd/iwd_state.c` + `.h` - State machine

**Connection Flow**:
1. User clicks network in list
2. Check security type (open vs psk vs 8021x)
3. If psk: show auth dialog (`wifi_auth_dialog_show`)
4. Call `network.Connect()` D-Bus method
5. iwd calls agent's `RequestPassphrase`
6. Return passphrase from dialog
7. Monitor `Station.State` PropertyChanged
8. Update UI: connecting → connected

**State Machine** (`iwd_state.c`):
```c
typedef enum {
    IWD_STATE_OFF,        // Powered = false
    IWD_STATE_IDLE,       // Powered = true, disconnected
    IWD_STATE_SCANNING,
    IWD_STATE_CONNECTING,
    IWD_STATE_CONNECTED,
    IWD_STATE_ERROR       // iwd not running
} IWD_State;
```

**Known Networks**:
- List via KnownNetwork interface
- Operations: Forget, Set AutoConnect
- UI: star icon for known, context menu

**Verification**: Connect to open/WPA2 networks, disconnect, forget network

---

### Phase 5: Advanced Features

**Goal**: Handle edge cases and advanced scenarios

**Implementations**:

1. **Hidden Networks**:
   - Add "Connect to Hidden Network" button
   - Call `Station.ConnectHiddenNetwork(ssid)`

2. **Multiple Adapters**:
   - Monitor all `/net/connman/iwd/[0-9]+` paths
   - UI: dropdown/tabs if multiple devices
   - Config: preferred adapter selection

3. **Daemon Restart Handling**:
   - Monitor NameOwnerChanged for `net.connman.iwd`
   - On restart: re-query ObjectManager, re-register agent, recreate proxies
   - Set error state while daemon down

4. **Polkit Integration**:
   - Detect `NotAuthorized` errors
   - Show user-friendly permission error dialog
   - Document required polkit rules (don't auto-install)

**Error Handling**:
- iwd not running → error state icon
- No wireless device → graceful message
- Permission denied → polkit error dialog
- Auth failure → clear error message (wrong password)

**Verification**: Handle iwd restart, multiple adapters, polkit errors

---

### Phase 6: Theme & Polish

**Goal**: Professional UI appearance and internationalization

**Files to Create**:
- `data/theme.edc` - Edje theme definition
- `data/icons/*.svg` - Icon source files
- `po/POTFILES.in` - i18n file list
- `po/eiwd.pot` - Translation template
- `src/e_mod_config.c` - Configuration dialog

**Theme Groups** (`theme.edc`):
- `e/modules/iwd/main` - Gadget icon
- `e/modules/iwd/signal/{0,25,50,75,100}` - Signal strength icons

**Configuration Dialog** (`e_mod_config.c`):
- Auto-connect to known networks: checkbox
- Show hidden networks: checkbox
- Signal refresh interval: slider (1-60s)
- Preferred adapter: dropdown

**i18n**:
- Mark strings with `D_(str)` macro (dgettext)
- Meson gettext integration

**Verification**: Theme scales properly, config saves, translations work

---

### Phase 7: Testing & Documentation

**Testing**:
- Unit tests: SSID parsing, signal conversion, config serialization
- Memory leak check: Valgrind during connect/disconnect cycles
- Manual checklist:
  - [ ] Module loads without errors
  - [ ] Scan, connect, disconnect work
  - [ ] Wrong password shows error
  - [ ] Known network auto-connect
  - [ ] iwd restart recovery
  - [ ] Suspend/resume handling
  - [ ] No device graceful degradation

**Documentation** (`README.md`, `INSTALL.md`):
- Overview and features
- Dependencies
- Building with Meson
- Installation paths
- iwd setup requirements
- Polkit configuration
- Troubleshooting

**Verification**: All tests pass, documentation complete

---

### Phase 8: Packaging & Distribution

**Packaging**:
- Arch Linux PKGBUILD
- Gentoo ebuild
- Generic tarball

**Installation**:
```bash
meson setup build
ninja -C build
sudo ninja -C build install
```

Module location: `/usr/lib64/enlightenment/modules/iwd/`

**Verification**: Clean install works, module appears in E module list

---

## Directory Structure

```
/home/nemunaire/workspace/eiwd/
├── meson.build                  # Root build config
├── meson_options.txt
├── README.md
├── INSTALL.md
├── LICENSE
├── data/
│   ├── meson.build
│   ├── module.desktop          # Module metadata
│   ├── theme.edc               # Edje theme
│   └── icons/                  # SVG/PNG icons
├── po/                         # i18n
│   ├── POTFILES.in
│   └── eiwd.pot
├── src/
│   ├── meson.build
│   ├── e_mod_main.c            # Module entry point
│   ├── e_mod_main.h
│   ├── e_mod_config.c          # Config dialog
│   ├── e_mod_gadget.c          # Shelf icon
│   ├── e_mod_popup.c           # Popup window
│   ├── iwd/
│   │   ├── iwd_dbus.c          # D-Bus connection
│   │   ├── iwd_dbus.h
│   │   ├── iwd_device.c        # Device abstraction
│   │   ├── iwd_device.h
│   │   ├── iwd_network.c       # Network abstraction
│   │   ├── iwd_network.h
│   │   ├── iwd_agent.c         # Agent implementation
│   │   ├── iwd_agent.h
│   │   ├── iwd_state.c         # State machine
│   │   └── iwd_state.h
│   └── ui/
│       ├── wifi_status.c       # Connection status widget
│       ├── wifi_status.h
│       ├── wifi_list.c         # Network list widget
│       ├── wifi_list.h
│       ├── wifi_auth.c         # Passphrase dialog
│       └── wifi_auth.h
└── tests/
    ├── meson.build
    └── test_network.c
```

---

## Critical Files (Implementation Order)

1. **`meson.build`** - Build system foundation
2. **`src/e_mod_main.c`** - Module lifecycle (init/shutdown/save)
3. **`src/iwd/iwd_dbus.c`** - D-Bus connection to iwd
4. **`src/iwd/iwd_agent.c`** - Passphrase handling (essential for secured networks)
5. **`src/e_mod_gadget.c`** - Primary user interface (shelf icon)

---

## Key Technical Decisions

**Build System**: Meson (modern, used by newer E modules)
**UI Framework**: Elementary widgets (EFL/Enlightenment standard)
**D-Bus Library**: eldbus (EFL integration, async)
**State Management**: Signal-driven (no polling)
**Security**: Never log passphrases, rely on iwd for credential storage

---

## Performance Targets

- Startup: < 100ms
- Popup open: < 200ms
- Network scan: < 2s
- Memory footprint: < 5 MB
- No periodic polling (signal-driven only)

---

## Dependencies

**Build**:
- meson >= 0.56
- ninja
- gcc/clang
- pkg-config
- edje_cc

**Runtime**:
- enlightenment >= 0.25
- efl (elementary, eldbus, ecore, evas, edje, eina)
- iwd >= 1.0
- dbus

**Optional**:
- polkit (permissions management)

---

## Security Considerations

1. **Never log passphrases** - No debug output of credentials
2. **Clear sensitive data** - memset passphrases after use
3. **D-Bus only** - No plaintext credential storage in module
4. **Polkit enforcement** - Respect system authorization policies
5. **Validate D-Bus params** - Don't trust all incoming data

---

## Known Limitations

- No VPN support (out of scope per PRD)
- No ethernet management (iwd is Wi-Fi only)
- Basic EAP UI (username/password only, no advanced cert config)
- No WPS support in initial version

---

## Success Criteria

- Module loads and appears in Enlightenment module list
- Can scan for networks and display them sorted by known + signal
- Can connect to open and WPA2/WPA3 networks with passphrase
- Can disconnect and forget networks
- Handles iwd daemon restart gracefully
- No UI freezes during scan/connect operations
- Memory leak free (Valgrind clean)
- Feature parity with econnman Wi-Fi functionality
