/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <LibCore/System.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/FilePicker.h>
#include <LibGUI/Label.h>
#include <LibGUI/Process.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath unix proc exec"));
    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath proc exec"));
    TRY(Core::System::unveil("/bin/CompatibilityAssistant", "x"));
    TRY(Core::System::unveil("/home", "r"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto window = GUI::Window::construct();
    window->set_title("AppImage Launcher");
    window->resize(430, 150);
    window->set_resizable(false);

    auto& widget = window->set_main_widget<GUI::Widget>();
    widget.set_fill_with_background_color(true);
    widget.set_layout<GUI::VerticalBoxLayout>(8, 10);

    auto& label = widget.add<GUI::Label>("Open .AppImage files from a graphical launcher"_string);
    label.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    auto& launch_button = widget.add<GUI::Button>("Choose AppImage..."_string);
    launch_button.set_fixed_height(32);
    launch_button.on_click = [window](auto) {
        auto path = GUI::FilePicker::get_open_filepath(window.ptr(), "Choose AppImage"sv, "/home/anon"sv, false, GUI::Dialog::ScreenPosition::CenterWithinParent, { { "AppImage files", { "AppImage", "appimage" } }, GUI::FileTypeFilter::all_files() });
        if (!path.has_value())
            return;
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/CompatibilityAssistant"sv, Array { path.value().bytes_as_string_view() });
    };

    auto& hint = widget.add<GUI::Label>("Tip: use MixOffice + AppImage Launcher for portable Linux workflows."_string);
    hint.set_text_wrapping(Gfx::TextWrapping::Wrap);
    hint.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    window->show();
    return app->exec();
}
