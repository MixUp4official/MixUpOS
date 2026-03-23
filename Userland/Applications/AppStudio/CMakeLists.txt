serenity_component(
    AppStudio
    RECOMMENDED
    TARGETS AppStudio
)

set(SOURCES
    main.cpp
)

serenity_app(AppStudio ICON app-settings)
target_link_libraries(AppStudio PRIVATE LibCore LibGUI LibMain)
