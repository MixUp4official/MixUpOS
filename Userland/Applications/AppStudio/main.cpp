/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <AK/HashMap.h>
#include <AK/Vector.h>
#include <LibCore/System.h>
#include <LibCore/Process.h>
#include <LibFileSystem/FileSystem.h>
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

    struct AppEntry {
        ByteString name;
        ByteString description;
        ByteString executable;
    };

    Array<AppEntry, 5> apps = {
        AppEntry { "DriverHub", "Driver and compatibility monitor", "/bin/DriverHub" },
        AppEntry { "LauncherForge", "Launcher profile builder", "/bin/LauncherForge" },
        AppEntry { "FeatureBoard", "Feature roadmap and progress board", "/bin/FeatureBoard" },
        AppEntry { "AppStudio", "Control center for all custom apps", "/bin/AppStudio" },
        AppEntry { "SystemTweaks", "Preset toggles for compatibility and visuals", "/bin/SystemTweaks" },
    };

    Vector<ByteString> app_names;
    HashMap<ByteString, AppEntry const*> app_lookup;
    int detected_apps = 0;
    for (auto const& app_entry : apps) {
        app_names.append(app_entry.name);
        app_lookup.set(app_entry.name, &app_entry);
        if (FileSystem::exists(app_entry.executable))
            ++detected_apps;
    }

    auto model = GUI::ItemListModel<ByteString>::create(move(app_names));

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
    auto& executable_label = detail_frame.add<GUI::Label>("Executable: -");
    auto& availability_label = detail_frame.add<GUI::Label>("Status: -");
    auto& suite_health = detail_frame.add<GUI::Progressbar>();
    suite_health.set_range(0, 100);
    suite_health.set_value((detected_apps * 100) / apps.size());

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);

    auto& launch_btn = actions.add<GUI::Button>("Quick launch");
    auto& scan_btn = actions.add<GUI::Button>("Rescan suite");
    auto& status = root.add<GUI::Label>(ByteString::formatted("Suite healthy: {}/{} apps ready", detected_apps, apps.size()));

    app_list.on_selection_change = [&] {
        auto index = app_list.selection().first();
        if (!index.is_valid())
            return;

        auto app_name = model->data(index, GUI::ModelRole::Display).to_byte_string();
        auto maybe_entry = app_lookup.get(app_name);
        if (!maybe_entry.has_value())
            return;
        auto const& app = *maybe_entry.value();

        selected_app.set_text(ByteString::formatted("Selected: {}", app.name));
        app_desc.set_text(app.description);
        executable_label.set_text(ByteString::formatted("Executable: {}", app.executable));
        if (FileSystem::exists(app.executable))
            availability_label.set_text("Status: Installed");
        else
            availability_label.set_text("Status: Missing executable");
    };

    launch_btn.on_click = [&](auto) {
        auto index = app_list.selection().first();
        if (!index.is_valid()) {
            status.set_text("Select an app before launching.");
            return;
        }
        auto app_name = model->data(index, GUI::ModelRole::Display).to_byte_string();
        auto maybe_entry = app_lookup.get(app_name);
        if (!maybe_entry.has_value())
            return;

        auto const& app = *maybe_entry.value();
        if (!FileSystem::exists(app.executable)) {
            status.set_text(ByteString::formatted("Cannot launch {}: executable missing", app.name));
            return;
        }

        Vector<StringView> launch_arguments;
        if (auto spawn_result = Core::Process::spawn(app.executable, launch_arguments.span()); spawn_result.is_error()) {
            status.set_text(ByteString::formatted("Launch failed for {}: {}", app.name, spawn_result.error()));
            return;
        }
        status.set_text(ByteString::formatted("Launched {}", app.name));
    };

    scan_btn.on_click = [&](auto) {
        int installed_count = 0;
        for (auto const& app : apps) {
            if (FileSystem::exists(app.executable))
                ++installed_count;
        }
        suite_health.set_value((installed_count * 100) / apps.size());
        status.set_text(ByteString::formatted("Rescan complete: {}/{} apps available.", installed_count, apps.size()));
    };

    app_list.set_cursor(model->index(0, 0), GUI::AbstractView::SelectionUpdate::Set);
    heading.set_tooltip("Unified dashboard for all newly added custom applications.");

    window->show();
    return app->exec();
}
