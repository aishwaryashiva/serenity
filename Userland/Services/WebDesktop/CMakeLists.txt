set(SOURCES
    main.cpp
)

serenity_bin(WebDesktop)
target_link_libraries(WebDesktop PRIVATE LibCore LibMain)