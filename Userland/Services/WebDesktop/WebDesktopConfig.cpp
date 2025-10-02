/*
 * Copyright (c) 2024, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "WebDesktopConfig.h"
#include <AK/JsonObject.h>
#include <AK/JsonParser.h>
#include <LibCore/File.h>
#include <LibCore/System.h>

namespace WebDesktop {

WebDesktopConfig& WebDesktopConfig::the()
{
    static WebDesktopConfig instance;
    return instance;
}

ErrorOr<void> WebDesktopConfig::load_from_file(ByteString const& config_path)
{
    auto file = TRY(Core::File::open(config_path, Core::File::OpenMode::Read));
    auto file_contents = TRY(file->read_until_eof());
    auto json_string = StringView { file_contents };
    
    auto json_result = JsonParser(json_string).parse();
    if (json_result.is_error()) {
        dbgln("Failed to parse WebDesktop config: {}", json_result.error());
        set_defaults();
        return {};
    }
    
    auto json_value = json_result.release_value();
    if (!json_value.is_object()) {
        dbgln("WebDesktop config is not a JSON object");
        set_defaults();
        return {};
    }
    
    auto config = json_value.as_object();
    
    // Load security config
    if (auto security_obj = config.get_object("security"sv); security_obj.has_value()) {
        auto& sec = security_obj.value();
        
        if (auto origins = sec.get_array("allowedOrigins"sv); origins.has_value()) {
            m_security.allowed_origins.clear();
            for (auto const& origin : origins.value().values()) {
                if (origin.is_string()) {
                    m_security.allowed_origins.append(origin.as_string());
                }
            }
        }
        
        m_security.require_https = sec.get_bool("requireHttps"sv).value_or(true);
        m_security.allow_localhost = sec.get_bool("allowLocalhost"sv).value_or(true);
        m_security.allow_file_protocol = sec.get_bool("allowFileProtocol"sv).value_or(false);
        m_security.require_authentication = sec.get_bool("requireAuthentication"sv).value_or(false);
        m_security.authentication_token = sec.get_byte_string("authenticationToken"sv).value_or(""sv);
        
        if (auto blocked = sec.get_array("blockedCommands"sv); blocked.has_value()) {
            m_security.blocked_commands.clear();
            for (auto const& cmd : blocked.value().values()) {
                if (cmd.is_string()) {
                    m_security.blocked_commands.append(cmd.as_string());
                }
            }
        }
    }
    
    // Load system config
    if (auto system_obj = config.get_object("system"sv); system_obj.has_value()) {
        auto& sys = system_obj.value();
        
        m_system.enable_shutdown = sys.get_bool("enableShutdown"sv).value_or(true);
        m_system.enable_restart = sys.get_bool("enableRestart"sv).value_or(true);
        m_system.enable_logout = sys.get_bool("enableLogout"sv).value_or(true);
        m_system.enable_lock = sys.get_bool("enableLock"sv).value_or(true);
        m_system.enable_suspend = sys.get_bool("enableSuspend"sv).value_or(false);
        m_system.enable_hibernate = sys.get_bool("enableHibernate"sv).value_or(false);
        m_system.enable_volume_control = sys.get_bool("enableVolumeControl"sv).value_or(true);
        m_system.enable_brightness_control = sys.get_bool("enableBrightnessControl"sv).value_or(true);
        m_system.enable_screenshot = sys.get_bool("enableScreenshot"sv).value_or(true);
        m_system.max_commands_per_minute = sys.get_u32("maxCommandsPerMinute"sv).value_or(10);
        m_system.command_timeout_seconds = sys.get_u32("commandTimeoutSeconds"sv).value_or(30);
    }
    
    return {};
}

ErrorOr<void> WebDesktopConfig::save_to_file(ByteString const& config_path)
{
    JsonObject config;
    
    // Security config
    JsonObject security;
    JsonArray allowed_origins;
    for (auto const& origin : m_security.allowed_origins) {
        allowed_origins.append(origin);
    }
    security.set("allowedOrigins", move(allowed_origins));
    security.set("requireHttps", m_security.require_https);
    security.set("allowLocalhost", m_security.allow_localhost);
    security.set("allowFileProtocol", m_security.allow_file_protocol);
    security.set("requireAuthentication", m_security.require_authentication);
    security.set("authenticationToken", m_security.authentication_token);
    
    JsonArray blocked_commands;
    for (auto const& cmd : m_security.blocked_commands) {
        blocked_commands.append(cmd);
    }
    security.set("blockedCommands", move(blocked_commands));
    
    config.set("security", move(security));
    
    // System config
    JsonObject system;
    system.set("enableShutdown", m_system.enable_shutdown);
    system.set("enableRestart", m_system.enable_restart);
    system.set("enableLogout", m_system.enable_logout);
    system.set("enableLock", m_system.enable_lock);
    system.set("enableSuspend", m_system.enable_suspend);
    system.set("enableHibernate", m_system.enable_hibernate);
    system.set("enableVolumeControl", m_system.enable_volume_control);
    system.set("enableBrightnessControl", m_system.enable_brightness_control);
    system.set("enableScreenshot", m_system.enable_screenshot);
    system.set("maxCommandsPerMinute", m_system.max_commands_per_minute);
    system.set("commandTimeoutSeconds", m_system.command_timeout_seconds);
    
    config.set("system", move(system));
    
    auto file = TRY(Core::File::open(config_path, Core::File::OpenMode::Write));
    auto config_string = config.to_byte_string();
    TRY(file->write_until_depleted(config_string.bytes()));
    
    return {};
}

bool WebDesktopConfig::is_origin_allowed(ByteString const& origin) const
{
    // Check if origin is in allowed list
    for (auto const& allowed : m_security.allowed_origins) {
        if (allowed == "*" || allowed == origin) {
            return true;
        }
    }
    
    // Check localhost if allowed
    if (m_security.allow_localhost && (origin == "https://localhost" || origin == "https://127.0.0.1")) {
        return true;
    }
    
    return false;
}

bool WebDesktopConfig::is_command_allowed(ByteString const& command) const
{
    // Check if command is blocked
    for (auto const& blocked : m_security.blocked_commands) {
        if (blocked == command) {
            return false;
        }
    }
    
    // Check system-specific permissions
    if (command == "shutdown" && !m_system.enable_shutdown) return false;
    if (command == "restart" && !m_system.enable_restart) return false;
    if (command == "logout" && !m_system.enable_logout) return false;
    if (command == "lock" && !m_system.enable_lock) return false;
    if (command == "suspend" && !m_system.enable_suspend) return false;
    if (command == "hibernate" && !m_system.enable_hibernate) return false;
    if ((command == "volumeUp" || command == "volumeDown" || command == "volumeMute") && !m_system.enable_volume_control) return false;
    if ((command == "brightnessUp" || command == "brightnessDown") && !m_system.enable_brightness_control) return false;
    if (command == "screenshot" && !m_system.enable_screenshot) return false;
    
    return true;
}

bool WebDesktopConfig::is_authenticated(ByteString const& token) const
{
    if (!m_security.require_authentication) {
        return true;
    }
    
    return token == m_security.authentication_token;
}

void WebDesktopConfig::set_defaults()
{
    // Default security config
    m_security.allowed_origins = {
        "https://localhost",
        "https://127.0.0.1"
    };
    m_security.require_https = true;
    m_security.allow_localhost = true;
    m_security.allow_file_protocol = false;
    m_security.require_authentication = false;
    m_security.authentication_token = "";
    m_security.blocked_commands = {};
    
    // Default system config
    m_system.enable_shutdown = true;
    m_system.enable_restart = true;
    m_system.enable_logout = true;
    m_system.enable_lock = true;
    m_system.enable_suspend = false;
    m_system.enable_hibernate = false;
    m_system.enable_volume_control = true;
    m_system.enable_brightness_control = true;
    m_system.enable_screenshot = true;
    m_system.max_commands_per_minute = 10;
    m_system.command_timeout_seconds = 30;
}

}