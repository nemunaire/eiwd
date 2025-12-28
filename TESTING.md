# Testing Checklist - eiwd

Manual testing checklist for verifying eiwd functionality.

## Pre-Testing Setup

### Environment Verification

- [ ] iwd service is running: `systemctl status iwd`
- [ ] Wireless device is detected: `iwctl device list`
- [ ] D-Bus connection works: `dbus-send --system --dest=net.connman.iwd --print-reply / org.freedesktop.DBus.Introspectable.Introspect`
- [ ] No conflicting services (wpa_supplicant, NetworkManager Wi-Fi)
- [ ] User has proper permissions (polkit rules configured)

### Build Verification

```bash
# Clean build
rm -rf build
meson setup build
ninja -C build

# Verify artifacts
ls -lh build/src/module.so          # Should be ~230KB
ls -lh build/data/e-module-iwd.edj  # Should be ~10-12KB

# Check for warnings
ninja -C build 2>&1 | grep -i warning
```

Expected: No critical warnings, module compiles successfully.

## Module Loading Tests

### Basic Loading

- [ ] Module loads without errors: `enlightenment_remote -module-load iwd`
- [ ] Module appears in Settings → Modules
- [ ] Module shows "Running" status
- [ ] No errors in `~/.cache/enlightenment/enlightenment.log`
- [ ] Gadget can be added to shelf

### Initialization

- [ ] Gadget icon appears on shelf after adding
- [ ] Icon shows appropriate initial state (gray/disconnected or green/connected)
- [ ] Tooltip displays correctly on hover
- [ ] No crashes or freezes after loading

## UI Interaction Tests

### Gadget

- [ ] Left-click opens popup menu
- [ ] Icon color reflects current state:
  - Gray: Disconnected
  - Orange/Yellow: Connecting
  - Green: Connected
  - Red: Error (iwd not running)
- [ ] Tooltip shows correct information:
  - Current SSID (if connected)
  - Signal strength
  - Connection status
- [ ] Multiple clicks toggle popup open/close without issues

### Popup Menu

- [ ] Popup appears at correct position near gadget
- [ ] Current connection section shows:
  - Connected SSID (if applicable)
  - Signal strength
  - Disconnect button (when connected)
- [ ] Available networks list displays:
  - Network SSIDs
  - Security type indicators (lock icons)
  - Signal strength
  - Known networks marked/sorted appropriately
- [ ] Action buttons present:
  - "Rescan" button
  - "Hidden..." button (if enabled in config)
  - "Enable/Disable Wi-Fi" button
- [ ] Popup stays open when interacting with widgets
- [ ] Clicking outside popup closes it

### Configuration Dialog

- [ ] Config dialog opens from module settings
- [ ] All settings visible:
  - Auto-connect checkbox
  - Show hidden networks checkbox
  - Signal refresh slider
  - Adapter selection (if multiple devices)
- [ ] Changes save correctly
- [ ] Applied settings persist after restart
- [ ] Dialog can be closed with OK/Cancel
- [ ] Multiple opens don't create duplicate dialogs

## Network Operations Tests

### Scanning

- [ ] Manual scan via "Rescan" button works
- [ ] Networks appear in list after scan
- [ ] List updates showing new networks
- [ ] Signal strength values reasonable (-30 to -90 dBm)
- [ ] Duplicate networks not shown
- [ ] Scan doesn't freeze UI
- [ ] Periodic auto-refresh works (based on config interval)

### Connecting to Open Network

- [ ] Click on open network initiates connection
- [ ] Icon changes to "connecting" state (orange)
- [ ] No passphrase dialog appears
- [ ] Connection succeeds within 10 seconds
- [ ] Icon changes to "connected" state (green)
- [ ] Tooltip shows connected SSID
- [ ] Current connection section updated in popup

### Connecting to Secured Network (WPA2/WPA3)

