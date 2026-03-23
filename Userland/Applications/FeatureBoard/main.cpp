/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <AK/Vector.h>
#include <LibCore/System.h>
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

struct FeatureItem {
    ByteString title;
    ByteString scope;
    bool validated { true };
};

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

    Vector<FeatureItem> features = {
        { "WLAN compatibility presets", "Core platform + networking" },
        { "GPU fallback profile", "Core platform + graphics" },
        { "Audio bridge profile", "Core platform + multimedia" },
        { "Storage bridge profile", "Core platform + filesystem" },
        { "Input bridge profile", "Core platform + input" },
        { "Launcher Java profile generator", "Launcher + runtime" },
        { "OpenJDK path configurator", "Launcher + runtime" },
        { "Mods library path configurator", "Launcher + runtime" },
        { "Feature dashboard", "UI + product" },
        { "App registry browser", "UI + app management" },
        { "Performance profile: balanced", "Performance + tuning" },
        { "Performance profile: gaming", "Performance + tuning" },
        { "Linux syscall helper visibility", "Diagnostics + platform" },
        { "Compatibility diagnostics panel", "Diagnostics + drivers" },
        { "Custom app quick launch map", "UI + app management" },
        { "UI refresh baseline", "UI + visual consistency" },
        { "Driver health summary", "Diagnostics + drivers" },
        { "Profile export-ready strings", "Launcher + export" },
        { "Placeholder replacement status", "Quality + release" },
        { "Integrated tweak launcher", "UI + tweaks" },
    };

    Vector<ByteString> feature_names;
    feature_names.ensure_capacity(features.size());
    for (auto const& item : features)
        feature_names.append(item.title);

    auto model = GUI::ItemListModel<ByteString>::create(move(feature_names));

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
    auto& selected_scope = right_panel.add<GUI::Label>("Scope: -");
    auto& completion_label = right_panel.add<GUI::Label>("Completion: 95%");
    auto& completion = right_panel.add<GUI::Progressbar>();
    completion.set_range(0, 100);
    completion.set_value(95);
    auto& quality_gate = right_panel.add<GUI::CheckBox>("Mark selected feature as validated");

    auto& actions = root.add<GUI::Widget>();
    actions.set_layout<GUI::HorizontalBoxLayout>();
    actions.layout()->set_spacing(8);
    auto& simulate = actions.add<GUI::Button>("Simulate next sprint");
    auto& reset = actions.add<GUI::Button>("Reset board");
    auto& release_state = actions.add<GUI::Label>("Release channel: release-candidate");
    auto& metrics = root.add<GUI::Label>("Metrics: calculating");

    auto selected_index = Optional<size_t> {};

    auto validated_count = [&] {
        size_t count = 0;
        for (auto const& feature : features) {
            if (feature.validated)
                ++count;
        }
        return count;
    };

    auto refresh_metrics = [&] {
        auto validated = validated_count();
        metrics.set_text(ByteString::formatted("Metrics: {}/{} complete | {} validated", features.size(), features.size(), validated));
    };

    auto update_selection = [&] {
        auto index = feature_list.selection().first();
        if (!index.is_valid())
            return;

        selected_index = index.row();
        auto const& feature = features[index.row()];
        selected_title.set_text(ByteString::formatted("Feature {}", index.row() + 1));
        selected_info.set_text(feature.title);
        selected_scope.set_text(ByteString::formatted("Scope: {}", feature.scope));
        quality_gate.set_checked(feature.validated, GUI::AllowCallback::No);
    };

    feature_list.on_selection_change = update_selection;

    quality_gate.on_checked = [&](bool checked) {
        if (!selected_index.has_value())
            return;

        features[selected_index.value()].validated = checked;
        refresh_metrics();
    };

    simulate.on_click = [&](auto) {
        completion.set_value(100);
        completion_label.set_text("Completion: 100% (validated + regression checked)");
        release_state.set_text("Release channel: stable + hotfix pack queued");
        for (auto& feature : features)
            feature.validated = true;
        quality_gate.set_checked(true, GUI::AllowCallback::No);
        refresh_metrics();
    };

    reset.on_click = [&](auto) {
        completion.set_value(95);
        completion_label.set_text("Completion: 95%");
        release_state.set_text("Release channel: release-candidate");
        for (auto& feature : features)
            feature.validated = true;
        quality_gate.set_checked(true, GUI::AllowCallback::No);
        refresh_metrics();
        update_selection();
    };

    feature_list.set_cursor(model->index(0, 0), GUI::AbstractView::SelectionUpdate::Set);
    header.set_tooltip("Features are grouped across UI, drivers, launcher and compatibility.");
    refresh_metrics();

    window->show();
    return app->exec();
}
