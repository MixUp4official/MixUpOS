serenity_component(
    LauncherForge
    RECOMMENDED
    TARGETS LauncherForge
)

set(SOURCES
    main.cpp
)

serenity_app(LauncherForge ICON app-settings)
target_link_libraries(LauncherForge PRIVATE LibCore LibGUI LibMain)
