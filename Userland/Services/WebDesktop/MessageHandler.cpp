/*
 * Copyright (c) 2024, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "MessageHandler.h"
#include "WebDesktopConfig.h"
#include <AK/JsonParser.h>
#include <AK/URL.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibGUI/Application.h>
#include <LibMain/Main.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

namespace WebDesktop {

MessageHandler::MessageHandler()
    : m_server(Core::LocalServer::construct())
{
    // Load configuration
    auto config_result = WebDesktopConfig::the().load_from_file("/etc/webdesktop.conf");
    if (config_result.is_error()) {
        dbgln("Failed to load WebDesktop config, using defaults: {}", config_result.error());
    }
}

MessageHandler::~MessageHandler()
{
    stop();
}

ErrorOr<void> MessageHandler::start()
{
    TRY(setup_socket());
    
    m_running = true;
    dbgln("WebDesktop MessageHandler started on socket /tmp/webdesktop-messages");
    
    return {};
}

void MessageHandler::stop()
{
    m_running = false;
    if (m_server) {
        m_server->close();
    }
}

ErrorOr<void> MessageHandler::setup_socket()
{
    TRY(m_server->take_over_from_system_server("/tmp/webdesktop-messages"));
    m_server->on_accept = [this](NonnullOwnPtr<Core::LocalSocket> socket) {
        handle_connection(move(socket));
    };
    
    return {};
}

void MessageHandler::handle_connection(NonnullOwnPtr<Core::LocalSocket> socket)
{
    auto buffer = ByteBuffer::create_uninitialized(4096).release_value_but_fixme_should_propagate_errors();
    
    while (m_running) {
        auto nread = socket->read_some(buffer).release_value_but_fixme_should_propagate_errors();
        if (nread == 0) {
            break; // Connection closed
        }
        
        auto message_data = StringView { buffer.span().slice(0, nread) };
        
        // Parse JSON message
        auto json_result = JsonParser(message_data).parse();
        if (json_result.is_error()) {
            dbgln("Failed to parse message: {}", json_result.error());
            continue;
        }
        
        auto json_value = json_result.release_value();
        if (!json_value.is_object()) {
            dbgln("Message is not a JSON object");
            continue;
        }
        
        auto json_object = json_value.as_object();
        
        // Extract message components
        MessageRequest request;
        request.command = json_object.get_byte_string("command"sv).value_or(""sv);
        request.data = json_object.get_object("data"sv).value_or(JsonObject {});
        request.origin = URL::URL(json_object.get_byte_string("origin"sv).value_or(""sv));
        request.target_origin = json_object.get_byte_string("targetOrigin"sv).value_or("*"sv);
        
        handle_message(request);
    }
}

void MessageHandler::handle_message(MessageRequest const& request)
{
    dbgln("Received message: command='{}', origin='{}', targetOrigin='{}'", 
          request.command, request.origin, request.target_origin);
    
    // Validate origin using config
    if (!WebDesktopConfig::the().is_origin_allowed(request.origin.serialize())) {
        dbgln("Origin not allowed: {}", request.origin);
        return;
    }
    
    // Check if command is allowed
    if (!WebDesktopConfig::the().is_command_allowed(request.command)) {
        dbgln("Command not allowed: {}", request.command);
        return;
    }
    
    // Check authentication if required
    auto auth_token = request.data.get_byte_string("authToken"sv).value_or(""sv);
    if (!WebDesktopConfig::the().is_authenticated(auth_token)) {
        dbgln("Authentication failed for command: {}", request.command);
        return;
    }
    
    // Parse and execute command
    auto command = parse_command(request.command);
    if (command == SystemCommand::Unknown) {
        dbgln("Unknown command: {}", request.command);
        return;
    }
    
    auto result = execute_system_command(command, request.data);
    if (result.is_error()) {
        dbgln("Failed to execute command '{}': {}", request.command, result.error());
    }
}

SystemCommand MessageHandler::parse_command(ByteString const& command)
{
    if (command == "shutdown") return SystemCommand::Shutdown;
    if (command == "restart") return SystemCommand::Restart;
    if (command == "logout") return SystemCommand::Logout;
    if (command == "lock") return SystemCommand::Lock;
    if (command == "suspend") return SystemCommand::Suspend;
    if (command == "hibernate") return SystemCommand::Hibernate;
    if (command == "volumeUp") return SystemCommand::VolumeUp;
    if (command == "volumeDown") return SystemCommand::VolumeDown;
    if (command == "volumeMute") return SystemCommand::VolumeMute;
    if (command == "brightnessUp") return SystemCommand::BrightnessUp;
    if (command == "brightnessDown") return SystemCommand::BrightnessDown;
    if (command == "screenshot") return SystemCommand::Screenshot;
    
    return SystemCommand::Unknown;
}

bool MessageHandler::validate_origin(URL::URL const& origin, ByteString const& target_origin)
{
    // Check if origin is valid
    if (!origin.is_valid()) {
        return false;
    }
    
    // Require HTTPS for security (unless explicitly allowed)
    if (m_require_https && origin.scheme() != "https") {
        return false;
    }
    
    // Check against allowed origins
    for (auto const& allowed_origin : m_allowed_origins) {
        if (allowed_origin == "*" || origin.serialize() == allowed_origin) {
            return true;
        }
        
        // Check if origin matches target_origin
        if (target_origin != "*" && origin.serialize() == target_origin) {
            return true;
        }
    }
    
    return false;
}

ErrorOr<void> MessageHandler::execute_system_command(SystemCommand command, JsonObject const& data)
{
    switch (command) {
    case SystemCommand::Shutdown:
        return shutdown_system();
    case SystemCommand::Restart:
        return restart_system();
    case SystemCommand::Logout:
        return logout_user();
    case SystemCommand::Lock:
        return lock_screen();
    case SystemCommand::Suspend:
        return suspend_system();
    case SystemCommand::Hibernate:
        return hibernate_system();
    case SystemCommand::VolumeUp:
        return adjust_volume(true);
    case SystemCommand::VolumeDown:
        return adjust_volume(false);
    case SystemCommand::VolumeMute:
        return mute_volume();
    case SystemCommand::BrightnessUp:
        return adjust_brightness(true);
    case SystemCommand::BrightnessDown:
        return adjust_brightness(false);
    case SystemCommand::Screenshot:
        return take_screenshot();
    default:
        return Error::from_string_literal("Unknown system command");
    }
}

ErrorOr<void> MessageHandler::shutdown_system()
{
    dbgln("Executing system shutdown");
    
    // Use systemctl or direct shutdown command
    auto result = Core::System::exec("/bin/shutdown", Vector<ByteString> { "shutdown", "now" }, Core::System::SearchInPath::No);
    if (result.is_error()) {
        // Fallback to direct shutdown
        TRY(Core::System::exec("/sbin/shutdown", Vector<ByteString> { "-h", "now" }, Core::System::SearchInPath::No));
    }
    
    return {};
}

ErrorOr<void> MessageHandler::restart_system()
{
    dbgln("Executing system restart");
    
    // Use systemctl or direct reboot command
    auto result = Core::System::exec("/bin/reboot", Vector<ByteString> { "reboot" }, Core::System::SearchInPath::No);
    if (result.is_error()) {
        // Fallback to direct reboot
        TRY(Core::System::exec("/sbin/reboot", Vector<ByteString> { "-r", "now" }, Core::System::SearchInPath::No));
    }
    
    return {};
}

ErrorOr<void> MessageHandler::logout_user()
{
    dbgln("Executing user logout");
    
    // Send SIGTERM to login session
    auto session_pid = getppid();
    if (session_pid > 1) {
        TRY(Core::System::kill(session_pid, SIGTERM));
    }
    
    return {};
}

ErrorOr<void> MessageHandler::lock_screen()
{
    dbgln("Executing screen lock");
    
    // Launch screen locker if available
    TRY(Core::System::exec("/bin/ScreenLocker", Vector<ByteString> {}, Core::System::SearchInPath::No));
    
    return {};
}

ErrorOr<void> MessageHandler::suspend_system()
{
    dbgln("Executing system suspend");
    
    // Use systemctl or direct suspend
    auto result = Core::System::exec("/bin/systemctl", Vector<ByteString> { "suspend" }, Core::System::SearchInPath::No);
    if (result.is_error()) {
        // Fallback to direct suspend
        TRY(Core::System::exec("/sbin/suspend", Vector<ByteString> {}, Core::System::SearchInPath::No));
    }
    
    return {};
}

ErrorOr<void> MessageHandler::hibernate_system()
{
    dbgln("Executing system hibernate");
    
    // Use systemctl or direct hibernate
    auto result = Core::System::exec("/bin/systemctl", Vector<ByteString> { "hibernate" }, Core::System::SearchInPath::No);
    if (result.is_error()) {
        // Fallback to direct hibernate
        TRY(Core::System::exec("/sbin/hibernate", Vector<ByteString> {}, Core::System::SearchInPath::No));
    }
    
    return {};
}

ErrorOr<void> MessageHandler::adjust_volume(bool increase)
{
    dbgln("Adjusting volume: {}", increase ? "up" : "down");
    
    // Use amixer or pactl for volume control
    Vector<ByteString> args;
    if (increase) {
        args = { "set", "Master", "5%+" };
    } else {
        args = { "set", "Master", "5%-" };
    }
    
    auto result = Core::System::exec("/usr/bin/amixer", args, Core::System::SearchInPath::No);
    if (result.is_error()) {
        // Fallback to pactl
        if (increase) {
            args = { "set-sink-volume", "@DEFAULT_SINK@", "+5%" };
        } else {
            args = { "set-sink-volume", "@DEFAULT_SINK@", "-5%" };
        }
        TRY(Core::System::exec("/usr/bin/pactl", args, Core::System::SearchInPath::No));
    }
    
    return {};
}

ErrorOr<void> MessageHandler::mute_volume()
{
    dbgln("Toggling volume mute");
    
    // Use amixer for mute toggle
    auto result = Core::System::exec("/usr/bin/amixer", Vector<ByteString> { "set", "Master", "toggle" }, Core::System::SearchInPath::No);
    if (result.is_error()) {
        // Fallback to pactl
        TRY(Core::System::exec("/usr/bin/pactl", Vector<ByteString> { "set-sink-mute", "@DEFAULT_SINK@", "toggle" }, Core::System::SearchInPath::No));
    }
    
    return {};
}

ErrorOr<void> MessageHandler::adjust_brightness(bool increase)
{
    dbgln("Adjusting brightness: {}", increase ? "up" : "down");
    
    // Use xbacklight or brightnessctl for brightness control
    Vector<ByteString> args;
    if (increase) {
        args = { "inc", "5" };
    } else {
        args = { "dec", "5" };
    }
    
    auto result = Core::System::exec("/usr/bin/brightnessctl", args, Core::System::SearchInPath::No);
    if (result.is_error()) {
        // Fallback to xbacklight
        if (increase) {
            args = { "+5" };
        } else {
            args = { "-5" };
        }
        TRY(Core::System::exec("/usr/bin/xbacklight", args, Core::System::SearchInPath::No));
    }
    
    return {};
}

ErrorOr<void> MessageHandler::take_screenshot()
{
    dbgln("Taking screenshot");
    
    // Use gnome-screenshot or scrot for screenshots
    auto timestamp = Core::System::get_time();
    auto filename = ByteString::formatted("/tmp/screenshot_{}.png", timestamp);
    
    auto result = Core::System::exec("/usr/bin/gnome-screenshot", Vector<ByteString> { "-f", filename }, Core::System::SearchInPath::No);
    if (result.is_error()) {
        // Fallback to scrot
        TRY(Core::System::exec("/usr/bin/scrot", Vector<ByteString> { filename }, Core::System::SearchInPath::No));
    }
    
    return {};
}

}