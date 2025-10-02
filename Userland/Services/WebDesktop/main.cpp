/*
 * Copyright (c) 2024, SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "MessageHandler.h"
#include <LibCore/ArgsParser.h>
#include <LibCore/System.h>
#include <LibGUI/Application.h>
#include <LibMain/Main.h>
#include <unistd.h>

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    ByteString target_url = "https://example.com";
    bool kiosk_mode = true;
    bool enable_message_handler = true;
    
    Core::ArgsParser args_parser;
    args_parser.add_option(target_url, "URL to load in fullscreen", "url", 'u', "url");
    args_parser.add_option(kiosk_mode, "Enable kiosk mode (disable navigation)", "kiosk", 'k');
    args_parser.add_option(enable_message_handler, "Enable message handler for system commands", "messages", 'm');
    args_parser.parse(arguments);

    TRY(Core::System::pledge("stdio recvfd sendfd rpath exec proc unix"));
    
    // Start message handler if enabled
    if (enable_message_handler) {
        auto message_handler = make<WebDesktop::MessageHandler>();
        TRY(message_handler->start());
        
        // Keep message handler running in background
        auto message_handler_thread = Threading::BackgroundAction<ErrorOr<void>>::create(
            [message_handler = move(message_handler)]() -> ErrorOr<void> {
                // Keep the message handler running
                while (true) {
                    sleep(1);
                }
                return {};
            });
        
        message_handler_thread->start();
    }
    
    // Launch browser in fullscreen kiosk mode
    Vector<ByteString> browser_args = {
        "/bin/Browser",
        "--url", target_url,
        "--fullscreen",
        "--kiosk"
    };
    
    if (kiosk_mode) {
        browser_args.append("--disable-navigation");
    }
    
    TRY(Core::System::exec("/bin/Browser", browser_args, Core::System::SearchInPath::No));
    return 0;
}