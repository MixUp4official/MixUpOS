serenity_component(
    RecoveryCenter
    RECOMMENDED
    TARGETS RecoveryCenter
)

set(SOURCES
    main.cpp
)

serenity_app(RecoveryCenter ICON app-settings)
target_link_libraries(RecoveryCenter PRIVATE LibConfig LibCore LibGUI LibMain)
