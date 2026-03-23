/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <AK/LexicalPath.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Label.h>
#include <LibGUI/MessageBox.h>
#include <LibGUI/Process.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

enum class PrimaryAction {
    OpenInFileManager,
    OpenInHexEditor,
};

struct FileHandlingPlan {
    StringView headline;
    StringView detail;
    StringView primary_button_label;
    PrimaryAction primary_action;
};

static FileHandlingPlan plan_for_path(StringView path)
{
    auto lower_path = path.to_lowercase();

    if (lower_path.ends_with(".deb"sv)) {
        return {
            .headline = "Debian Package"sv,
            .detail = "This Linux package can be unpacked and explored in File Manager."sv,
            .primary_button_label = "Open Package"sv,
            .primary_action = PrimaryAction::OpenInFileManager,
        };
    }

    if (lower_path.ends_with(".rpm"sv)) {
        return {
            .headline = "RPM Package"sv,
            .detail = "RPM packages are recognized, but native installation is not implemented yet."sv,
            .primary_button_label = "Inspect Binary"sv,
            .primary_action = PrimaryAction::OpenInHexEditor,
        };
    }

    if (lower_path.ends_with(".appimage"sv)) {
        return {
            .headline = "AppImage Bundle"sv,
            .detail = "AppImage runtime support is not available yet on this system."sv,
            .primary_button_label = "Inspect Binary"sv,
            .primary_action = PrimaryAction::OpenInHexEditor,
        };
    }

    if (lower_path.ends_with(".kext"sv) || lower_path.contains(".kext/"sv)) {
        return {
            .headline = "macOS Kext Bundle"sv,
            .detail = "Kext bundles can be inspected and managed from /driverkext using KextManager."sv,
            .primary_button_label = "Open Bundle"sv,
            .primary_action = PrimaryAction::OpenInFileManager,
        };
    }

    if (lower_path.ends_with(".exe"sv) || lower_path.ends_with(".msi"sv) || lower_path.ends_with(".dll"sv)) {
        return {
            .headline = "Windows Binary"sv,
            .detail = "Windows executables and installers are not natively runnable yet in MixUp!OS."sv,
            .primary_button_label = "Inspect Binary"sv,
            .primary_action = PrimaryAction::OpenInHexEditor,
        };
    }

    if (lower_path.ends_with(".so"sv) || lower_path.ends_with(".dylib"sv)) {
        return {
            .headline = "Shared Library"sv,
            .detail = "Shared libraries are components for programs, not standalone apps you can launch directly."sv,
            .primary_button_label = "Inspect Binary"sv,
            .primary_action = PrimaryAction::OpenInHexEditor,
        };
    }

    if (lower_path.ends_with(".dmg"sv) || lower_path.ends_with(".pkg"sv)) {
        return {
            .headline = "Apple Package"sv,
            .detail = "Apple disk images and installer packages are recognized, but not supported natively yet."sv,
            .primary_button_label = "Inspect Binary"sv,
            .primary_action = PrimaryAction::OpenInHexEditor,
        };
    }

    return {
        .headline = "Foreign File"sv,
        .detail = "This file type is recognized, but does not have a native runtime yet."sv,
        .primary_button_label = "Inspect Binary"sv,
        .primary_action = PrimaryAction::OpenInHexEditor,
    };
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath unix proc exec"));

    ByteString path;
    Core::ArgsParser args_parser;
    args_parser.add_positional_argument(path, "Foreign file to inspect", "path", Core::ArgsParser::Required::No);
    args_parser.parse(arguments);

    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath proc exec"));
    TRY(Core::System::unveil("/bin/FileManager", "x"));
    TRY(Core::System::unveil("/bin/HexEditor", "x"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    if (path.is_empty()) {
        GUI::MessageBox::show_error(nullptr, "No file path was provided.");
        return 1;
    }

    if (auto real_path = FileSystem::real_path(path); !real_path.is_error())
        path = real_path.release_value();

    auto plan = plan_for_path(path);
    auto filename = LexicalPath(path).basename();

    auto window = GUI::Window::construct();
    window->set_title("Compatibility Assistant");
    window->resize(520, 220);
    window->set_resizable(false);
    auto app_icon = GUI::Icon::default_icon("app-settings"sv);
    window->set_icon(app_icon.bitmap_for_size(16));

    auto main_widget = window->set_main_widget<GUI::Widget>();
    main_widget->set_fill_with_background_color(true);
    main_widget->set_layout<GUI::VerticalBoxLayout>(8, 8);

    auto& headline_label = main_widget->add<GUI::Label>(MUST(String::from_utf8(plan.headline)));
    headline_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);
    headline_label.set_fixed_height(24);

    auto& filename_label = main_widget->add<GUI::Label>(MUST(String::formatted("File: {}", filename)));
    filename_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);
    filename_label.set_fixed_height(20);
    filename_label.set_text_wrapping(Gfx::TextWrapping::DontWrap);

    auto& path_label = main_widget->add<GUI::Label>(MUST(String::formatted("Path: {}", path)));
    path_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);
    path_label.set_fixed_height(20);
    path_label.set_text_wrapping(Gfx::TextWrapping::DontWrap);

    auto& detail_label = main_widget->add<GUI::Label>(MUST(String::from_utf8(plan.detail)));
    detail_label.set_text_alignment(Gfx::TextAlignment::TopLeft);
    detail_label.set_text_wrapping(Gfx::TextWrapping::Wrap);

    auto& button_row = main_widget->add<GUI::Widget>();
    button_row.set_layout<GUI::HorizontalBoxLayout>(GUI::Margins {}, 6);
    button_row.set_fixed_height(32);
    button_row.add_spacer();

    auto& primary_button = button_row.add<GUI::Button>(MUST(String::from_utf8(plan.primary_button_label)));
    primary_button.on_click = [window, path, plan](auto) {
        if (plan.primary_action == PrimaryAction::OpenInFileManager)
            GUI::Process::spawn_or_show_error(window.ptr(), "/bin/FileManager"sv, Array { path.bytes_as_string_view() });
        else
            GUI::Process::spawn_or_show_error(window.ptr(), "/bin/HexEditor"sv, Array { path.bytes_as_string_view() });
        GUI::Application::the()->quit();
    };

    auto& reveal_button = button_row.add<GUI::Button>("Reveal In Files"_string);
    reveal_button.on_click = [window, path](auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/FileManager"sv, Array { "-s"sv, "-r"sv, path.bytes_as_string_view() });
        GUI::Application::the()->quit();
    };

    auto& close_button = button_row.add<GUI::Button>("Close"_string);
    close_button.on_click = [](auto) {
        GUI::Application::the()->quit();
    };

    window->show();
    return app->exec();
}