- [ ] Click on secured network opens passphrase dialog
- [ ] Dialog shows network name
- [ ] Password field is hidden (dots/asterisks)
- [ ] Entering correct passphrase connects successfully
- [ ] Wrong passphrase shows error message
- [ ] Cancel button closes dialog without connecting
- [ ] Connection state updates correctly
- [ ] Passphrase is not logged to any logs

### Disconnecting

- [ ] "Disconnect" button appears when connected
- [ ] Clicking disconnect terminates connection
- [ ] Icon changes to disconnected state
- [ ] Current connection section clears
- [ ] No error messages on clean disconnect

### Forgetting Network

- [ ] Known networks can be forgotten (via context menu or dedicated UI)
- [ ] Forgetting removes from known list
- [ ] Network still appears in scan results (as unknown)
- [ ] Auto-connect disabled after forgetting

### Hidden Networks

- [ ] "Hidden..." button opens dialog
- [ ] Can enter SSID manually
- [ ] Passphrase field available for secured networks
- [ ] Connection attempt works correctly
- [ ] Error handling for non-existent SSID
- [ ] Successfully connected hidden network saved

## State Management Tests

### Connection States

- [ ] OFF state: Wi-Fi powered off, icon gray
- [ ] IDLE state: Wi-Fi on but disconnected, icon gray
- [ ] SCANNING state: Scan in progress
- [ ] CONNECTING state: Connection attempt, icon orange
- [ ] CONNECTED state: Active connection, icon green
- [ ] ERROR state: iwd not running, icon red

### Transitions

- [ ] Disconnected → Connecting → Connected works smoothly
- [ ] Connected → Disconnecting → Disconnected works smoothly
- [ ] Error → Idle when iwd starts
- [ ] UI updates reflect state changes within 1-2 seconds

## Advanced Features Tests

### Multiple Adapters

If system has multiple wireless devices:
- [ ] Both devices detected
- [ ] Can select preferred adapter in config
- [ ] Switching adapters works correctly
- [ ] Each adapter shows separate networks

### iwd Daemon Restart

```bash
# While module is running and connected
sudo systemctl restart iwd
```

- [ ] Gadget shows error state (red) when iwd stops
- [ ] Error dialog appears notifying daemon stopped
- [ ] Automatic reconnection when iwd restarts
- [ ] Agent re-registers successfully
- [ ] Can reconnect to networks after restart
- [ ] No module crashes

### Auto-Connect

- [ ] Enable auto-connect in config
- [ ] Disconnect from current network
- [ ] Module reconnects automatically to known network
- [ ] Disable auto-connect prevents automatic connection
- [ ] Auto-connect works after system restart

### Polkit Permission Errors

```bash
# Temporarily break polkit rules
sudo mv /etc/polkit-1/rules.d/50-iwd.rules /tmp/
```

- [ ] Permission denied error shows user-friendly message
- [ ] Error dialog suggests polkit configuration
- [ ] Module doesn't crash
- [ ] Restoring rules allows operations again

## Error Handling Tests

### No Wireless Device

```bash
# Simulate by blocking with rfkill
sudo rfkill block wifi
```

- [ ] Gadget shows appropriate state
- [ ] Error message clear to user
- [ ] Unblocking device recovers gracefully

### Wrong Password

- [ ] Entering wrong WPA password shows error
- [ ] Error message is helpful (not just "Failed")
- [ ] Can retry with different password
- [ ] Multiple failures don't crash module

### Network Out of Range

- [ ] Attempting to connect to weak/distant network
- [ ] Timeout handled gracefully
- [ ] Error message explains problem

### iwd Not Running

```bash
sudo systemctl stop iwd
```

- [ ] Gadget immediately shows error state
- [ ] User-friendly error dialog
- [ ] Instructions to start iwd service
- [ ] Module continues running (no crash)

## Performance Tests

### Responsiveness

- [ ] Popup opens within 200ms of click
- [ ] Network list populates within 500ms
- [ ] UI remains responsive during scan
- [ ] No freezing during connect operations
- [ ] Configuration dialog opens quickly

### Resource Usage

```bash
# Check memory usage
ps aux | grep enlightenment
```

