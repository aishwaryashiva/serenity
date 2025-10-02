/*
 * Copyright (c) 2024, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "WebDesktopMessageClient.h"
#include <AK/JsonObject.h>
#include <LibCore/System.h>

namespace WebDesktop {

WebDesktopMessageClient::WebDesktopMessageClient()
{
}

WebDesktopMessageClient::~WebDesktopMessageClient()
{
    disconnect();
}

ErrorOr<void> WebDesktopMessageClient::connect()
{
    if (m_connected) {
        return {};
    }
    
    m_socket = TRY(Core::LocalSocket::connect("/tmp/webdesktop-messages"sv));
    m_connected = true;
    
    return {};
}

void WebDesktopMessageClient::disconnect()
{
    if (m_socket) {
        m_socket->close();
        m_socket = nullptr;
    }
    m_connected = false;
}

ErrorOr<void> WebDesktopMessageClient::send_message(ByteString const& command, JsonObject const& data, URL::URL const& origin)
{
    if (!m_connected) {
        TRY(connect());
    }
    
    JsonObject message;
    message.set("command", command);
    message.set("data", data);
    message.set("origin", origin.serialize());
    message.set("targetOrigin", "*");
    
    return send_json_message(message);
}

ErrorOr<void> WebDesktopMessageClient::send_json_message(JsonObject const& message)
{
    if (!m_socket) {
        return Error::from_string_literal("Not connected to WebDesktop message handler");
    }
    
    auto message_string = message.to_byte_string();
    auto message_bytes = message_string.bytes();
    
    TRY(m_socket->write_until_depleted(message_bytes));
    
    return {};
}

ErrorOr<void> WebDesktopMessageClient::send_shutdown_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("shutdown", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_restart_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("restart", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_logout_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("logout", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_lock_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("lock", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_suspend_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("suspend", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_hibernate_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("hibernate", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_volume_up_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("volumeUp", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_volume_down_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("volumeDown", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_volume_mute_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("volumeMute", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_brightness_up_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("brightnessUp", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_brightness_down_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("brightnessDown", data, origin);
}

ErrorOr<void> WebDesktopMessageClient::send_screenshot_command(URL::URL const& origin)
{
    JsonObject data;
    return send_message("screenshot", data, origin);
}

}