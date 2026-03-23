/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <LibCore/System.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/Frame.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Label.h>
#include <LibGUI/ListView.h>
#include <LibGUI/Model.h>
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
    window->set_title("AppStudio - Custom App Control Center");
    window->set_icon(icon.bitmap_for_size(16));
    window->resize(700, 420);

    Array<ByteString, 5> app_names = {
        "DriverHub",
        "LauncherForge",
        "FeatureBoard",
        "AppStudio",
        "SystemTweaks",
    };

    auto model = GUI::ItemListModel<ByteString>::create(Vector<ByteString>(app_names.span()));

    auto& root = window->set_main_widget<GUI::Widget>();
    root.set_fill_with_background_color(true);
    root.set_layout<GUI::VerticalBoxLayout>();
    root.layout()->set_margins(8);
    root.layout()->set_spacing(8);

    auto& heading = root.add<GUI::Label>("Custom Apps Suite");

    auto& body = root.add<GUI::Widget>();
    body.set_layout<GUI::HorizontalBoxLayout>();
    body.layout()->set_spacing(8);

    auto& list_frame = body.add<GUI::Frame>();
    list_frame.set_frame_shape(GUI::FrameShape::Container);
    list_frame.set_layout<GUI::VerticalBoxLayout>();
    list_frame.layout()->set_margins(8);
    list_frame.layout()->set_spacing(6);
    list_frame.add<GUI::Label>("Installed apps");

    auto& app_list = list_frame.add<GUI::ListView>();
    app_list.set_model(model);

    auto& detail_frame = body.add<GUI::Frame>();
    detail_frame.set_frame_shape(GUI::FrameShape::Container);
    detail_frame.set_layout<GUI::VerticalBoxLayout>();
    detail_frame.layout()->set_margins(8);
    detail_frame.layout()->set_spacing(8);

    auto& selected_app = detail_frame.add<GUI::Label>("Select an app to inspect");
    auto& app_desc = detail_frame.add<GUI::Label>("No app selected.");
    auto& suite_health = detail_frame.add<GUI::Progressbar>();
    suite_health.set_range(0, 100);
    suite_health.set_value(96);

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);

    auto& launch_btn = actions.add<GUI::Button>("Quick launch (simulated)");
    auto& scan_btn = actions.add<GUI::Button>("Rescan suite");
    auto& status = root.add<GUI::Label>("Suite healthy: 5/5 apps ready");

    app_list.on_selection_change = [&] {
        auto index = app_list.selection().first();
        if (!index.is_valid())
            return;

        auto const& app_name = app_names[index.row()];
        selected_app.set_text(ByteString::formatted("Selected: {}", app_name));

        if (app_name == "DriverHub")
            app_desc.set_text("Driver and Linux compatibility monitor.");
        else if (app_name == "LauncherForge")
            app_desc.set_text("Minecraft launcher/OpenJDK profile builder.");
        else if (app_name == "FeatureBoard")
            app_desc.set_text("Feature roadmap and rollout tracker.");
        else if (app_name == "SystemTweaks")
            app_desc.set_text("Preset toggles for compatibility and visuals.");
        else
            app_desc.set_text("Control center for all custom apps.");
    };

    launch_btn.on_click = [&](auto) {
        status.set_text("Quick launch requested for selected app (simulated).");
    };

    scan_btn.on_click = [&](auto) {
        suite_health.set_value(100);
        status.set_text("Rescan complete: all custom apps verified.");
    };

    app_list.set_cursor(model->index(0, 0), GUI::AbstractView::SelectionUpdate::Set);
    heading.set_tooltip("Unified dashboard for all newly added custom applications.");

    window->show();
    return app->exec();
}