- [ ] Module uses < 10 MB RAM
- [ ] No memory leaks after multiple connect/disconnect cycles
- [ ] CPU usage < 1% when idle
- [ ] CPU spike during scan acceptable (< 3 seconds)

### Stability

- [ ] No crashes after 10 connect/disconnect cycles
- [ ] Module stable for 1+ hour of operation
- [ ] Theme rendering consistent
- [ ] No visual glitches in popup

## Theme Tests

### Visual Appearance

- [ ] Theme file loads successfully
- [ ] Icon appearance matches theme groups
- [ ] Colors appropriate for each state
- [ ] Signal strength indicator displays
- [ ] Theme scales properly with shelf size
- [ ] Theme works in different Enlightenment themes

### Fallback Behavior

```bash
# Rename theme to simulate missing
sudo mv /usr/lib*/enlightenment/modules/iwd/*/e-module-iwd.edj \
         /usr/lib*/enlightenment/modules/iwd/*/e-module-iwd.edj.bak
```

- [ ] Module still functions with colored rectangles
- [ ] No crashes due to missing theme
- [ ] Warning logged about missing theme
- [ ] Restoring theme works after module reload

## Integration Tests

### Suspend/Resume

```bash
# Trigger system suspend
systemctl suspend
```

After resume:
- [ ] Module still functional
- [ ] Reconnects to previous network
- [ ] No errors in logs

### Multiple Instances

- [ ] Can add multiple gadgets to different shelves
- [ ] Each instance updates independently
- [ ] Removing one doesn't affect others
- [ ] All instances show same connection state

### Configuration Persistence

- [ ] Settings saved to `~/.config/enlightenment/module.iwd.cfg`
- [ ] Settings persist across Enlightenment restarts
- [ ] Settings persist across system reboots
- [ ] Corrupted config file handled gracefully

## Regression Tests

After code changes, verify:

### Core Functionality

- [ ] Module loads
- [ ] Can scan networks
- [ ] Can connect to WPA2 network
- [ ] Can disconnect
- [ ] Configuration dialog works

### No New Issues

- [ ] No new compiler warnings
- [ ] No new memory leaks (valgrind)
- [ ] No new crashes in logs
- [ ] Documentation still accurate

## Memory Leak Testing

```bash
# Run Enlightenment under Valgrind (slow!)
valgrind --leak-check=full \
         --track-origins=yes \
         --log-file=valgrind.log \
         enlightenment_start

# Perform operations:
# - Load module
# - Scan networks
# - Connect/disconnect 5 times
# - Open config dialog
# - Unload module

# Check results
grep "definitely lost" valgrind.log
grep "indirectly lost" valgrind.log
```

Expected: No memory leaks from eiwd code (EFL/E leaks may exist).

## Cleanup After Testing

```bash
# Restore any changed files
sudo systemctl start iwd
sudo rfkill unblock wifi

# Restore polkit rules if moved
sudo mv /tmp/50-iwd.rules /etc/polkit-1/rules.d/

# Restore theme if renamed
# ...

# Clear test networks
sudo rm /var/lib/iwd/TestNetwork.psk
```

## Test Report Template

```
## Test Report - eiwd v0.1.0

**Date**: YYYY-MM-DD
**Tester**: Name
**System**: Distribution, Kernel version
**E Version**: 0.27.x
**iwd Version**: X.XX

### Summary
- Tests Passed: XX/YY
- Tests Failed: Z
- Critical Issues: N

### Failed Tests
1. Test name: Description of failure
2. ...

### Notes
- Any observations
- Performance metrics
- Suggestions

### Conclusion
[Pass/Fail/Conditional Pass]
```

## Automated Testing (Future)

Placeholder for unit tests:

```c
// tests/test_network.c
// Basic functionality tests

#include <check.h>
#include "iwd_network.h"

START_TEST(test_network_creation)
{
    IWD_Network *net = iwd_network_new("/test/path");
    ck_assert_ptr_nonnull(net);
    iwd_network_free(net);
}
END_TEST

// More tests...
```

Build and run:
```bash
meson test -C build
```
