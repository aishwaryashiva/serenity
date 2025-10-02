set(SOURCES
    main.cpp
    MessageHandler.cpp
    WebDesktopConfig.cpp
    WebDesktopMessageClient.cpp
    NetworkDetector.cpp
)

serenity_bin(WebDesktop)
target_link_libraries(WebDesktop PRIVATE LibCore LibMain LibThreading LibHTTP)