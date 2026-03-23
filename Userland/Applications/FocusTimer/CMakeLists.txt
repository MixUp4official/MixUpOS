serenity_component(
    FocusTimer
    RECOMMENDED
    TARGETS FocusTimer
)

set(SOURCES
    main.cpp
)

serenity_app(FocusTimer ICON app-clock)
target_link_libraries(FocusTimer PRIVATE LibCore LibGUI LibMain)
