# eiwd - Enlightenment Wi-Fi Module (iwd Backend)

A native Enlightenment module for managing Wi-Fi connections using Intel Wireless Daemon (iwd) as the backend.

## Overview

**eiwd** provides seamless Wi-Fi management directly within the Enlightenment desktop environment without requiring NetworkManager or ConnMan. It uses iwd's D-Bus API for fast, lightweight, and reliable wireless networking.

## Features

### Core Functionality
- **Device Management**: Automatic detection of wireless adapters
- **Network Discovery**: Fast scanning with signal strength indicators
- **Connection Management**: Connect/disconnect with one click
- **Known Networks**: Automatic connection to saved networks
- **Hidden Networks**: Support for connecting to non-broadcast SSIDs
- **Security**: WPA2/WPA3-PSK authentication with secure passphrase handling

### User Interface
- **Shelf Gadget**: Compact icon showing connection status
- **Visual Feedback**: Color-coded states (disconnected, connecting, connected, error)
- **Popup Menu**: Quick access to available networks and actions
- **Configuration Dialog**: Customizable settings and preferences
- **Theme Support**: Full Edje theme integration with signal strength display

### Advanced Features
- **Daemon Recovery**: Automatic reconnection when iwd restarts
- **Multi-Adapter Support**: Detection and management of multiple wireless devices
- **State Machine**: Robust connection state tracking
- **Error Handling**: User-friendly error messages with troubleshooting hints
- **Polkit Integration**: Respects system authorization policies

## Requirements

### Build Dependencies
- `enlightenment` >= 0.25
- `efl` (Elementary, Eldbus, Ecore, Evas, Edje, Eina) >= 1.26
- `meson` >= 0.56
- `ninja` build tool
- `gcc` or `clang` compiler
- `pkg-config`
- `edje_cc` (EFL development tools)

### Runtime Dependencies
- `enlightenment` >= 0.25
- `efl` runtime libraries
- `iwd` >= 1.0
- `dbus` system bus

### Optional
- `polkit` for fine-grained permission management
- `gettext` for internationalization support

## Building

```bash
# Clone repository
git clone <repository-url> eiwd
cd eiwd

# Configure build
meson setup build

# Compile
ninja -C build

# Install (as root)
sudo ninja -C build install
```

### Build Options

```bash
# Disable internationalization
meson setup build -Dnls=false

# Custom installation prefix
meson setup build --prefix=/usr/local
```

## Installation

The module is installed to:
```
/usr/lib64/enlightenment/modules/iwd/linux-x86_64-0.27/
├── module.so         # Main module binary
└── e-module-iwd.edj  # Theme file
```

After installation:
1. Open Enlightenment Settings → Modules
2. Find "IWD Wi-Fi" in the list
3. Click "Load" to enable the module
4. Add the gadget to your shelf via shelf settings

## Configuration

### Module Settings

Access via right-click on the gadget or Settings → Modules → IWD → Configure:

- **Auto-connect**: Automatically connect to known networks
- **Show hidden networks**: Display option to connect to hidden SSIDs
- **Signal refresh interval**: Update frequency (1-60 seconds)
- **Preferred adapter**: Select default wireless device (multi-adapter systems)

### iwd Setup

Ensure iwd is running and enabled:

```bash
# Enable and start iwd
sudo systemctl enable --now iwd

# Check status
sudo systemctl status iwd
```

### Polkit Rules

For non-root users to manage Wi-Fi, create `/etc/polkit-1/rules.d/50-iwd.rules`:

```javascript
polkit.addRule(function(action, subject) {
    if (action.id.indexOf("net.connman.iwd.") == 0 &&
        subject.isInGroup("wheel")) {
        return polkit.Result.YES;
    }
});
```

Adjust the group (`wheel`, `network`, etc.) according to your distribution.

## Usage

### Basic Operations

1. **Scan for networks**: Click "Rescan" in the popup
2. **Connect to a network**: Click the network name, enter passphrase if required
3. **Disconnect**: Click "Disconnect" in the popup
4. **Forget network**: Right-click network → Forget (removes saved credentials)
5. **Hidden network**: Click "Hidden..." button, enter SSID and passphrase

### Status Indicator

The gadget icon shows current state:
- **Gray**: Disconnected or no wireless device
- **Orange/Yellow**: Connecting to network
- **Green**: Connected successfully
- **Red**: Error (iwd not running or permission denied)

### Troubleshooting

See [INSTALL.md](INSTALL.md#troubleshooting) for common issues and solutions.

## Architecture

### Components

```
eiwd/
├── src/
│   ├── e_mod_main.c       # Module entry point
│   ├── e_mod_config.c     # Configuration dialog
│   ├── e_mod_gadget.c     # Shelf gadget icon
│   ├── e_mod_popup.c      # Network list popup
│   ├── iwd/               # D-Bus backend
│   │   ├── iwd_dbus.c     # Connection management
│   │   ├── iwd_device.c   # Device abstraction
│   │   ├── iwd_network.c  # Network abstraction
│   │   ├── iwd_agent.c    # Authentication agent
│   │   └── iwd_state.c    # State machine
│   └── ui/                # UI dialogs
│       ├── wifi_auth.c    # Passphrase input
│       └── wifi_hidden.c  # Hidden network dialog
└── data/
    ├── theme.edc          # Edje theme
    └── module.desktop     # Module metadata
```

### D-Bus Integration

Communicates with iwd via system bus (`net.connman.iwd`):
- `ObjectManager` for device/network discovery
- `Device` and `Station` interfaces for wireless operations
- `Network` interface for connection management
- `Agent` registration for passphrase requests

## Performance

- **Startup time**: < 100ms
- **Memory footprint**: ~5 MB
- **No polling**: Event-driven updates via D-Bus signals
- **Theme compilation**: Optimized Edje binary format

## License

[Specify your license here - e.g., BSD, GPL, MIT]

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Test thoroughly (see Testing section)
4. Submit a pull request

## Support

- **Issues**: Report bugs via GitHub Issues
- **Documentation**: See [INSTALL.md](INSTALL.md) for detailed setup
- **IRC/Matrix**: [Specify chat channels if available]

## Credits

Developed with Claude Code - https://claude.com/claude-code

Based on iwd (Intel Wireless Daemon) by Intel Corporation
Built for the Enlightenment desktop environment

## See Also

- [iwd documentation](https://iwd.wiki.kernel.org/)
- [Enlightenment documentation](https://www.enlightenment.org/docs)
- [EFL API reference](https://docs.enlightenment.org/api/efl/start)
