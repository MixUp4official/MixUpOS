/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <AK/LexicalPath.h>
#include <LibCore/System.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/FilePicker.h>
#include <LibGUI/Label.h>
#include <LibGUI/MessageBox.h>
#include <LibGUI/Process.h>
#include <LibGUI/Statusbar.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath unix proc exec"));
    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath proc exec"));
    TRY(Core::System::unveil("/bin/CompatibilityAssistant", "x"));
    TRY(Core::System::unveil("/bin/chmod", "x"));
    TRY(Core::System::unveil("/bin/mkdir", "x"));
    TRY(Core::System::unveil("/bin/cp", "x"));
    TRY(Core::System::unveil("/home", "r"));
    TRY(Core::System::unveil("/home/anon", "rwc"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto window = GUI::Window::construct();
    window->set_title("AppImage Launcher");
    window->resize(520, 220);
    window->set_resizable(false);

    auto& widget = window->set_main_widget<GUI::Widget>();
    widget.set_fill_with_background_color(true);
    widget.set_layout<GUI::VerticalBoxLayout>(8, 10);

    auto& label = widget.add<GUI::Label>("Open, integrate and manage .AppImage files"_string);
    label.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    ByteString selected_path;
    auto& selected_label = widget.add<GUI::Label>("No AppImage selected."_string);
    selected_label.set_text_wrapping(Gfx::TextWrapping::Wrap);
    selected_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    auto& launch_button = widget.add<GUI::Button>("Choose AppImage..."_string);
    launch_button.set_fixed_height(32);
    launch_button.on_click = [window, &selected_path, &selected_label](auto) {
        auto path = GUI::FilePicker::get_open_filepath(window.ptr(), "Choose AppImage"sv, "/home/anon"sv, false, GUI::Dialog::ScreenPosition::CenterWithinParent, { { "AppImage files", { "AppImage", "appimage" } }, GUI::FileTypeFilter::all_files() });
        if (!path.has_value())
            return;
        selected_path = path.value();
        selected_label.set_text(MUST(String::formatted("Selected: {}", selected_path)));
    };

    auto& actions = widget.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>(GUI::Margins {}, 6);

    auto& run_button = actions.add<GUI::Button>("Run AppImage"_string);
    run_button.on_click = [window, &selected_path](auto) {
        if (selected_path.is_empty()) {
            GUI::MessageBox::show_error(window.ptr(), "Select an AppImage first.");
            return;
        }
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/chmod"sv, Array { "+x"sv, selected_path.bytes_as_string_view() });
        GUI::Process::spawn_or_show_error(window.ptr(), selected_path.bytes_as_string_view());
    };

    auto& integrate_button = actions.add<GUI::Button>("Integrate"_string);
    integrate_button.on_click = [window, &selected_path](auto) {
        if (selected_path.is_empty()) {
            GUI::MessageBox::show_error(window.ptr(), "Select an AppImage first.");
            return;
        }

        auto apps_dir = "/home/anon/Applications/AppImages"sv;
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/mkdir"sv, Array { "-p"sv, apps_dir });

        auto file_name = LexicalPath(selected_path).basename();
        auto integrated_path = ByteString::formatted("{}/{}", apps_dir, file_name);
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/cp"sv, Array { selected_path.bytes_as_string_view(), integrated_path.view() });
        GUI::MessageBox::show(window.ptr(), "AppImage copied to /home/anon/Applications/AppImages.\nYou can now launch it from this tool.", "AppImage integrated", GUI::MessageBox::Type::Information);
    };

    auto& compatibility_button = actions.add<GUI::Button>("Compatibility Assistant"_string);
    compatibility_button.on_click = [window, &selected_path](auto) {
        if (selected_path.is_empty()) {
            GUI::MessageBox::show_error(window.ptr(), "Select an AppImage first.");
            return;
        }
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/CompatibilityAssistant"sv, Array { selected_path.bytes_as_string_view() });
    };

    auto& hint = widget.add<GUI::Label>("Improved workflow: choose once, run directly, integrate into AppImages folder, or analyze compatibility."_string);
    hint.set_text_wrapping(Gfx::TextWrapping::Wrap);
    hint.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    auto& status = widget.add<GUI::Statusbar>();
    status.set_text("AppImage Launcher ready");

    window->show();
    return app->exec();
}
