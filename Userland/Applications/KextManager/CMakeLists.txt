serenity_component(
    KextManager
    RECOMMENDED
    TARGETS KextManager
)

set(SOURCES
    main.cpp
)

serenity_app(KextManager ICON app-run)
target_link_libraries(KextManager PRIVATE LibCore LibFileSystem LibGUI LibMain)
