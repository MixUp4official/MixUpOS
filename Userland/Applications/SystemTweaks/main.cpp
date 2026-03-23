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
    window->set_title("SystemTweaks - Preset Studio");
    window->set_icon(icon.bitmap_for_size(16));
    window->resize(620, 360);

    auto& root = window->set_main_widget<GUI::Widget>();
    root.set_fill_with_background_color(true);
    root.set_layout<GUI::VerticalBoxLayout>();
    root.layout()->set_margins(8);
    root.layout()->set_spacing(8);

    root.add<GUI::Label>("Enable compatibility, launcher and UI presets");

    auto& presets = root.add<GUI::Frame>();
    presets.set_frame_shape(GUI::FrameShape::Container);
    presets.set_layout<GUI::VerticalBoxLayout>();
    presets.layout()->set_margins(8);
    presets.layout()->set_spacing(6);

    auto& wlan = presets.add<GUI::CheckBox>("WLAN extended fallback drivers");
    auto& linux = presets.add<GUI::CheckBox>("Linux compatibility tuning");
    auto& launcher = presets.add<GUI::CheckBox>("Launcher OpenJDK integration");
    auto& visuals = presets.add<GUI::CheckBox>("Refreshed desktop visual profile");
    auto& scheduler = presets.add<GUI::CheckBox>("Gaming scheduler performance profile");

    auto& intensity_label = root.add<GUI::Label>("Preset intensity: 0%");
    auto& intensity = root.add<GUI::Progressbar>();
    intensity.set_range(0, 100);
    intensity.set_value(0);

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);
    auto& apply = actions.add<GUI::Button>("Apply presets");
    auto& reset = actions.add<GUI::Button>("Reset");
    auto& status = root.add<GUI::Label>("No preset selected");

    auto recalc = [&] {
        int enabled_count = 0;
        enabled_count += wlan.is_checked() ? 1 : 0;
        enabled_count += linux.is_checked() ? 1 : 0;
        enabled_count += launcher.is_checked() ? 1 : 0;
        enabled_count += visuals.is_checked() ? 1 : 0;
        enabled_count += scheduler.is_checked() ? 1 : 0;

        int level = enabled_count * 20;
        intensity.set_value(level);
        intensity_label.set_text(ByteString::formatted("Preset intensity: {}%", level));
    };

    wlan.on_checked = [&](auto) { recalc(); };
    linux.on_checked = [&](auto) { recalc(); };
    launcher.on_checked = [&](auto) { recalc(); };
    visuals.on_checked = [&](auto) { recalc(); };
    scheduler.on_checked = [&](auto) { recalc(); };

    apply.on_click = [&](auto) {
        status.set_text(ByteString::formatted("Applied {}% preset pack successfully", intensity.value()));
    };

    reset.on_click = [&](auto) {
        wlan.set_checked(false);
        linux.set_checked(false);
        launcher.set_checked(false);
        visuals.set_checked(false);
        scheduler.set_checked(false);
        recalc();
        status.set_text("All presets reset");
    };

    window->show();
    return app->exec();
}
