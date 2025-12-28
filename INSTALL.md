# Installation Guide - eiwd

Detailed installation instructions for the eiwd Enlightenment Wi-Fi module.

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Building from Source](#building-from-source)
3. [Installation](#installation)
4. [Configuration](#configuration)
5. [Troubleshooting](#troubleshooting)
6. [Uninstallation](#uninstallation)

## System Requirements

### Supported Distributions

eiwd has been tested on:
- Arch Linux
- Gentoo Linux
- Debian/Ubuntu (with manual EFL installation)
- Fedora

### Minimum Versions

| Component | Minimum Version | Recommended |
|-----------|----------------|-------------|
| Enlightenment | 0.25.x | 0.27.x |
| EFL | 1.26.x | 1.28.x |
| iwd | 1.0 | Latest stable |
| Linux kernel | 4.14+ | 5.4+ |
| D-Bus | 1.10+ | 1.14+ |

### Wireless Hardware

Any wireless adapter supported by the Linux kernel and iwd:
- Intel Wi-Fi (best support)
- Atheros (ath9k, ath10k)
- Realtek (rtl8xxx series)
- Broadcom (with appropriate drivers)

Check compatibility: `iwd --version` and `iwctl device list`

## Building from Source

### 1. Install Build Dependencies

#### Arch Linux
```bash
sudo pacman -S base-devel meson ninja enlightenment efl iwd
```

#### Gentoo
```bash
sudo emerge --ask dev-util/meson dev-util/ninja enlightenment efl net-wireless/iwd
```

#### Debian/Ubuntu
```bash
sudo apt install build-essential meson ninja-build \
    libefl-all-dev enlightenment-dev iwd
```

#### Fedora
```bash
sudo dnf install @development-tools meson ninja-build \
    efl-devel enlightenment-devel iwd
```

### 2. Get Source Code

```bash
# Clone repository
git clone <repository-url> eiwd
cd eiwd

# Or extract tarball
tar xzf eiwd-0.1.0.tar.gz
cd eiwd-0.1.0
```

### 3. Configure Build

```bash
# Default configuration
meson setup build

# Custom options
meson setup build \
    --prefix=/usr \
    --libdir=lib64 \
    -Dnls=true
```

Available options:
- `--prefix=PATH`: Installation prefix (default: `/usr/local`)
- `--libdir=NAME`: Library directory name (auto-detected)
- `-Dnls=BOOL`: Enable/disable translations (default: `true`)

### 4. Compile

```bash
ninja -C build
```

Expected output:
```
[14/14] Linking target src/module.so
```

Verify compilation:
```bash
ls -lh build/src/module.so build/data/e-module-iwd.edj
```

Should show:
- `module.so`: ~230-240 KB
- `e-module-iwd.edj`: ~10-12 KB

## Installation

### System-Wide Installation

```bash
# Install module
sudo ninja -C build install

# On some systems, update library cache
sudo ldconfig
```

### Installation Paths

Default paths (with `--prefix=/usr`):
```
/usr/lib64/enlightenment/modules/iwd/
├── linux-x86_64-0.27/
│   ├── module.so
│   └── e-module-iwd.edj
└── module.desktop
```

The architecture suffix (`linux-x86_64-0.27`) matches your Enlightenment version.

### Verify Installation

```bash
# Check files
ls -R /usr/lib64/enlightenment/modules/iwd/

# Check module metadata
cat /usr/lib64/enlightenment/modules/iwd/module.desktop
```

## Configuration

### 1. Enable the Module

#### Via GUI:
1. Open Enlightenment Settings (Settings → Modules)
2. Find "IWD" or "IWD Wi-Fi" in the list
3. Select it and click "Load"
4. The module should show "Running" status

#### Via Command Line:
```bash
# Enable module
enlightenment_remote -module-load iwd

# Verify
enlightenment_remote -module-list | grep iwd
```

### 2. Add Gadget to Shelf

1. Right-click on your shelf → Shelf → Contents
2. Find "IWD Wi-Fi" in the gadget list
3. Click to add it to the shelf
4. Position it as desired

Alternatively:
1. Right-click shelf → Add → Gadget → IWD Wi-Fi

### 3. Configure iwd Service

```bash
# Enable iwd service
sudo systemctl enable iwd.service

# Start iwd
sudo systemctl start iwd.service

# Check status
systemctl status iwd.service
```

Expected output should show "active (running)".

### 4. Disable Conflicting Services

iwd conflicts with wpa_supplicant and NetworkManager's Wi-Fi management:

```bash
# Stop and disable wpa_supplicant
sudo systemctl stop wpa_supplicant.service
sudo systemctl disable wpa_supplicant.service

# If using NetworkManager, configure it to ignore Wi-Fi
sudo mkdir -p /etc/NetworkManager/conf.d/
cat << EOF | sudo tee /etc/NetworkManager/conf.d/wifi-backend.conf
[device]
wifi.backend=iwd
EOF

sudo systemctl restart NetworkManager
```

Or disable NetworkManager entirely:
```bash
sudo systemctl stop NetworkManager
sudo systemctl disable NetworkManager
```

### 5. Configure Permissions

#### Polkit Rules

Create `/etc/polkit-1/rules.d/50-iwd.rules`:

```javascript
/* Allow users in 'wheel' group to manage Wi-Fi */
polkit.addRule(function(action, subject) {
    if (action.id.indexOf("net.connman.iwd.") == 0) {
        if (subject.isInGroup("wheel")) {
            return polkit.Result.YES;
        }
    }
});
```

Adjust group as needed:
- Arch/Gentoo: `wheel`
- Debian/Ubuntu: `sudo` or `netdev`
- Fedora: `wheel`

Reload polkit:
```bash
sudo systemctl restart polkit
```

#### Alternative: D-Bus Policy

Create `/etc/dbus-1/system.d/iwd-custom.conf`:

```xml
<!DOCTYPE busconfig PUBLIC
 "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <policy group="wheel">
    <allow send_destination="net.connman.iwd"/>
    <allow send_interface="net.connman.iwd.Agent"/>
  </policy>
</busconfig>
```

Reload D-Bus:
```bash
sudo systemctl reload dbus
```

### 6. Module Configuration

Right-click the gadget → Configure, or:
Settings → Modules → IWD → Configure

Options:
- **Auto-connect to known networks**: Enabled by default
- **Show hidden networks**: Show "Hidden..." button
- **Signal refresh interval**: Update frequency (default: 5s)
- **Preferred adapter**: For systems with multiple wireless cards

Configuration is saved automatically to:
`~/.config/enlightenment/module.iwd.cfg`

## Troubleshooting

### Module Won't Load

**Symptom**: Module appears in list but won't load, or crashes on load.

**Solutions**:
```bash
# Check Enlightenment logs
tail -f ~/.cache/enlightenment/enlightenment.log

# Verify module dependencies
ldd /usr/lib64/enlightenment/modules/iwd/linux-x86_64-0.27/module.so

# Ensure iwd is running
systemctl status iwd

# Check D-Bus connection
dbus-send --system --print-reply \
    --dest=net.connman.iwd \
    / org.freedesktop.DBus.Introspectable.Introspect
```

### No Wireless Devices Shown

**Symptom**: Gadget shows "No device" or red error state.

**Solutions**:
```bash
# Check if iwd sees the device
iwctl device list

# Verify wireless is powered on
rfkill list
# If blocked:
rfkill unblock wifi

# Check kernel driver
lspci -k | grep -A 3 Network

# Ensure device is not managed by other services
nmcli device status  # Should show "unmanaged"
```

### Permission Denied Errors

**Symptom**: Cannot scan or connect, error messages mention "NotAuthorized".

**Solutions**:
```bash
# Check polkit rules
ls -la /etc/polkit-1/rules.d/50-iwd.rules

# Verify group membership
groups $USER

# Test D-Bus permissions manually
gdbus call --system \
    --dest net.connman.iwd \
    --object-path /net/connman/iwd \
    --method org.freedesktop.DBus.Introspectable.Introspect

# Check audit logs (if available)
sudo journalctl -u polkit --since "1 hour ago"
```

### Connection Failures

**Symptom**: Can scan but cannot connect to networks.

**Solutions**:
```bash
# Enable iwd debug logging
sudo systemctl edit iwd.service
# Add:
# [Service]
# ExecStart=
# ExecStart=/usr/lib/iwd/iwd --debug

sudo systemctl daemon-reload
sudo systemctl restart iwd

# Check logs
sudo journalctl -u iwd -f

# Test connection manually
iwctl station wlan0 connect "Your SSID"

# Verify known networks
ls /var/lib/iwd/
```

### Theme Not Loading

**Symptom**: Gadget appears as colored rectangles instead of proper icons.

**Solutions**:
```bash
# Verify theme file exists
ls -la /usr/lib64/enlightenment/modules/iwd/*/e-module-iwd.edj

# Check file permissions
chmod 644 /usr/lib64/enlightenment/modules/iwd/*/e-module-iwd.edj

# Test theme manually
edje_player /usr/lib64/enlightenment/modules/iwd/*/e-module-iwd.edj

# Reinstall
sudo ninja -C build install
```

### Gadget Not Updating

**Symptom**: Connection state doesn't change or networks don't appear.

**Solutions**:
```bash
# Check D-Bus signals are being received
dbus-monitor --system "interface='net.connman.iwd.Device'"

# Verify signal refresh interval (should be 1-60 seconds)
# Increase in module config if too low

# Restart module
enlightenment_remote -module-unload iwd
enlightenment_remote -module-load iwd
```

### iwd Daemon Crashes

**Symptom**: Gadget shows red error state intermittently.

**Solutions**:
```bash
# Check iwd logs
sudo journalctl -u iwd --since "30 minutes ago"

# Update iwd
sudo pacman -Syu iwd  # Arch
sudo emerge --update iwd  # Gentoo

# Report bug with:
# - iwd version: iwd --version
# - Kernel version: uname -r
# - Wireless chip: lspci | grep Network
```

### Common Error Messages

| Error Message | Cause | Solution |
|---------------|-------|----------|
| "Wi-Fi daemon (iwd) has stopped" | iwd service not running | `sudo systemctl start iwd` |
| "Permission Denied" dialog | Polkit rules not configured | Set up polkit (see Configuration) |
| "Failed to connect: operation failed" | Wrong password | Re-enter passphrase |
| "No device" in gadget | No wireless adapter detected | Check `rfkill`, drivers |

## Uninstallation

### Remove Module

```bash
# From build directory
sudo ninja -C build uninstall
```

### Manual Removal

```bash
# Remove module files
sudo rm -rf /usr/lib64/enlightenment/modules/iwd

# Remove configuration
rm -f ~/.config/enlightenment/module.iwd.cfg
```

### Cleanup

```bash
# Re-enable previous Wi-Fi manager
sudo systemctl enable --now wpa_supplicant
# or
sudo systemctl enable --now NetworkManager
```

## Getting Help

If problems persist:

1. Check logs: `~/.cache/enlightenment/enlightenment.log`
2. Enable debug mode (see Troubleshooting)
3. Report issue with:
   - Distribution and version
   - Enlightenment version: `enlightenment -version`
   - EFL version: `pkg-config --modversion elementary`
   - iwd version: `iwd --version`
   - Wireless chipset: `lspci | grep -i network`
   - Relevant log output

## Next Steps

After successful installation:
- Read [README.md](README.md) for usage instructions
- Configure module settings to your preferences
- Add networks and test connectivity
- Customize theme (advanced users)
