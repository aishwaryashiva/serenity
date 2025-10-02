/*
 * Copyright (c) 2024, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/ByteString.h>
#include <AK/JsonObject.h>
#include <AK/URL.h>
#include <LibCore/LocalSocket.h>
#include <LibCore/System.h>

namespace WebDesktop {

class WebDesktopMessageClient {
public:
    WebDesktopMessageClient();
    ~WebDesktopMessageClient();

    ErrorOr<void> connect();
    void disconnect();
    
    // System command methods
    ErrorOr<void> send_shutdown_command(URL::URL const& origin);
    ErrorOr<void> send_restart_command(URL::URL const& origin);
    ErrorOr<void> send_logout_command(URL::URL const& origin);
    ErrorOr<void> send_lock_command(URL::URL const& origin);
    ErrorOr<void> send_suspend_command(URL::URL const& origin);
    ErrorOr<void> send_hibernate_command(URL::URL const& origin);
    ErrorOr<void> send_volume_up_command(URL::URL const& origin);
    ErrorOr<void> send_volume_down_command(URL::URL const& origin);
    ErrorOr<void> send_volume_mute_command(URL::URL const& origin);
    ErrorOr<void> send_brightness_up_command(URL::URL const& origin);
    ErrorOr<void> send_brightness_down_command(URL::URL const& origin);
    ErrorOr<void> send_screenshot_command(URL::URL const& origin);
    
    // Generic message sending
    ErrorOr<void> send_message(ByteString const& command, JsonObject const& data, URL::URL const& origin);

private:
    ErrorOr<void> send_json_message(JsonObject const& message);
    
    OwnPtr<Core::LocalSocket> m_socket;
    bool m_connected { false };
};

}