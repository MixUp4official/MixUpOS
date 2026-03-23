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
    window->set_title("FeatureBoard - Mega Update Tracker");
    window->set_icon(icon.bitmap_for_size(16));
    window->resize(720, 420);

    Array<ByteString, 20> features = {
        "WLAN compatibility presets",
        "GPU fallback profile",
        "Audio bridge profile",
        "Storage bridge profile",
        "Input bridge profile",
        "Launcher Java profile generator",
        "OpenJDK path configurator",
        "Mods library path configurator",
        "Feature dashboard",
        "App registry browser",
        "Performance profile: balanced",
        "Performance profile: gaming",
        "Linux syscall helper visibility",
        "Compatibility diagnostics panel",
        "Custom app quick launch map",
        "UI refresh baseline",
        "Driver health summary",
        "Profile export-ready strings",
        "Placeholder replacement status",
        "Integrated tweak launcher",
    };

    auto model = GUI::ItemListModel<ByteString>::create(Vector<ByteString>(features.span()));

    auto& root = window->set_main_widget<GUI::Widget>();
    root.set_fill_with_background_color(true);
    root.set_layout<GUI::VerticalBoxLayout>();
    root.layout()->set_margins(8);
    root.layout()->set_spacing(8);

    auto& header = root.add<GUI::Label>("20 Features shipped in the mega update");

    auto& body = root.add<GUI::Widget>();
    body.set_layout<GUI::HorizontalBoxLayout>();
    body.layout()->set_spacing(8);

    auto& list_frame = body.add<GUI::Frame>();
    list_frame.set_frame_shape(GUI::FrameShape::Container);
    list_frame.set_layout<GUI::VerticalBoxLayout>();
    list_frame.layout()->set_margins(8);
    list_frame.layout()->set_spacing(6);

    auto& feature_list = list_frame.add<GUI::ListView>();
    feature_list.set_model(model);

    auto& right_panel = body.add<GUI::Frame>();
    right_panel.set_frame_shape(GUI::FrameShape::Container);
    right_panel.set_layout<GUI::VerticalBoxLayout>();
    right_panel.layout()->set_margins(8);
    right_panel.layout()->set_spacing(6);

    auto& selected_title = right_panel.add<GUI::Label>("Select a feature");
    auto& selected_info = right_panel.add<GUI::Label>("Feature details will appear here.");
    auto& completion_label = right_panel.add<GUI::Label>("Completion: 100%");
    auto& completion = right_panel.add<GUI::Progressbar>();
    completion.set_range(0, 100);
    completion.set_value(100);

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);
    auto& simulate = actions.add<GUI::Button>("Simulate next sprint");
    auto& release_state = actions.add<GUI::Label>("Release channel: stable");

    feature_list.on_selection_change = [&] {
        auto index = feature_list.selection().first();
        if (!index.is_valid())
            return;
        selected_title.set_text(ByteString::formatted("Feature {}", index.row() + 1));
        selected_info.set_text(features[index.row()]);
    };

    simulate.on_click = [&](auto) {
        release_state.set_text("Release channel: stable + hotfix pack queued");
        completion_label.set_text("Completion: 100% (validated)");
    };

    feature_list.set_cursor(model->index(0, 0), GUI::AbstractView::SelectionUpdate::Set);
    header.set_tooltip("Features are grouped across UI, drivers, launcher and compatibility.");

    window->show();
    return app->exec();
}
