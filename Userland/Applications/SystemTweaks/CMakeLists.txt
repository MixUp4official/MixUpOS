serenity_component(
    SystemTweaks
    RECOMMENDED
    TARGETS SystemTweaks
)

set(SOURCES
    main.cpp
)

serenity_app(SystemTweaks ICON app-settings)
target_link_libraries(SystemTweaks PRIVATE LibCore LibGUI LibMain)
