/*
 * Copyright (c) 2024, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/ByteString.h>
#include <AK/Function.h>
#include <AK/URL.h>
#include <LibCore/EventLoop.h>
#include <LibCore/Timer.h>
#include <LibCore/System.h>

namespace WebDesktop {

enum class NetworkStatus {
    Unknown,
    Connected,
    Disconnected,
    Limited,
    NoInternet
};

struct NetworkInfo {
    ByteString interface_name;
    ByteString ip_address;
    ByteString gateway;
    ByteString dns_servers;
    ByteString mac_address;
    int signal_strength { -1 }; // -1 for wired, 0-100 for wireless
    bool is_wireless { false };
    NetworkStatus status { NetworkStatus::Unknown };
};

class NetworkDetector {
public:
    NetworkDetector();
    ~NetworkDetector();

    ErrorOr<void> start();
    void stop();
    
    // Network status queries
    NetworkStatus get_internet_status() const { return m_internet_status; }
    NetworkStatus get_local_network_status() const { return m_local_network_status; }
    NetworkInfo get_network_info() const { return m_network_info; }
    
    // Callbacks
    Function<void(NetworkStatus)> on_internet_status_changed;
    Function<void(NetworkStatus)> on_local_network_status_changed;
    Function<void(NetworkInfo)> on_network_info_changed;
    
    // Manual checks
    ErrorOr<void> check_internet_connectivity();
    ErrorOr<void> check_local_network();
    ErrorOr<void> check_dns_resolution();
    ErrorOr<void> update_network_info();
    
    // Network configuration
    ErrorOr<void> configure_network(ByteString const& interface, ByteString const& ip, 
                                   ByteString const& subnet, ByteString const& gateway, 
                                   ByteString const& dns_servers);
    ErrorOr<void> reset_network_settings();
    ErrorOr<void> restart_network_service();

private:
    void start_periodic_checks();
    void stop_periodic_checks();
    
    ErrorOr<void> check_connectivity_to_url(URL::URL const& url);
    ErrorOr<ByteString> get_network_interface_info();
    ErrorOr<ByteString> get_ip_address(ByteString const& interface);
    ErrorOr<ByteString> get_gateway(ByteString const& interface);
    ErrorOr<ByteString> get_dns_servers();
    ErrorOr<ByteString> get_mac_address(ByteString const& interface);
    ErrorOr<int> get_signal_strength(ByteString const& interface);
    ErrorOr<bool> is_wireless_interface(ByteString const& interface);
    
    NetworkStatus m_internet_status { NetworkStatus::Unknown };
    NetworkStatus m_local_network_status { NetworkStatus::Unknown };
    NetworkInfo m_network_info;
    
    NonnullOwnPtr<Core::Timer> m_periodic_timer;
    bool m_running { false };
    
    // Test URLs for connectivity checks
    Vector<URL::URL> m_test_urls;
    Vector<URL::URL> m_local_test_urls;
};

}