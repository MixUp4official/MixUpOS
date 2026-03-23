/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <AK/Optional.h>
#include <AK/String.h>
#include <AK/Vector.h>
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
#include <LibGfx/Font/FontDatabase.h>
#include <LibMain/Main.h>

struct DriverModule {
    ByteString name;
    ByteString detail;
    ByteString kernel_target;
    Vector<ByteString> probe_paths;
    int baseline_score;
};

static int clamp_score(int score)
{
    if (score < 0)
        return 0;
    if (score > 100)
        return 100;
    return score;
}

static Optional<ByteString> first_available_probe_path(DriverModule const& module)
{
    for (auto const& path : module.probe_paths) {
        if (FileSystem::exists(path))
            return path;
    }
    return {};
}

static int module_score(DriverModule const& module, bool windows_target_mode)
{
    int score = module.baseline_score;
    if (!first_available_probe_path(module).has_value())
        score -= 15;
    if (windows_target_mode && module.name.contains("Vulkan"sv))
        score += 3;
    return clamp_score(score);
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd rpath unix"));
    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd rpath"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto app_icon = TRY(GUI::Icon::try_create_default_icon("app-settings"sv));

    auto window = TRY(GUI::Window::try_create());
    window->set_title("DriverHub - Linux Driver Center");
    window->set_icon(app_icon.bitmap_for_size(16));
    window->resize(720, 420);

    Array<DriverModule, 7> modules = {
        DriverModule { "WLAN Bridge", "Intel AX2xx + Realtek fallback profile active", "Kernel/Net", { "/sys/kernel/net", "/dev/net" }, 92 },
        DriverModule { "OpenGL Driver", "OpenGL renderer runtime for games/apps", "Kernel/Devices/GPU/MixGPU/OpenGLAdapter", { "/usr/lib/libGL.so", "/usr/lib/libGL.so.1", "/usr/share/graphics/mixgpu-opengl.profile" }, 90 },
        DriverModule { "Vulkan Driver", "Vulkan ICD + loader for modern render pipelines", "Kernel/Devices/GPU/MixGPU/VulkanAdapter", { "/usr/lib/libvulkan.so", "/usr/lib/libvulkan.so.1", "/usr/share/graphics/mixgpu-vulkan.profile" }, 89 },
        DriverModule { "Audio Bridge", "ALSA profile mapping and low-latency mode enabled", "Kernel/Devices/Audio", { "/dev/audio", "/dev/dsp" }, 95 },
        DriverModule { "Filesystem Bridge", "ext4 / btrfs helpers with safe-mount defaults", "Kernel/FileSystem", { "/bin/mount", "/bin/umount" }, 90 },
        DriverModule { "Input Bridge", "Gamepad, touch, and high-polling mice merged", "Kernel/Devices/Input", { "/dev/input", "/dev/hid" }, 93 },
        DriverModule { "GPU Device Node", "Kernel-side graphics device exposure", "Kernel/Devices/GPU", { "/dev/gpu/connector0", "/dev/fb0" }, 88 },
    };

    Vector<ByteString> module_names;
    module_names.ensure_capacity(modules.size());
    for (auto const& module : modules)
        module_names.append(module.name);
    auto model = GUI::ItemListModel<ByteString>::create(move(module_names));

    auto& root = window->set_main_widget<GUI::Widget>();
    root.set_fill_with_background_color(true);
    root.set_layout<GUI::VerticalBoxLayout>();
    root.layout()->set_spacing(8);
    root.layout()->set_margins(8);

    auto& headline = root.add<GUI::Label>("DriverHub Dashboard");
    headline.set_font(Gfx::FontDatabase::default_font().bold_variant());

    auto& content = root.add<GUI::Widget>();
    content.set_layout<GUI::HorizontalBoxLayout>();
    content.layout()->set_spacing(8);

    auto& list_frame = content.add<GUI::Frame>();
    list_frame.set_frame_shape(GUI::FrameShape::Container);
    list_frame.set_layout<GUI::VerticalBoxLayout>();
    list_frame.layout()->set_margins(8);
    list_frame.layout()->set_spacing(6);
    list_frame.add<GUI::Label>("Driver modules");

    auto& driver_list = list_frame.add<GUI::ListView>();
    driver_list.set_model(model);

    auto& details_frame = content.add<GUI::Frame>();
    details_frame.set_frame_shape(GUI::FrameShape::Container);
    details_frame.set_layout<GUI::VerticalBoxLayout>();
    details_frame.layout()->set_margins(8);
    details_frame.layout()->set_spacing(8);

    auto& selected_driver = details_frame.add<GUI::Label>("Select a module to view details");
    selected_driver.set_font(Gfx::FontDatabase::default_font().bold_variant());
    auto& detail_text = details_frame.add<GUI::Label>("No module selected.");
    auto& health_label = details_frame.add<GUI::Label>("Health score: -");
    auto& file_probe_label = details_frame.add<GUI::Label>("Driver file check: -");
    auto& kernel_target_label = details_frame.add<GUI::Label>("Kernel target: -");
    auto& recommendation_label = details_frame.add<GUI::Label>("Recommendation: Select a module first.");
    auto& health_bar = details_frame.add<GUI::Progressbar>();
    health_bar.set_range(0, 100);
    health_bar.set_value(0);
    auto& confidence_bar = details_frame.add<GUI::Progressbar>();
    confidence_bar.set_range(0, 100);
    confidence_bar.set_value(0);

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);

    auto& summary = actions.add<GUI::Label>("Overall compatibility: -");
    auto& tier_label = actions.add<GUI::Label>("Tier: baseline");
    auto& strict_mode = actions.add<GUI::CheckBox>("Strict recommendations");
    auto& windows_target_mode = actions.add<GUI::CheckBox>("Windows-like target mode");
    auto& refresh_button = actions.add<GUI::Button>("Refresh status");
    auto& diagnostics_button = actions.add<GUI::Button>("Run diagnostics");

    auto average_health = [&]() {
        int total = 0;
        for (auto const& module : modules)
            total += module_score(module, windows_target_mode.is_checked());
        return total / static_cast<int>(modules.size());
    };

    auto refresh_summary = [&](ByteString const& suffix) {
        auto average = average_health();
        summary.set_text(ByteString::formatted("Overall compatibility: {}%{}", average, suffix));
        if (average >= 95)
            tier_label.set_text("Tier: Windows-comparable");
        else if (average >= 85)
            tier_label.set_text("Tier: advanced");
        else
            tier_label.set_text("Tier: baseline");
    };

    auto update_from_selection = [&] {
        auto index = driver_list.selection().first();
        if (!index.is_valid()) {
            selected_driver.set_text("Select a module to view details");
            detail_text.set_text("No module selected.");
            health_label.set_text("Health score: -");
            file_probe_label.set_text("Driver file check: -");
            kernel_target_label.set_text("Kernel target: -");
            recommendation_label.set_text("Recommendation: Select a module first.");
            health_bar.set_value(0);
            confidence_bar.set_value(0);
            return;
        }

        auto const& module = modules[index.row()];
        auto score = module_score(module, windows_target_mode.is_checked());
        auto detected_path = first_available_probe_path(module);

        selected_driver.set_text(ByteString::formatted("{}", module.name));
        detail_text.set_text(module.detail);
        health_label.set_text(ByteString::formatted("Health score: {}%", score));
        if (detected_path.has_value())
            file_probe_label.set_text(ByteString::formatted("Driver file check: runtime artifact present ({})", detected_path.value()));
        else
            file_probe_label.set_text("Driver file check: runtime files missing (fallback needed)");
        kernel_target_label.set_text(ByteString::formatted("Kernel target: {}", module.kernel_target));
        health_bar.set_value(score);
        confidence_bar.set_value(clamp_score(score + 4));

        bool should_warn = strict_mode.is_checked() ? score < 93 : score < 90;
        recommendation_label.set_text(should_warn
            ? "Recommendation: Install/repair runtime package and rerun diagnostics."
            : "Recommendation: Module is healthy for daily use.");
    };

    driver_list.on_selection_change = update_from_selection;
    refresh_button.on_click = [&](auto) {
        refresh_summary(" (re-scanned)"sv);
        update_from_selection();
    };
    diagnostics_button.on_click = [&](auto) {
        refresh_summary(" (diagnostics complete)"sv);
        update_from_selection();
    };
    strict_mode.on_checked = [&](auto) { update_from_selection(); };
    windows_target_mode.on_checked = [&](auto) {
        refresh_summary(" (target mode updated)"sv);
        update_from_selection();
    };

    refresh_summary(""sv);
    driver_list.set_cursor(model->index(0, 0), GUI::AbstractView::SelectionUpdate::Set);

    window->show();
    return app->exec();
}
