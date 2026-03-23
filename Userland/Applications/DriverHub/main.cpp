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
#include <LibGfx/Font/FontDatabase.h>
#include <LibMain/Main.h>

struct DriverStatus {
    ByteString name;
    ByteString detail;
    int health_score;
};

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
    window->resize(640, 400);

    Array<DriverStatus, 5> statuses = {
        DriverStatus { "WLAN Bridge", "Intel AX2xx + Realtek fallback profile active", 92 },
        DriverStatus { "GPU Bridge", "Mesa fallback + Vulkan compatibility shim loaded", 88 },
        DriverStatus { "Audio Bridge", "ALSA profile mapping and low-latency mode enabled", 95 },
        DriverStatus { "Filesystem Bridge", "ext4 / btrfs helpers with safe-mount defaults", 90 },
        DriverStatus { "Input Bridge", "Gamepad, touch, and high-polling mice merged", 93 },
    };

    auto model = GUI::ItemListModel<ByteString>::create({
        "WLAN Bridge",
        "GPU Bridge",
        "Audio Bridge",
        "Filesystem Bridge",
        "Input Bridge",
    });

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
    detail_text.set_autosize(false);
    auto& health_label = details_frame.add<GUI::Label>("Health score: -");
    auto& health_bar = details_frame.add<GUI::Progressbar>();
    health_bar.set_range(0, 100);
    health_bar.set_value(0);

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);

    auto& summary = actions.add<GUI::Label>("Overall compatibility: 91%");
    auto& refresh_button = actions.add<GUI::Button>("Refresh status");

    auto update_from_selection = [&] {
        auto index = driver_list.selection().first();
        if (!index.is_valid()) {
            selected_driver.set_text("Select a module to view details");
            detail_text.set_text("No module selected.");
            health_label.set_text("Health score: -");
            health_bar.set_value(0);
            return;
        }
        auto const& status = statuses[index.row()];
        selected_driver.set_text(ByteString::formatted("{}", status.name));
        detail_text.set_text(status.detail);
        health_label.set_text(ByteString::formatted("Health score: {}%", status.health_score));
        health_bar.set_value(status.health_score);
    };

    driver_list.on_selection_change = update_from_selection;
    refresh_button.on_click = [&](auto) {
        summary.set_text("Overall compatibility: 94% (re-scanned)");
        update_from_selection();
    };

    driver_list.set_cursor(model->index(0, 0), GUI::AbstractView::SelectionUpdate::Set);

    window->show();
    return app->exec();
}
