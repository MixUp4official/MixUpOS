serenity_component(
    LinuxDriverManager
    RECOMMENDED
    TARGETS LinuxDriverManager
)

set(SOURCES
    main.cpp
)

serenity_app(LinuxDriverManager ICON app-run)
target_link_libraries(LinuxDriverManager PRIVATE LibCore LibGUI LibMain)
