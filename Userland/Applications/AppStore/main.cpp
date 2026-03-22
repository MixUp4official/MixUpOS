/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "MainWidget.h"
#include <LibConfig/Client.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/System.h>
#include <LibDesktop/Launcher.h>
#include <LibGUI/Action.h>
#include <LibGUI/Application.h>
#include <LibGUI/CommonActions.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Menubar.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>
#include <LibURL/URL.h>

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd thread rpath cpath wpath unix proc exec"));

    bool show_updates_only = false;
    bool show_installed_only = false;

    Core::ArgsParser args_parser;
    args_parser.add_option(show_updates_only, "Show only packages with updates available", "updates-only", 'u');
    args_parser.add_option(show_installed_only, "Show only installed packages", "installed-only", 'i');
    args_parser.parse(arguments);

    auto app = TRY(GUI::Application::create(arguments));

    Config::pledge_domain("AppStore");
    app->set_config_domain("AppStore"_string);

    auto const man_file = "/usr/share/man/man1/Applications/AppStore.md";
    TRY(Desktop::Launcher::add_allowed_handler_with_only_specific_urls("/bin/Help", { URL::create_with_file_scheme(man_file) }));
    TRY(Desktop::Launcher::seal_allowlist());

    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil("/home", "rwc"));
    TRY(Core::System::unveil("/usr", "r"));
    TRY(Core::System::unveil("/bin/Browser", "x"));
    TRY(Core::System::unveil("/bin/FileManager", "x"));
    TRY(Core::System::unveil("/bin/Terminal", "x"));
    TRY(Core::System::unveil("/tmp/session/%sid/portal/request", "rw"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto app_icon = GUI::Icon::default_icon("app-browser"sv);

    auto window = GUI::Window::construct();
    window->restore_size_and_position("AppStore"sv, "Window"sv, { { 920, 580 } });
    window->save_size_and_position_on_close("AppStore"sv, "Window"sv);
    window->set_title("App Store");
    window->set_icon(app_icon.bitmap_for_size(16));

    auto widget = TRY(AppStore::MainWidget::try_create());
    window->set_main_widget(widget.ptr());
    TRY(widget->initialize());

    if (show_updates_only)
        widget->set_filter_mode(AppStore::PackageModel::FilterMode::Updates);
    else if (show_installed_only)
        widget->set_filter_mode(AppStore::PackageModel::FilterMode::Installed);

    auto file_menu = window->add_menu("&File"_string);
    file_menu->add_action(GUI::Action::create("&Reload", { Mod_Ctrl, Key_R }, [&](auto&) {
        widget->reload_packages();
    }));
    file_menu->add_action(GUI::Action::create("Fetch &Latest", { Mod_Ctrl | Mod_Shift, Key_R }, [&](auto&) {
        widget->fetch_latest_packages();
    }));
    file_menu->add_action(GUI::Action::create("Update &All", { Mod_Ctrl | Mod_Shift, Key_U }, [&](auto&) {
        widget->update_all_packages();
    }));
    file_menu->add_separator();
    file_menu->add_action(GUI::CommonActions::make_quit_action([&](auto&) {
        GUI::Application::the()->quit();
    }));

    auto view_menu = window->add_menu("&View"_string);
    view_menu->add_action(GUI::Action::create("Show &All Packages", [&](auto&) {
        widget->set_filter_mode(AppStore::PackageModel::FilterMode::All);
    }));
    view_menu->add_action(GUI::Action::create("Show &Installed", [&](auto&) {
        widget->set_filter_mode(AppStore::PackageModel::FilterMode::Installed);
    }));
    view_menu->add_action(GUI::Action::create("Show &Updates", [&](auto&) {
        widget->set_filter_mode(AppStore::PackageModel::FilterMode::Updates);
    }));
    view_menu->add_separator();
    view_menu->add_action(GUI::CommonActions::make_fullscreen_action([&](auto&) {
        window->set_fullscreen(!window->is_fullscreen());
    }));

    auto help_menu = window->add_menu("&Help"_string);
    help_menu->add_action(GUI::CommonActions::make_command_palette_action(window));
    help_menu->add_action(GUI::CommonActions::make_help_action([&](auto&) {
        Desktop::Launcher::open(URL::create_with_file_scheme(man_file), "/bin/Help");
    }));
    help_menu->add_action(GUI::CommonActions::make_about_action("App Store"_string, app_icon, window));

    window->show();
    return app->exec();
}
