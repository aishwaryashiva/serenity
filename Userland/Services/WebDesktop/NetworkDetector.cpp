/*
 * Copyright (c) 2024, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "NetworkDetector.h"
#include <AK/URL.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibHTTP/Job.h>
#include <LibHTTP/JobQueue.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace WebDesktop {

NetworkDetector::NetworkDetector()
    : m_periodic_timer(Core::Timer::create_repeating(30000, [this] { // Check every 30 seconds
        check_internet_connectivity();
        check_local_network();
        update_network_info();
    }))
{
    // Initialize test URLs
    m_test_urls = {
        URL::URL("https://httpbin.org/status/200"),
        URL::URL("https://google.com"),
        URL::URL("https://cloudflare.com")
    };
    
    m_local_test_urls = {
        URL::URL("http://localhost:8080/status"),
        URL::URL("http://127.0.0.1:8080/status"),
        URL::URL("http://192.168.1.1") // Common router IP
    };
}

NetworkDetector::~NetworkDetector()
{
    stop();
}

ErrorOr<void> NetworkDetector::start()
{
    m_running = true;
    
    // Initial network check
    TRY(check_internet_connectivity());
    TRY(check_local_network());
    TRY(update_network_info());
    
    // Start periodic checks
    start_periodic_checks();
    
    return {};
}

void NetworkDetector::stop()
{
    m_running = false;
    stop_periodic_checks();
}

void NetworkDetector::start_periodic_checks()
{
    if (m_periodic_timer) {
        m_periodic_timer->start();
    }
}

void NetworkDetector::stop_periodic_checks()
{
    if (m_periodic_timer) {
        m_periodic_timer->stop();
    }
}

ErrorOr<void> NetworkDetector::check_internet_connectivity()
{
    NetworkStatus previous_status = m_internet_status;
    
    // Try to connect to test URLs
    bool connected = false;
    for (auto const& url : m_test_urls) {
        auto result = check_connectivity_to_url(url);
        if (!result.is_error()) {
            connected = true;
            break;
        }
    }
    
    m_internet_status = connected ? NetworkStatus::Connected : NetworkStatus::NoInternet;
    
    if (m_internet_status != previous_status && on_internet_status_changed) {
        on_internet_status_changed(m_internet_status);
    }
    
    return {};
}

ErrorOr<void> NetworkDetector::check_local_network()
{
    NetworkStatus previous_status = m_local_network_status;
    
    // Check if we have a network interface with an IP address
    auto interface_info = get_network_interface_info();
    if (interface_info.is_error()) {
        m_local_network_status = NetworkStatus::Disconnected;
    } else {
        // Try to ping local resources
        bool local_connected = false;
        for (auto const& url : m_local_test_urls) {
            auto result = check_connectivity_to_url(url);
            if (!result.is_error()) {
                local_connected = true;
                break;
            }
        }
        
        m_local_network_status = local_connected ? NetworkStatus::Connected : NetworkStatus::Limited;
    }
    
    if (m_local_network_status != previous_status && on_local_network_status_changed) {
        on_local_network_status_changed(m_local_network_status);
    }
    
    return {};
}

ErrorOr<void> NetworkDetector::check_dns_resolution()
{
    // Try to resolve a known domain
    auto result = check_connectivity_to_url(URL::URL("https://google.com"));
    return result;
}

ErrorOr<void> NetworkDetector::update_network_info()
{
    NetworkInfo previous_info = m_network_info;
    
    // Get network interface information
    auto interface_info = get_network_interface_info();
    if (interface_info.is_error()) {
        return interface_info.release_error();
    }
    
    m_network_info.interface_name = interface_info.release_value();
    
    // Get IP address
    auto ip_result = get_ip_address(m_network_info.interface_name);
    if (!ip_result.is_error()) {
        m_network_info.ip_address = ip_result.release_value();
    }
    
    // Get gateway
    auto gateway_result = get_gateway(m_network_info.interface_name);
    if (!gateway_result.is_error()) {
        m_network_info.gateway = gateway_result.release_value();
    }
    
    // Get DNS servers
    auto dns_result = get_dns_servers();
    if (!dns_result.is_error()) {
        m_network_info.dns_servers = dns_result.release_value();
    }
    
    // Get MAC address
    auto mac_result = get_mac_address(m_network_info.interface_name);
    if (!mac_result.is_error()) {
        m_network_info.mac_address = mac_result.release_value();
    }
    
    // Check if wireless
    auto wireless_result = is_wireless_interface(m_network_info.interface_name);
    if (!wireless_result.is_error()) {
        m_network_info.is_wireless = wireless_result.release_value();
        
        // Get signal strength for wireless interfaces
        if (m_network_info.is_wireless) {
            auto signal_result = get_signal_strength(m_network_info.interface_name);
            if (!signal_result.is_error()) {
                m_network_info.signal_strength = signal_result.release_value();
            }
        } else {
            m_network_info.signal_strength = -1; // Wired connection
        }
    }
    
    // Update status based on connectivity
    if (m_internet_status == NetworkStatus::Connected) {
        m_network_info.status = NetworkStatus::Connected;
    } else if (m_local_network_status == NetworkStatus::Connected) {
        m_network_info.status = NetworkStatus::Limited;
    } else {
        m_network_info.status = NetworkStatus::Disconnected;
    }
    
    // Notify if info changed
    if (m_network_info.interface_name != previous_info.interface_name ||
        m_network_info.ip_address != previous_info.ip_address ||
        m_network_info.gateway != previous_info.gateway ||
        m_network_info.dns_servers != previous_info.dns_servers ||
        m_network_info.mac_address != previous_info.mac_address ||
        m_network_info.signal_strength != previous_info.signal_strength ||
        m_network_info.is_wireless != previous_info.is_wireless ||
        m_network_info.status != previous_info.status) {
        
        if (on_network_info_changed) {
            on_network_info_changed(m_network_info);
        }
    }
    
    return {};
}

ErrorOr<void> NetworkDetector::check_connectivity_to_url(URL::URL const& url)
{
    // Create a simple HTTP request to test connectivity
    auto job = HTTP::Job::construct(url, HTTP::Job::Method::HEAD);
    
    // Set a short timeout
    job->set_timeout(5000); // 5 seconds
    
    // Execute the job synchronously
    auto result = job->start();
    if (result.is_error()) {
        return result.release_error();
    }
    
    // Wait for completion
    while (job->is_running()) {
        Core::EventLoop::current().pump(Core::EventLoop::WaitMode::SelectForRead, 100);
    }
    
    if (job->is_failed()) {
        return Error::from_string_literal("HTTP request failed");
    }
    
    return {};
}

ErrorOr<ByteString> NetworkDetector::get_network_interface_info()
{
    // Try to get the primary network interface
    // This is a simplified implementation - in reality, you'd parse /proc/net/dev or use netlink
    
    // Check for common interface names
    Vector<ByteString> common_interfaces = { "eth0", "wlan0", "enp0s3", "ens33" };
    
    for (auto const& interface : common_interfaces) {
        auto ip_result = get_ip_address(interface);
        if (!ip_result.is_error() && !ip_result.value().is_empty()) {
            return interface;
        }
    }
    
    return Error::from_string_literal("No active network interface found");
}

ErrorOr<ByteString> NetworkDetector::get_ip_address(ByteString const& interface)
{
    // Create socket for ioctl
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return Error::from_errno(errno);
    }
    
    // Get interface information
    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface.characters(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        close(sockfd);
        return Error::from_errno(errno);
    }
    
    close(sockfd);
    
    // Convert IP address to string
    struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr->sin_addr, ip_str, INET_ADDRSTRLEN) == nullptr) {
        return Error::from_errno(errno);
    }
    
    return ByteString(ip_str);
}

ErrorOr<ByteString> NetworkDetector::get_gateway(ByteString const& interface)
{
    // Read gateway from /proc/net/route
    auto file = TRY(Core::File::open("/proc/net/route", Core::File::OpenMode::Read));
    auto content = TRY(file->read_until_eof());
    auto lines = ByteString(content).split('\n');
    
    for (auto const& line : lines) {
        if (line.starts_with(interface)) {
            auto parts = line.split('\t');
            if (parts.size() >= 3) {
                // Parse gateway IP (in hex)
                auto gateway_hex = parts[2];
                if (gateway_hex.length() == 8) {
                    // Convert hex to IP address
                    u32 gateway_ip = strtoul(gateway_hex.characters(), nullptr, 16);
                    struct in_addr addr;
                    addr.s_addr = gateway_ip;
                    
                    char ip_str[INET_ADDRSTRLEN];
                    if (inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN) != nullptr) {
                        return ByteString(ip_str);
                    }
                }
            }
        }
    }
    
    return Error::from_string_literal("Gateway not found");
}

ErrorOr<ByteString> NetworkDetector::get_dns_servers()
{
    // Read DNS servers from /etc/resolv.conf
    auto file = TRY(Core::File::open("/etc/resolv.conf", Core::File::OpenMode::Read));
    auto content = TRY(file->read_until_eof());
    auto lines = ByteString(content).split('\n');
    
    Vector<ByteString> dns_servers;
    for (auto const& line : lines) {
        if (line.starts_with("nameserver ")) {
            auto dns = line.substring(11); // Remove "nameserver " prefix
            if (!dns.is_empty()) {
                dns_servers.append(dns);
            }
        }
    }
    
    if (dns_servers.is_empty()) {
        return Error::from_string_literal("No DNS servers found");
    }
    
    // Join DNS servers with commas
    StringBuilder builder;
    for (size_t i = 0; i < dns_servers.size(); ++i) {
        if (i > 0) {
            builder.append(", ");
        }
        builder.append(dns_servers[i]);
    }
    
    return builder.to_byte_string();
}

ErrorOr<ByteString> NetworkDetector::get_mac_address(ByteString const& interface)
{
    // Read MAC address from /sys/class/net/<interface>/address
    auto path = ByteString::formatted("/sys/class/net/{}/address", interface);
    auto file = TRY(Core::File::open(path, Core::File::OpenMode::Read));
    auto content = TRY(file->read_until_eof());
    
    auto mac = ByteString(content).trim_whitespace();
    if (mac.is_empty()) {
        return Error::from_string_literal("MAC address not found");
    }
    
    return mac;
}

ErrorOr<int> NetworkDetector::get_signal_strength(ByteString const& interface)
{
    // Read signal strength from /proc/net/wireless
    auto file = TRY(Core::File::open("/proc/net/wireless", Core::File::OpenMode::Read));
    auto content = TRY(file->read_until_eof());
    auto lines = ByteString(content).split('\n');
    
    for (auto const& line : lines) {
        if (line.starts_with(interface)) {
            auto parts = line.split(' ');
            if (parts.size() >= 3) {
                // Signal strength is typically the third field
                auto signal_str = parts[2];
                auto signal = signal_str.to_int();
                if (signal.has_value()) {
                    // Convert from dBm to percentage (rough approximation)
                    int percentage = 100 + signal.value(); // dBm is typically negative
                    return max(0, min(100, percentage));
                }
            }
        }
    }
    
    return Error::from_string_literal("Signal strength not found");
}

ErrorOr<bool> NetworkDetector::is_wireless_interface(ByteString const& interface)
{
    // Check if interface is wireless by looking for wireless-specific files
    auto wireless_path = ByteString::formatted("/sys/class/net/{}/wireless", interface);
    auto result = Core::System::stat(wireless_path);
    return !result.is_error();
}

ErrorOr<void> NetworkDetector::configure_network(ByteString const& interface, ByteString const& ip, 
                                                ByteString const& subnet, ByteString const& gateway, 
                                                ByteString const& dns_servers)
{
    // This would typically involve calling system network configuration tools
    // For now, we'll just log the configuration
    dbgln("Configuring network: interface={}, ip={}, subnet={}, gateway={}, dns={}", 
          interface, ip, subnet, gateway, dns_servers);
    
    // In a real implementation, you would:
    // 1. Stop the network interface
    // 2. Configure IP address, subnet, gateway
    // 3. Update DNS servers in /etc/resolv.conf
    // 4. Restart the network interface
    // 5. Update routing table
    
    return {};
}

ErrorOr<void> NetworkDetector::reset_network_settings()
{
    dbgln("Resetting network settings to defaults");
    
    // In a real implementation, you would:
    // 1. Restore default network configuration
    // 2. Restart network services
    // 3. Update network information
    
    return {};
}

ErrorOr<void> NetworkDetector::restart_network_service()
{
    dbgln("Restarting network service");
    
    // In a real implementation, you would:
    // 1. Stop network services
    // 2. Restart network services
    // 3. Wait for services to come back up
    // 4. Update network information
    
    return {};
}

}