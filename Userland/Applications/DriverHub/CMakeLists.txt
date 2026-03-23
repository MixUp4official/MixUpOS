serenity_component(
    DriverHub
    RECOMMENDED
    TARGETS DriverHub
)

set(SOURCES
    main.cpp
)

serenity_app(DriverHub ICON app-settings)
target_link_libraries(DriverHub PRIVATE LibCore LibGUI LibMain)
