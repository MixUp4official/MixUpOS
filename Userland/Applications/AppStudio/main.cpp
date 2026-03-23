/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <AK/HashMap.h>
#include <AK/Optional.h>
#include <AK/Vector.h>
#include <LibCore/Process.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/CheckBox.h>
#include <LibGUI/Frame.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Label.h>
#include <LibGUI/ListView.h>
#include <LibGUI/Model.h>
#include <LibGUI/Progressbar.h>
#include <LibGUI/Window.h>
#include <LibGUI/Widget.h>
#include <LibMain/Main.h>

struct AppEntry {
    ByteString name;
    ByteString description;
    ByteString executable;
};

struct GraphicsProbe {
    bool has_opengl { false };
    bool has_vulkan { false };
};

static bool executable_exists(ByteString const& executable)
{
    return FileSystem::exists(executable);
}

static GraphicsProbe probe_graphics_stack()
{
    GraphicsProbe probe;
    probe.has_opengl = FileSystem::exists("/usr/lib/libGL.so") || FileSystem::exists("/usr/lib/libGL.so.1") || FileSystem::exists("/usr/share/graphics/mixgpu-opengl.profile");
    probe.has_vulkan = FileSystem::exists("/usr/lib/libvulkan.so") || FileSystem::exists("/usr/lib/libvulkan.so.1") || FileSystem::exists("/usr/share/graphics/mixgpu-vulkan.profile");
    return probe;
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd rpath unix"));
    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd rpath proc exec"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil("/bin", "x"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto icon = TRY(GUI::Icon::try_create_default_icon("app-settings"sv));

    auto window = TRY(GUI::Window::try_create());
    window->set_title("AppStudio - Custom App Control Center");
    window->set_icon(icon.bitmap_for_size(16));
    window->resize(720, 430);

    Array<AppEntry, 5> apps = {
        AppEntry { "DriverHub", "Driver and compatibility monitor", "/bin/DriverHub" },
        AppEntry { "LauncherForge", "Launcher profile builder", "/bin/LauncherForge" },
        AppEntry { "FeatureBoard", "Feature roadmap and progress board", "/bin/FeatureBoard" },
        AppEntry { "AppStudio", "Control center for all custom apps", "/bin/AppStudio" },
        AppEntry { "SystemTweaks", "Preset toggles for compatibility and visuals", "/bin/SystemTweaks" },
    };

    Vector<ByteString> app_names;
    HashMap<ByteString, AppEntry const*> app_lookup;
    app_names.ensure_capacity(apps.size());
    for (auto const& app_entry : apps) {
        app_names.append(app_entry.name);
        app_lookup.set(app_entry.name, &app_entry);
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
    auto& graphics_stack = detail_frame.add<GUI::Label>("Graphics stack: probing OpenGL/Vulkan availability");
    auto& quick_hint = detail_frame.add<GUI::Label>("Tip: use quick launch for the selected app.");
    auto& suite_health = detail_frame.add<GUI::Progressbar>();
    suite_health.set_range(0, 100);
    auto& install_breakdown = detail_frame.add<GUI::Label>("Installed: - | Missing: -");

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);

    auto& launch_btn = actions.add<GUI::Button>("Quick launch");
    auto& launch_all_btn = actions.add<GUI::Button>("Launch all installed");
    auto& open_experimentalmix = actions.add<GUI::Button>("Open ExperimentalMix");
    auto& scan_btn = actions.add<GUI::Button>("Rescan suite");
    auto& auto_rescan = actions.add<GUI::CheckBox>("Auto-rescan after launch");
    auto& windows_comparable_mode = actions.add<GUI::CheckBox>("Windows-comparable profile");
    auto& status = root.add<GUI::Label>("Suite status: pending first scan");

    auto run_app = [&](ByteString const& executable) -> ErrorOr<void> {
        Vector<StringView> launch_arguments;
        TRY(Core::Process::spawn(executable, launch_arguments.span()));
        return {};
    };

    auto refresh_suite_state = [&]() -> int {
        int installed_count = 0;
        for (auto const& app_entry : apps) {
            if (executable_exists(app_entry.executable))
                ++installed_count;
        }

        suite_health.set_value((installed_count * 100) / apps.size());
        install_breakdown.set_text(ByteString::formatted("Installed: {} | Missing: {}", installed_count, apps.size() - installed_count));

        auto probe = probe_graphics_stack();
        if (probe.has_opengl && probe.has_vulkan)
            graphics_stack.set_text("Graphics stack: OpenGL + Vulkan runtime detected");
        else if (probe.has_opengl)
            graphics_stack.set_text("Graphics stack: OpenGL detected, Vulkan missing");
        else if (probe.has_vulkan)
            graphics_stack.set_text("Graphics stack: Vulkan detected, OpenGL missing");
        else
            graphics_stack.set_text("Graphics stack: neither OpenGL nor Vulkan runtime detected");

        return installed_count;
    };

    auto update_selected_app_details = [&] {
        auto index = app_list.selection().first();
        if (!index.is_valid()) {
            selected_app.set_text("Select an app to inspect");
            app_desc.set_text("No app selected.");
            executable_label.set_text("Executable: -");
            availability_label.set_text("Status: -");
            return;
        }

        auto app_name = model->data(index, GUI::ModelRole::Display).to_byte_string();
        auto maybe_entry = app_lookup.get(app_name);
        if (!maybe_entry.has_value())
            return;
        auto const& app_entry = *maybe_entry.value();

        bool is_installed = executable_exists(app_entry.executable);
        selected_app.set_text(ByteString::formatted("Selected: {}", app_entry.name));
        app_desc.set_text(app_entry.description);
        executable_label.set_text(ByteString::formatted("Executable: {}", app_entry.executable));
        availability_label.set_text(is_installed ? "Status: Installed" : "Status: Missing executable");
        quick_hint.set_text(is_installed ? "Tip: quick launch starts the selected app in a new process."
                                        : "Tip: install this app binary before launching.");
    };

    app_list.on_selection_change = update_selected_app_details;

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
        auto const& app_entry = *maybe_entry.value();

        if (!executable_exists(app_entry.executable)) {
            status.set_text(ByteString::formatted("Cannot launch {}: executable missing", app_entry.name));
            return;
        }

        if (auto launch_result = run_app(app_entry.executable); launch_result.is_error()) {
            status.set_text(ByteString::formatted("Launch failed for {}: {}", app_entry.name, launch_result.error()));
            return;
        }

        status.set_text(ByteString::formatted("Launched {}", app_entry.name));
        if (auto_rescan.is_checked()) {
            auto installed_count = refresh_suite_state();
            status.set_text(ByteString::formatted("Launched {} | suite healthy: {}/{}", app_entry.name, installed_count, apps.size()));
        }
    };

    launch_all_btn.on_click = [&](auto) {
        int launched_count = 0;
        int skipped_count = 0;
        for (auto const& app_entry : apps) {
            if (!executable_exists(app_entry.executable)) {
                ++skipped_count;
                continue;
            }
            if (run_app(app_entry.executable).is_error()) {
                ++skipped_count;
                continue;
            }
            ++launched_count;
        }

        status.set_text(ByteString::formatted("Batch launch done: {} launched, {} skipped.", launched_count, skipped_count));
        if (auto_rescan.is_checked())
            refresh_suite_state();
    };

    open_experimentalmix.on_click = [&](auto) {
        if (auto launch_result = run_app("/bin/Browser"sv); launch_result.is_error()) {
            status.set_text(ByteString::formatted("Cannot open ExperimentalMix: {}", launch_result.error()));
            return;
        }
        status.set_text("ExperimentalMix started.");
    };

    scan_btn.on_click = [&](auto) {
        auto installed_count = refresh_suite_state();
        status.set_text(ByteString::formatted("Rescan complete: {}/{} apps available.", installed_count, apps.size()));
        update_selected_app_details();
    };

    windows_comparable_mode.on_checked = [&](bool checked) {
        if (checked)
            status.set_text("Windows-comparable profile enabled: prioritize Vulkan + driver diagnostics.");
        else
            status.set_text("Windows-comparable profile disabled.");
    };

    app_list.set_cursor(model->index(0, 0), GUI::AbstractView::SelectionUpdate::Set);
    heading.set_tooltip("Unified dashboard for all newly added custom applications.");
    auto installed_count = refresh_suite_state();
    status.set_text(ByteString::formatted("Suite healthy: {}/{} apps ready", installed_count, apps.size()));

    window->show();
    return app->exec();
}
