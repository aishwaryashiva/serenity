# SerenityOS Web Desktop Implementation

This implementation replaces the traditional SerenityOS desktop with a fullscreen web browser experience.

## What Was Implemented

### 1. WebDesktop Service
- **Location**: `Userland/Services/WebDesktop/`
- **Purpose**: Launches browser in fullscreen kiosk mode
- **Features**: 
  - Configurable target URL
  - Kiosk mode (no navigation controls)
  - Fullscreen by default

### 2. Modified SystemServer Configuration
- **File**: `Base/etc/SystemServer.ini`
- **Changes**:
  - Added WebDesktop service for graphical mode
  - Disabled WindowServer in graphical mode (moved to text mode only)
  - Disabled Taskbar and Desktop in user mode

### 3. Enhanced Browser for Kiosk Mode
- **Files**: `Userland/Applications/Browser/`
- **New Features**:
  - `--fullscreen` flag to start in fullscreen
  - `--kiosk` flag for kiosk mode
  - `--disable-navigation` flag to disable navigation controls
  - Hides all UI elements (toolbar, status bar, tab bar) in kiosk mode
  - Prevents window resizing in kiosk mode

### 4. Modified LoginServer
- **File**: `Userland/Services/LoginServer/main.cpp`
- **Changes**:
  - Checks for `WEB_DESKTOP_MODE` environment variable
  - Launches WebDesktop instead of SystemServer when enabled
  - Configurable via `WEB_DESKTOP_URL` environment variable

## Configuration

### Environment Variables
- `WEB_DESKTOP_MODE=1`: Enables web desktop mode
- `WEB_DESKTOP_URL=https://your-app.com`: Sets the target URL

### SystemServer Configuration
```ini
[WebDesktop]
Executable=/bin/WebDesktop
Arguments=--url https://example.com --kiosk
User=window
SystemModes=graphical
KeepAlive=true
Priority=high
StdIO=/dev/tty0
```

## How It Works

1. **Boot Process**: 
   - Kernel loads → SystemServer starts → LoginServer launches
   - LoginServer checks for `WEB_DESKTOP_MODE` environment variable
   - If enabled, launches WebDesktop instead of traditional desktop

2. **WebDesktop Service**:
   - Launches Browser with fullscreen and kiosk flags
   - Browser hides all UI elements and loads the specified URL
   - User sees only the web content in fullscreen

3. **Kiosk Mode**:
   - No navigation controls (back, forward, home, reload)
   - No toolbar, status bar, or tab bar
   - No window resizing or moving
   - Fullscreen only

## Benefits

- **No Traditional Desktop**: Users see only the web application
- **Kiosk Mode**: Prevents users from navigating away or accessing system
- **Configurable**: Easy to change target URL via environment variables
- **All Drivers Load**: Graphics, network, and other drivers still initialize normally
- **Fullscreen Experience**: Web app appears as the entire desktop

## Usage

### To Enable Web Desktop Mode:
1. Set environment variables in `Base/etc/SystemServer.ini`:
   ```ini
   [LoginServer]
   Environment=WEB_DESKTOP_MODE=1 WEB_DESKTOP_URL=https://your-app.com
   ```

2. Build and run SerenityOS:
   ```bash
   Meta/serenity.sh build
   Meta/serenity.sh run
   ```

### To Disable Web Desktop Mode:
1. Remove or comment out the `Environment` line in `Base/etc/SystemServer.ini`
2. Rebuild and run

## Customization

### Change Target URL:
Modify the `WEB_DESKTOP_URL` environment variable in `Base/etc/SystemServer.ini`

### Disable Kiosk Mode:
Remove the `--kiosk` flag from WebDesktop arguments in `Base/etc/SystemServer.ini`

### Add More Browser Options:
Modify the `Arguments` line in the WebDesktop service configuration

## Files Modified

1. `Userland/Services/WebDesktop/main.cpp` - New WebDesktop service
2. `Userland/Services/WebDesktop/CMakeLists.txt` - Build configuration
3. `Userland/Services/CMakeLists.txt` - Added WebDesktop to build
4. `Base/etc/SystemServer.ini` - System service configuration
5. `Base/etc/SystemServerUser.ini` - User service configuration
6. `Userland/Applications/Browser/main.cpp` - Added kiosk mode flags
7. `Userland/Applications/Browser/Browser.h` - Added global kiosk variables
8. `Userland/Applications/Browser/BrowserWindow.cpp` - Kiosk mode UI handling
9. `Userland/Applications/Browser/Tab.cpp` - Hide UI elements in kiosk mode
10. `Userland/Services/LoginServer/main.cpp` - WebDesktop launch logic

## Testing

To test the implementation:

1. Build the system:
   ```bash
   Meta/serenity.sh build
   ```

2. Run in QEMU:
   ```bash
   Meta/serenity.sh run
   ```

3. The system should boot directly to a fullscreen web browser showing the configured URL

4. To test with a different URL, modify `WEB_DESKTOP_URL` in the configuration and rebuild

## Troubleshooting

- **Browser doesn't start**: Check that WebDesktop service is properly configured
- **URL doesn't load**: Verify network connectivity and URL validity
- **UI elements still visible**: Ensure kiosk mode flags are properly set
- **System hangs**: Check that all required services are still running

## Future Enhancements

- Add configuration file for WebDesktop settings
- Support for multiple URLs or web apps
- Add keyboard shortcuts for system access
- Implement session management
- Add logging and monitoring capabilities