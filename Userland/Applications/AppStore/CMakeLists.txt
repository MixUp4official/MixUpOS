serenity_component(
    AppStore
    RECOMMENDED
    TARGETS AppStore
)

set(SOURCES
    main.cpp
    MainWidget.cpp
    PackageModel.cpp
)

serenity_app(AppStore ICON app-browser)
target_link_libraries(AppStore PRIVATE LibConfig LibCore LibDesktop LibFileSystem LibGfx LibGUI LibHTTP LibMain LibMarkdown LibProtocol LibURL)
