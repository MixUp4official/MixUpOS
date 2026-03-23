serenity_component(
    DevLaunchpad
    RECOMMENDED
    TARGETS DevLaunchpad
)

set(SOURCES
    main.cpp
)

serenity_app(DevLaunchpad ICON app-terminal)
target_link_libraries(DevLaunchpad PRIVATE LibCore LibGUI LibMain)
