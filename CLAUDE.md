PRD — Enlightenment Wi-Fi Module (iwd Backend)
1. Overview
1.1 Purpose

Create an Enlightenment module that manages Wi-Fi connections using iwd (Intel Wireless Daemon) as the backend, providing functionality similar to econnman, but without ConnMan.

The module should:

    Integrate cleanly with Enlightenment (E17+)

    Use iwd’s D-Bus API directly

    Provide a simple, fast, and reliable Wi-Fi UI

    Follow Enlightenment UX conventions (gadget + popup)

1.2 Motivation

    ConnMan is increasingly deprecated or undesired on many systems

    iwd is lightweight, fast, and widely adopted (Arch, Fedora, Debian)

    Enlightenment currently lacks a first-class iwd-based Wi-Fi module

    Users want a native, non-NM, non-ConnMan Wi-Fi solution

2. Goals & Non-Goals
2.1 Goals

    Feature parity with basic econnman Wi-Fi features

    Zero dependency on NetworkManager or ConnMan

    D-Bus only (no shelling out to iwctl)

    Minimal background CPU/memory usage

    Robust behavior across suspend/resume and network changes

2.2 Non-Goals

    Ethernet management

    VPN management

    Cellular (WWAN) support

    Advanced enterprise Wi-Fi UI (EAP tuning beyond basics)

3. Target Users

    Enlightenment desktop users

    Minimalist / embedded systems using iwd

    Power users avoiding NetworkManager

    Distributions shipping iwd by default

4. User Experience
4.1 Gadget (Shelf Icon)

    Status icon:

        Disconnected

        Connecting

        Connected (signal strength tiers)

        Error

    Tooltip:

        Current SSID

        Signal strength

        Security type

4.2 Popup UI

Triggered by clicking the gadget.
Sections:

    Current Connection

        SSID

        Signal strength

        IP (optional)

        Disconnect button

    Available Networks

        Sorted by:

            Known networks

            Signal strength

        Icons for:

            Open

            WPA2/WPA3

        Connect button per network

    Actions

        Rescan

        Enable / Disable Wi-Fi

4.3 Authentication Flow

    On selecting a secured network:

        Prompt for passphrase

        Optional “remember network”

    Errors clearly reported (wrong password, auth failed, etc.)

5. Functional Requirements
5.1 Wi-Fi Control

    Enable / disable Wi-Fi (via iwd Powered)

    Trigger scan

    List available networks

    Connect to a network

    Disconnect from current network

5.2 Network State Monitoring

    React to:

        Connection changes

        Signal strength changes

        Device availability

        iwd daemon restart

5.3 Known Networks

    List known (previously connected) networks

    Auto-connect indication

    Forget network

5.4 Error Handling

    iwd not running

    No wireless device

    Permission denied (polkit)

    Authentication failure

6. Technical Requirements
6.1 Backend

    iwd via D-Bus

    Service: net.connman.iwd

    No external command execution

6.2 D-Bus Interfaces Used (Non-Exhaustive)

    net.connman.iwd.Adapter

    net.connman.iwd.Device

    net.connman.iwd.Network

    net.connman.iwd.Station

    net.connman.iwd.KnownNetwork

6.3 Permissions

    Requires polkit rules for:

        Scanning

        Connecting

        Forgetting networks

    Module must gracefully degrade without permissions

7. Architecture
7.1 Module Structure

e_iwd/
├── e_mod_main.c
├── e_mod_config.c
├── e_mod_gadget.c
├── e_mod_popup.c
├── iwd/
│   ├── iwd_manager.c
│   ├── iwd_device.c
│   ├── iwd_network.c
│   └── iwd_dbus.c
└── ui/
    ├── wifi_list.c
    ├── wifi_auth.c
    └── wifi_status.c

7.2 Data Flow

iwd (D-Bus)
   ↓
iwd_dbus.c
   ↓
iwd_manager / device / network
   ↓
UI layer (EFL widgets)
   ↓
Gadget / Popup

7.3 Threading Model

    Single main loop

    Async D-Bus calls via EFL

    No blocking calls on UI thread

8. State Model
8.1 Connection States

    OFF

    IDLE

    SCANNING

    CONNECTING

    CONNECTED

    ERROR

8.2 Transitions

    Triggered by:

        User actions

        iwd signals

        System suspend/resume

9. Configuration
9.1 Module Settings

    Auto-connect enabled / disabled

    Show hidden networks

    Signal strength refresh interval

    Preferred adapter (if multiple)

Stored using Enlightenment module config system.
10. Performance & Reliability
10.1 Performance

    Startup time < 100 ms

    No periodic polling; signal-driven updates

    Minimal memory footprint (< 5 MB)

10.2 Reliability

    Handle iwd restart gracefully

    Auto-rebind D-Bus objects

    Avoid crashes on device hot-plug

11. Security Considerations

    Never log passphrases

    Passphrases only sent over D-Bus to iwd

    Respect system polkit policies

    No plaintext storage in module config


13. Success Metrics

    Successful connect/disconnect in ≥ 99% cases

    No UI freezes during scan/connect

    Parity with econnman Wi-Fi UX

    Adoption by at least one major distro Enlightenment spin

14. Future Extensions (Out of Scope)

    Ethernet support

    VPN integration

    QR-based Wi-Fi sharing

    Per-network advanced EAP UI

15. Open Questions / Risks

    Polkit UX integration (password prompts)

    Multiple adapter handling UX

    iwd API changes across versions
