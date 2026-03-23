serenity_component(
    AppImageLauncher
    RECOMMENDED
    TARGETS AppImageLauncher
)

set(SOURCES
    main.cpp
)

serenity_app(AppImageLauncher ICON app-run)
target_link_libraries(AppImageLauncher PRIVATE LibCore LibGUI LibMain)
