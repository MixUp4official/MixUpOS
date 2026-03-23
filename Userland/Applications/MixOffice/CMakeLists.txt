serenity_component(
    MixOffice
    RECOMMENDED
    TARGETS MixOffice
)

set(SOURCES
    main.cpp
)

serenity_app(MixOffice ICON app-spreadsheet)
target_link_libraries(MixOffice PRIVATE LibCore LibFileSystem LibGUI LibMain)
