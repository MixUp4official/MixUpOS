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
#include <LibGUI/Icon.h>
#include <LibGUI/Label.h>
#include <LibGUI/Process.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

static void add_launch_button(GUI::Widget& parent, GUI::Window& window, StringView label, StringView executable)
{
    auto& button = parent.add<GUI::Button>(MUST(String::from_utf8(label)));
    button.set_fixed_height(30);
    button.on_click = [&window, executable](auto) {
        GUI::Process::spawn_or_show_error(&window, executable, Array<StringView, 0> {});
    };
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd rpath exec proc"));
    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd rpath exec"));
    TRY(Core::System::unveil("/bin/CodeIt", "x"));
    TRY(Core::System::unveil("/bin/Terminal", "x"));
    TRY(Core::System::unveil("/bin/FileManager", "x"));
    TRY(Core::System::unveil("/bin/Browser", "x"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto window = GUI::Window::construct();
    window->set_title("DevLaunchpad");
    window->resize(360, 220);
    window->set_resizable(false);
    auto app_icon = GUI::Icon::default_icon("app-terminal"sv);
    window->set_icon(app_icon.bitmap_for_size(16));

    auto& root = window->set_main_widget<GUI::Widget>();
    root.set_fill_with_background_color(true);
    root.set_layout<GUI::VerticalBoxLayout>(8, 8);

    auto& title = root.add<GUI::Label>("DevLaunchpad — Quick start tools"_string);
    title.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    add_launch_button(root, *window, "Open CodeIt"sv, "/bin/CodeIt"sv);
    add_launch_button(root, *window, "Open Terminal"sv, "/bin/Terminal"sv);
    add_launch_button(root, *window, "Open FileManager"sv, "/bin/FileManager"sv);
    add_launch_button(root, *window, "Open Browser"sv, "/bin/Browser"sv);

    auto& close = root.add<GUI::Button>("Close"_string);
    close.set_fixed_height(28);
    close.on_click = [](auto) { GUI::Application::the()->quit(); };

    window->show();
    return app->exec();
}
