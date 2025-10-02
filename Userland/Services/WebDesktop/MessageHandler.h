/*
 * Copyright (c) 2024, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/ByteString.h>
#include <AK/JsonObject.h>
#include <AK/JsonValue.h>
#include <AK/URL.h>
#include <LibCore/EventLoop.h>
#include <LibCore/LocalServer.h>
#include <LibCore/LocalSocket.h>
#include <LibCore/System.h>
#include <LibMain/Main.h>

namespace WebDesktop {

enum class SystemCommand {
    Shutdown,
    Restart,
    Logout,
    Lock,
    Suspend,
    Hibernate,
    VolumeUp,
    VolumeDown,
    VolumeMute,
    BrightnessUp,
    BrightnessDown,
    Screenshot,
    Unknown
};

struct MessageRequest {
    ByteString command;
    JsonObject data;
    URL::URL origin;
    ByteString target_origin;
};

class MessageHandler {
public:
    MessageHandler();
    ~MessageHandler();

    ErrorOr<void> start();
    void stop();

private:
    ErrorOr<void> setup_socket();
    void handle_connection(NonnullOwnPtr<Core::LocalSocket>);
    void handle_message(MessageRequest const&);
    SystemCommand parse_command(ByteString const& command);
    bool validate_origin(URL::URL const& origin, ByteString const& target_origin);
    ErrorOr<void> execute_system_command(SystemCommand command, JsonObject const& data);
    
    // System command implementations
    ErrorOr<void> shutdown_system();
    ErrorOr<void> restart_system();
    ErrorOr<void> logout_user();
    ErrorOr<void> lock_screen();
    ErrorOr<void> suspend_system();
    ErrorOr<void> hibernate_system();
    ErrorOr<void> adjust_volume(bool increase);
    ErrorOr<void> mute_volume();
    ErrorOr<void> adjust_brightness(bool increase);
    ErrorOr<void> take_screenshot();

    NonnullOwnPtr<Core::LocalServer> m_server;
    Core::EventLoop m_event_loop;
    bool m_running { false };
    
    // Security configuration
    Vector<ByteString> m_allowed_origins;
    bool m_require_https { true };
};

}