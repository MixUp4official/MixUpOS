/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/System.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/CheckBox.h>
#include <LibGUI/Frame.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Label.h>
#include <LibGUI/Progressbar.h>
#include <LibGUI/TextBox.h>
#include <LibGUI/Window.h>
#include <LibGUI/Widget.h>
#include <LibMain/Main.h>

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd rpath unix"));
    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd rpath"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto icon = TRY(GUI::Icon::try_create_default_icon("app-settings"sv));

    auto window = TRY(GUI::Window::try_create());
    window->set_title("LauncherForge - Minecraft Profile Builder");
    window->set_icon(icon.bitmap_for_size(16));
    window->resize(620, 340);

    auto& root = window->set_main_widget<GUI::Widget>();
    root.set_fill_with_background_color(true);
    root.set_layout<GUI::VerticalBoxLayout>();
    root.layout()->set_spacing(8);
    root.layout()->set_margins(8);

    root.add<GUI::Label>("Build custom launcher profiles with OpenJDK and mod libraries");

    auto& setup_frame = root.add<GUI::Frame>();
    setup_frame.set_frame_shape(GUI::FrameShape::Container);
    setup_frame.set_layout<GUI::VerticalBoxLayout>();
    setup_frame.layout()->set_margins(8);
    setup_frame.layout()->set_spacing(6);

    setup_frame.add<GUI::Label>("OpenJDK path");
    auto& java_home = setup_frame.add<GUI::TextBox>();
    java_home.set_text("/usr/lib/jvm/openjdk-21");

    setup_frame.add<GUI::Label>("Game libraries path");
    auto& libs_path = setup_frame.add<GUI::TextBox>();
    libs_path.set_text("/opt/mixupos/minecraft-libs");

    auto& flags_row = setup_frame.add<GUI::Widget>();
    flags_row.set_layout<GUI::HorizontalBoxLayout>();
    flags_row.layout()->set_spacing(8);
    auto& optimize_memory = flags_row.add<GUI::CheckBox>("Tune memory flags");
    auto& enable_mods = flags_row.add<GUI::CheckBox>("Enable curated mods");
    auto& safe_mode = flags_row.add<GUI::CheckBox>("Safe fallback mode");

    auto& progress = root.add<GUI::Progressbar>();
    progress.set_range(0, 100);
    progress.set_value(10);

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);

    auto& generate_profile = actions.add<GUI::Button>("Generate profile");
    auto& export_profile = actions.add<GUI::Button>("Export launcher JSON");
    auto& status = root.add<GUI::Label>("Ready to generate profile");

    auto update_progress = [&] {
        int score = 30;
        score += optimize_memory.is_checked() ? 20 : 0;
        score += enable_mods.is_checked() ? 30 : 0;
        score += safe_mode.is_checked() ? 20 : 0;
        progress.set_value(score);
    };

    optimize_memory.on_checked = [&](auto) { update_progress(); };
    enable_mods.on_checked = [&](auto) { update_progress(); };
    safe_mode.on_checked = [&](auto) { update_progress(); };

    generate_profile.on_click = [&](auto) {
        status.set_text(ByteString::formatted("Profile ready: JDK={} | Libs={}", java_home.text(), libs_path.text()));
    };

    export_profile.on_click = [&](auto) {
        status.set_text("launcher_profile.json exported to /home/anon/Downloads (simulated)");
    };

    window->show();
    return app->exec();
}
