# WebDesktop Offline Functionality

This document describes the offline functionality implemented in WebDesktop to handle scenarios where there is no internet connection.

## Overview

When WebDesktop detects that there is no internet connection, it automatically switches to a local offline error page that provides:

- **Network status monitoring** - Real-time display of connection status
- **Troubleshooting tools** - Built-in network diagnostic and repair tools
- **System access** - Access to local applications and system information
- **Network configuration** - Ability to configure network settings
- **Recovery assistance** - Automatic detection when connection is restored

## How It Works

### 1. Network Detection

WebDesktop uses a `NetworkDetector` class that continuously monitors:

- **Internet connectivity** - Tests connection to external servers
- **Local network status** - Checks local network interface status
- **DNS resolution** - Verifies DNS server functionality
- **WebDesktop service** - Ensures the message handler is running

### 2. Automatic Fallback

When no internet connection is detected:

1. WebDesktop automatically loads the offline error page
2. The page is served locally from `/usr/share/webdesktop/OfflineErrorPage.html`
3. All system functionality remains available through the message handler
4. Network status is continuously monitored for recovery

### 3. Offline Error Page Features

The offline error page provides:

#### Network Status Display
- Real-time status of internet, local network, DNS, and WebDesktop service
- Visual indicators (green/red/yellow) for connection status
- Detailed information about network configuration

#### Troubleshooting Tools
- **Retry Connection** - Manually retry network connection
- **Network Settings** - Configure IP, gateway, DNS servers
- **Restart Network** - Restart network services
- **System Information** - View system and network details

#### System Access
- **Local Apps** - Launch local applications
- **System Logs** - View system and network logs
- **Support** - Access support information

## Configuration

### Command Line Options

```bash
# Enable network checking (default: true)
/bin/WebDesktop --check-network

# Disable network checking
/bin/WebDesktop --no-check-network

# Specify custom URL
/bin/WebDesktop --url https://your-app.com --check-network

# Enable message handler for offline functionality
/bin/WebDesktop --messages --check-network
```

### Configuration File

Edit `/etc/webdesktop.conf` to configure offline behavior:

```json
{
  "security": {
    "allowedOrigins": [
      "file:///usr/share/webdesktop/OfflineErrorPage.html"
    ],
    "requireHttps": false,
    "allowFileProtocol": true
  },
  "system": {
    "enableNetworkManagement": true,
    "enableSystemInfo": true,
    "enableLogs": true
  }
}
```

## Network Detection Details

### Connectivity Tests

The system performs the following tests:

1. **Internet Test** - Attempts to connect to `https://httpbin.org/status/200`
2. **Local Network Test** - Checks local network interface status
3. **DNS Test** - Tries to resolve `google.com`
4. **Service Test** - Verifies WebDesktop message handler is running

### Network Information

The system collects and displays:

- **Interface Name** - Network interface (eth0, wlan0, etc.)
- **IP Address** - Current IP address
- **Gateway** - Network gateway
- **DNS Servers** - Configured DNS servers
- **MAC Address** - Network interface MAC address
- **Signal Strength** - WiFi signal strength (if applicable)

## Troubleshooting Commands

### Available Commands

The offline page can send the following commands to WebDesktop:

#### Network Management
- `configureNetwork` - Configure network settings
- `resetNetwork` - Reset to default settings
- `restartNetwork` - Restart network services

#### System Access
- `openLocalApp` - Launch local application
- `openSystemInfo` - Show system information
- `openLogs` - View system logs
- `contactSupport` - Access support

#### System Control
- `shutdown` - Shutdown system
- `restart` - Restart system
- `logout` - Logout user
- `lock` - Lock screen

### Message Format

Commands are sent as JSON messages:

```json
{
  "command": "configureNetwork",
  "data": {
    "interface": "eth0",
    "ipAddress": "192.168.1.100",
    "subnetMask": "255.255.255.0",
    "gateway": "192.168.1.1",
    "dnsServers": "8.8.8.8,8.8.4.4"
  },
  "origin": "file:///usr/share/webdesktop/OfflineErrorPage.html",
  "targetOrigin": "*"
}
```

## Testing

### Manual Testing

1. **Disconnect network** - Unplug ethernet cable or disable WiFi
2. **Start WebDesktop** - Run `/bin/WebDesktop --check-network`
3. **Observe offline page** - Should show network error page
4. **Test functionality** - Try troubleshooting tools
5. **Reconnect network** - Should automatically detect recovery

### Automated Testing

Run the test suite:

```bash
# Run offline functionality tests
/usr/share/webdesktop/test_offline.sh
```

The test suite covers:
- Normal operation with internet
- Offline operation detection
- Network recovery
- Local error page loading
- Network configuration
- System commands
- Network troubleshooting

## Implementation Details

### Files Structure

```
/usr/share/webdesktop/
├── OfflineErrorPage.html          # Offline error page
└── test_offline.sh               # Test suite

/etc/
└── webdesktop.conf               # Configuration file

/tmp/
└── webdesktop-messages           # Message handler socket
```

### Key Components

1. **NetworkDetector** - Monitors network connectivity
2. **MessageHandler** - Handles system commands
3. **OfflineErrorPage** - Local HTML page with troubleshooting UI
4. **WebDesktopConfig** - Configuration management

### Security Considerations

- Offline page runs with local file protocol
- System commands require proper authentication
- Network configuration is restricted to authorized users
- Message handler validates all incoming commands

## Troubleshooting

### Common Issues

#### Offline Page Not Loading
- Check that `/usr/share/webdesktop/OfflineErrorPage.html` exists
- Verify file permissions are readable
- Ensure WebDesktop is started with `--check-network`

#### Network Detection Not Working
- Check that network interfaces are active
- Verify DNS servers are configured
- Test manual connectivity to external servers

#### Message Handler Not Responding
- Check that `/tmp/webdesktop-messages` socket exists
- Verify WebDesktop is started with `--messages`
- Check system logs for errors

#### System Commands Not Working
- Verify authentication token (if required)
- Check command permissions in configuration
- Ensure target applications exist

### Debug Mode

Enable debug logging:

```bash
# Start with debug output
RUST_LOG=debug /bin/WebDesktop --check-network --messages
```

### Log Files

Check system logs for WebDesktop messages:

```bash
# View WebDesktop logs
journalctl -u webdesktop

# View network logs
journalctl -u NetworkManager
```

## Future Enhancements

### Planned Features

1. **WiFi Configuration** - GUI for WiFi setup
2. **Network Profiles** - Save/load network configurations
3. **Advanced Diagnostics** - Network performance testing
4. **Remote Support** - Remote troubleshooting capabilities
5. **Offline Apps** - Local applications for offline use

### Contributing

To contribute to offline functionality:

1. Fork the SerenityOS repository
2. Create a feature branch
3. Implement your changes
4. Add tests for new functionality
5. Submit a pull request

## License

This offline functionality is part of SerenityOS and is licensed under the BSD-2-Clause License.