serenity_component(
    FeatureBoard
    RECOMMENDED
    TARGETS FeatureBoard
)

set(SOURCES
    main.cpp
)

serenity_app(FeatureBoard ICON app-settings)
target_link_libraries(FeatureBoard PRIVATE LibCore LibGUI LibMain)
