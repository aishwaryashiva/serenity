# WebDesktop Message Handler

This service enables web pages to send system commands to SerenityOS through a secure message passing system.

## Features

- **Secure Communication**: Domain validation and authentication
- **System Commands**: Shutdown, restart, logout, lock, suspend, hibernate
- **Media Control**: Volume and brightness adjustment
- **Screenshot**: Take system screenshots
- **Configurable Security**: JSON-based configuration file

## Architecture

```
Web Page (JavaScript) 
    ↓ postMessage
Browser Tab
    ↓ Local Socket
WebDesktop MessageHandler
    ↓ System Commands
SerenityOS System
```

## Configuration

Edit `/etc/webdesktop.conf` to configure security settings:

```json
{
  "security": {
    "allowedOrigins": [
      "https://localhost",
      "https://127.0.0.1",
      "https://example.com"
    ],
    "requireHttps": true,
    "allowLocalhost": true,
    "allowFileProtocol": false,
    "requireAuthentication": false,
    "authenticationToken": "",
    "blockedCommands": []
  },
  "system": {
    "enableShutdown": true,
    "enableRestart": true,
    "enableLogout": true,
    "enableLock": true,
    "enableSuspend": false,
    "enableHibernate": false,
    "enableVolumeControl": true,
    "enableBrightnessControl": true,
    "enableScreenshot": true,
    "maxCommandsPerMinute": 10,
    "commandTimeoutSeconds": 30
  }
}
```

## Usage

### From JavaScript

```javascript
// Send a shutdown command
const message = {
    command: "shutdown",
    data: {
        authToken: "your-token-here" // if authentication is enabled
    },
    origin: window.location.origin,
    targetOrigin: "*"
};

// Send via postMessage to parent window or WebSocket
window.postMessage(message, "*");
```

### Available Commands

- `shutdown` - Shutdown the system
- `restart` - Restart the system
- `logout` - Logout current user
- `lock` - Lock the screen
- `suspend` - Suspend the system
- `hibernate` - Hibernate the system
- `volumeUp` - Increase volume
- `volumeDown` - Decrease volume
- `volumeMute` - Toggle mute
- `brightnessUp` - Increase brightness
- `brightnessDown` - Decrease brightness
- `screenshot` - Take a screenshot

## Security

### Origin Validation
- Only allows commands from configured origins
- Requires HTTPS by default (configurable)
- Supports localhost for development

### Authentication
- Optional token-based authentication
- Configurable per installation
- Token passed in message data

### Command Filtering
- Whitelist-based command filtering
- System-level permissions
- Rate limiting support

## Building

The WebDesktop service is built as part of the SerenityOS build system:

```bash
# Build the entire system
ninja

# Or build just the WebDesktop service
ninja WebDesktop
```

## Installation

1. Build SerenityOS with WebDesktop support
2. Configure `/etc/webdesktop.conf`
3. Start WebDesktop service with message handler enabled:
   ```bash
   /bin/WebDesktop --url https://your-web-app.com --messages
   ```

## Example Web Page

See `example.html` for a complete example of a web-based system control interface.

## Troubleshooting

### Commands Not Working
1. Check that the origin is in `allowedOrigins`
2. Verify the command is not in `blockedCommands`
3. Ensure authentication token is correct (if required)
4. Check system permissions for the command

### Connection Issues
1. Verify WebDesktop service is running
2. Check socket permissions at `/tmp/webdesktop-messages`
3. Ensure browser is in kiosk mode for message handling

### Security Warnings
1. Use HTTPS for production deployments
2. Set strong authentication tokens
3. Limit allowed origins to trusted domains
4. Regularly review and update blocked commands

## Development

### Adding New Commands

1. Add command to `SystemCommand` enum in `MessageHandler.h`
2. Implement handler in `MessageHandler.cpp`
3. Add parsing logic in `parse_command()`
4. Update configuration schema if needed
5. Test with example web page

### Customizing Security

1. Modify `WebDesktopConfig.cpp` for new security features
2. Update configuration file format
3. Add validation logic in `MessageHandler.cpp`
4. Test with various origin scenarios

## License

BSD-2-Clause License - see SerenityOS license for details.