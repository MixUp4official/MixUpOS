/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/String.h>
#include <LibCore/System.h>
#include <LibCore/Timer.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Label.h>
#include <LibGUI/SpinBox.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

static String format_seconds(i32 seconds)
{
    auto minutes = seconds / 60;
    auto remaining_seconds = seconds % 60;
    return MUST(String::formatted("{:02}:{:02}", minutes, remaining_seconds));
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd rpath wpath cpath"));
    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd rpath"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto window = GUI::Window::construct();
    window->set_title("FocusTimer");
    window->resize(320, 170);
    window->set_resizable(false);
    auto app_icon = GUI::Icon::default_icon("app-clock"sv);
    window->set_icon(app_icon.bitmap_for_size(16));

    auto& root = window->set_main_widget<GUI::Widget>();
    root.set_fill_with_background_color(true);
    root.set_layout<GUI::VerticalBoxLayout>(8, 8);

    auto& heading = root.add<GUI::Label>("FocusTimer — Pomodoro helper"_string);
    heading.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    auto& minute_selector = root.add<GUI::SpinBox>();
    minute_selector.set_range(1, 180);
    minute_selector.set_value(25);
    minute_selector.set_fixed_height(24);

    auto& time_label = root.add<GUI::Label>(format_seconds(25 * 60));
    time_label.set_text_alignment(Gfx::TextAlignment::Center);
    time_label.set_fixed_height(40);

    auto& button_row = root.add<GUI::Widget>();
    button_row.set_layout<GUI::HorizontalBoxLayout>(6);

    auto& start_button = button_row.add<GUI::Button>("Start"_string);
    auto& stop_button = button_row.add<GUI::Button>("Stop"_string);
    auto& reset_button = button_row.add<GUI::Button>("Reset"_string);

    i32 remaining_seconds = minute_selector.value() * 60;
    bool is_running = false;

    auto timer = TRY(Core::Timer::create_repeating(1000, [&] {
        if (!is_running)
            return;
        if (remaining_seconds > 0)
            --remaining_seconds;
        time_label.set_text(format_seconds(remaining_seconds));
        if (remaining_seconds == 0)
            is_running = false;
    }));

    minute_selector.on_change = [&](int value) {
        if (is_running)
            return;
        remaining_seconds = value * 60;
        time_label.set_text(format_seconds(remaining_seconds));
    };

    start_button.on_click = [&](auto) {
        is_running = true;
    };

    stop_button.on_click = [&](auto) {
        is_running = false;
    };

    reset_button.on_click = [&](auto) {
        is_running = false;
        remaining_seconds = minute_selector.value() * 60;
        time_label.set_text(format_seconds(remaining_seconds));
    };

    window->show();
    return app->exec();
}
