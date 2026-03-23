/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/String.h>
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

static bool looks_like_absolute_path(ByteString const& path)
{
    return path.starts_with('/');
}

static int clamp_quality(int value)
{
    if (value < 0)
        return 0;
    if (value > 100)
        return 100;
    return value;
}

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
    auto& quality_label = root.add<GUI::Label>("Profile quality: 10%");

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);

    auto& generate_profile = actions.add<GUI::Button>("Generate profile");
    auto& export_profile = actions.add<GUI::Button>("Export launcher JSON");
    auto& vanilla_preset = actions.add<GUI::Button>("Vanilla preset");
    auto& modded_preset = actions.add<GUI::Button>("Modded preset");
    auto& reset_all = actions.add<GUI::Button>("Reset");
    auto& status = root.add<GUI::Label>("Ready to generate profile");

    auto calculate_quality = [&] {
        int score = 0;
        score += looks_like_absolute_path(java_home.text()) ? 30 : 5;
        score += looks_like_absolute_path(libs_path.text()) ? 25 : 5;
        score += optimize_memory.is_checked() ? 15 : 0;
        score += enable_mods.is_checked() ? 20 : 0;
        score += safe_mode.is_checked() ? 10 : 0;
        return clamp_quality(score);
    };

    auto update_progress = [&] {
        auto score = calculate_quality();
        progress.set_value(score);
        quality_label.set_text(ByteString::formatted("Profile quality: {}%", score));
    };

    optimize_memory.on_checked = [&](auto) { update_progress(); };
    enable_mods.on_checked = [&](auto) { update_progress(); };
    safe_mode.on_checked = [&](auto) { update_progress(); };
    java_home.on_change = [&] { update_progress(); };
    libs_path.on_change = [&] { update_progress(); };

    generate_profile.on_click = [&](auto) {
        if (!looks_like_absolute_path(java_home.text()) || !looks_like_absolute_path(libs_path.text())) {
            status.set_text("Profile generation failed: use absolute OpenJDK and library paths.");
            return;
        }

        status.set_text(ByteString::formatted("Profile ready: JDK={} | Libs={}", java_home.text(), libs_path.text()));
    };

    export_profile.on_click = [&](auto) {
        if (progress.value() < 45) {
            status.set_text("Export blocked: profile quality too low. Use presets or validate paths.");
            return;
        }
        status.set_text("launcher_profile.json exported to /home/anon/Downloads (simulated)");
    };

    vanilla_preset.on_click = [&](auto) {
        optimize_memory.set_checked(true);
        enable_mods.set_checked(false);
        safe_mode.set_checked(true);
        update_progress();
        status.set_text("Vanilla preset applied.");
    };

    modded_preset.on_click = [&](auto) {
        optimize_memory.set_checked(true);
        enable_mods.set_checked(true);
        safe_mode.set_checked(false);
        update_progress();
        status.set_text("Modded preset applied.");
    };

    reset_all.on_click = [&](auto) {
        java_home.set_text("/usr/lib/jvm/openjdk-21");
        libs_path.set_text("/opt/mixupos/minecraft-libs");
        optimize_memory.set_checked(false);
        enable_mods.set_checked(false);
        safe_mode.set_checked(false);
        update_progress();
        status.set_text("Form reset to defaults.");
    };

    update_progress();

    window->show();
    return app->exec();
}
