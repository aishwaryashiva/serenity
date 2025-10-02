/*
 * Copyright (c) 2024, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/ByteString.h>
#include <AK/Vector.h>

namespace WebDesktop {

struct SecurityConfig {
    Vector<ByteString> allowed_origins;
    bool require_https { true };
    bool allow_localhost { true };
    bool allow_file_protocol { false };
    Vector<ByteString> blocked_commands;
    bool require_authentication { false };
    ByteString authentication_token;
};

struct SystemConfig {
    bool enable_shutdown { true };
    bool enable_restart { true };
    bool enable_logout { true };
    bool enable_lock { true };
    bool enable_suspend { false };
    bool enable_hibernate { false };
    bool enable_volume_control { true };
    bool enable_brightness_control { true };
    bool enable_screenshot { true };
    u32 max_commands_per_minute { 10 };
    u32 command_timeout_seconds { 30 };
};

class WebDesktopConfig {
public:
    static WebDesktopConfig& the();
    
    SecurityConfig const& security() const { return m_security; }
    SystemConfig const& system() const { return m_system; }
    
    ErrorOr<void> load_from_file(ByteString const& config_path);
    ErrorOr<void> save_to_file(ByteString const& config_path);
    
    // Validation methods
    bool is_origin_allowed(ByteString const& origin) const;
    bool is_command_allowed(ByteString const& command) const;
    bool is_authenticated(ByteString const& token) const;

private:
    WebDesktopConfig() = default;
    
    SecurityConfig m_security;
    SystemConfig m_system;
    
    void set_defaults();
};

}