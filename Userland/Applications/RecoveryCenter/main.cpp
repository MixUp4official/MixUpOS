/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibConfig/Client.h>
#include <LibCore/StandardPaths.h>
#include <LibCore/System.h>
#include <LibCore/Version.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/Clipboard.h>
#include <LibGUI/ConnectionToWindowServer.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Label.h>
#include <LibGUI/MessageBox.h>
#include <LibGUI/Process.h>
#include <LibGUI/SeparatorWidget.h>
#include <LibGUI/Statusbar.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

static ErrorOr<String> build_diagnostics_text()
{
    auto version = TRY(Core::Version::read_long_version_string());
    auto theme_name = GUI::ConnectionToWindowServer::the().get_system_theme();
    return String::formatted(
        "MixUpOS Diagnostics\n"
        "Version: {}\n"
        "Theme: {}\n"
        "Home: {}\n"
        "Recovery Center: available\n"
        "App Store updates entry: available",
        version,
        theme_name,
        Core::StandardPaths::home_directory());
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd rpath cpath wpath proc exec unix"));

    auto app = TRY(GUI::Application::create(arguments));
    Config::pledge_domain("RecoveryCenter");
    app->set_config_domain("RecoveryCenter"_string);

    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil("/home", "rwc"));
    TRY(Core::System::unveil("/bin/AppStore", "x"));
    TRY(Core::System::unveil("/bin/SystemMonitor", "x"));
    TRY(Core::System::unveil("/bin/Terminal", "x"));
    TRY(Core::System::unveil("/bin/FileManager", "x"));
    TRY(Core::System::unveil("/bin/reboot", "x"));
    TRY(Core::System::unveil("/bin/shutdown", "x"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto app_icon = GUI::Icon::default_icon("app-settings"sv);

    auto window = GUI::Window::construct();
    window->restore_size_and_position("RecoveryCenter"sv, "Window"sv, { { 620, 360 } });
    window->save_size_and_position_on_close("RecoveryCenter"sv, "Window"sv);
    window->set_title("Recovery Center");
    window->set_icon(app_icon.bitmap_for_size(16));

    auto& root = window->set_main_widget<GUI::Widget>();
    root.set_fill_with_background_color(true);
    root.set_layout<GUI::VerticalBoxLayout>(8, 8);

    auto& title = root.add<GUI::Label>("Recovery Center"_string);
    title.set_font(title.font().bold_variant());
    title.set_fixed_height(24);
    title.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    auto& intro = root.add<GUI::Label>("Quick access to updates, diagnostics, and safe recovery actions for MixUpOS."_string);
    intro.set_text_wrapping(Gfx::TextWrapping::Wrap);
    intro.set_fixed_height(32);
    intro.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    auto diagnostics_text = TRY(build_diagnostics_text());
    auto& diagnostics = root.add<GUI::Label>(diagnostics_text);
    diagnostics.set_text_wrapping(Gfx::TextWrapping::Wrap);
    diagnostics.set_fixed_height(84);
    diagnostics.set_text_alignment(Gfx::TextAlignment::TopLeft);

    root.add<GUI::HorizontalSeparator>();

    auto& row_one = root.add<GUI::Widget>();
    row_one.set_layout<GUI::HorizontalBoxLayout>(6);
    row_one.set_fixed_height(32);
    auto& updates_button = row_one.add<GUI::Button>("Software Updates"_string);
    auto& store_button = row_one.add<GUI::Button>("App Store"_string);
    auto& monitor_button = row_one.add<GUI::Button>("System Monitor"_string);

    auto& row_two = root.add<GUI::Widget>();
    row_two.set_layout<GUI::HorizontalBoxLayout>(6);
    row_two.set_fixed_height(32);
    auto& terminal_button = row_two.add<GUI::Button>("Open Terminal"_string);
    auto& files_button = row_two.add<GUI::Button>("Home Folder"_string);
    auto& copy_button = row_two.add<GUI::Button>("Copy Diagnostics"_string);

    auto& row_three = root.add<GUI::Widget>();
    row_three.set_layout<GUI::HorizontalBoxLayout>(6);
    row_three.set_fixed_height(32);
    auto& reboot_button = row_three.add<GUI::Button>("Reboot"_string);
    auto& shutdown_button = row_three.add<GUI::Button>("Shut Down"_string);
    row_three.add_spacer();

    auto& statusbar = root.add<GUI::Statusbar>();
    statusbar.set_text("Recovery Center ready"_string);

    updates_button.on_click = [&]([[maybe_unused]] auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/AppStore"sv, Array { "--updates-only"sv });
        statusbar.set_text("Opened Software Updates"_string);
    };

    store_button.on_click = [&]([[maybe_unused]] auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/AppStore"sv);
        statusbar.set_text("Opened App Store"_string);
    };

    monitor_button.on_click = [&]([[maybe_unused]] auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/SystemMonitor"sv);
        statusbar.set_text("Opened System Monitor"_string);
    };

    terminal_button.on_click = [&]([[maybe_unused]] auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/Terminal"sv);
        statusbar.set_text("Opened Terminal"_string);
    };

    files_button.on_click = [&]([[maybe_unused]] auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/FileManager"sv, Array { Core::StandardPaths::home_directory().bytes_as_string_view() });
        statusbar.set_text("Opened home folder"_string);
    };

    copy_button.on_click = [&]([[maybe_unused]] auto) {
        auto diagnostics_or_error = build_diagnostics_text();
        if (diagnostics_or_error.is_error()) {
            GUI::MessageBox::show_error(window.ptr(), "Failed to build diagnostics text.");
            return;
        }
        GUI::Clipboard::the().set_plain_text(diagnostics_or_error.release_value());
        statusbar.set_text("Copied diagnostics to clipboard"_string);
    };

    reboot_button.on_click = [&]([[maybe_unused]] auto) {
        auto rc = GUI::MessageBox::show(window.ptr(), "Reboot MixUpOS now?"sv, "Recovery Center"sv, GUI::MessageBox::Type::Question, GUI::MessageBox::InputType::YesNo);
        if (rc == GUI::Dialog::ExecResult::Yes)
            GUI::Process::spawn_or_show_error(window.ptr(), "/bin/reboot"sv);
    };

    shutdown_button.on_click = [&]([[maybe_unused]] auto) {
        auto rc = GUI::MessageBox::show(window.ptr(), "Shut down MixUpOS now?"sv, "Recovery Center"sv, GUI::MessageBox::Type::Question, GUI::MessageBox::InputType::YesNo);
        if (rc == GUI::Dialog::ExecResult::Yes)
            GUI::Process::spawn_or_show_error(window.ptr(), "/bin/shutdown"sv, Array { "--now"sv });
    };

    window->show();
    return app->exec();
}
