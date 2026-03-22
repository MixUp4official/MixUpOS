serenity_component(
    CompatibilityAssistant
    RECOMMENDED
    TARGETS CompatibilityAssistant
)

set(SOURCES
    main.cpp
)

serenity_app(CompatibilityAssistant ICON app-settings)
target_link_libraries(CompatibilityAssistant PRIVATE LibCore LibFileSystem LibGUI LibMain)
