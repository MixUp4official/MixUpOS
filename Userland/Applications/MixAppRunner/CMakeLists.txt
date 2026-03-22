serenity_component(
    MixAppRunner
    RECOMMENDED
    TARGETS MixAppRunner
)

set(SOURCES
    main.cpp
)

serenity_app(MixAppRunner ICON app-run)
target_link_libraries(MixAppRunner PRIVATE LibCore LibFileSystem LibGUI LibMain)
