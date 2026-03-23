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

struct OfficeTarget {
    StringView executable;
    StringView button_label;
    StringView description;
};

static Optional<OfficeTarget> target_for_path(StringView path)
{
    auto lower_path = path.to_lowercase();

    if (lower_path.ends_with(".doc"sv) || lower_path.ends_with(".docx"sv) || lower_path.ends_with(".odt"sv) || lower_path.ends_with(".rtf"sv) || lower_path.ends_with(".txt"sv) || lower_path.ends_with(".md"sv)) {
        return OfficeTarget {
            .executable = "/bin/TextEditor"sv,
            .button_label = "Writer"sv,
            .description = "Text and document editing"sv,
        };
    }

    if (lower_path.ends_with(".xls"sv) || lower_path.ends_with(".xlsx"sv) || lower_path.ends_with(".ods"sv) || lower_path.ends_with(".csv"sv) || lower_path.ends_with(".sheets"sv)) {
        return OfficeTarget {
            .executable = "/bin/Spreadsheet"sv,
            .button_label = "Spreadsheet"sv,
            .description = "Tables and calculations"sv,
        };
    }

    if (lower_path.ends_with(".ppt"sv) || lower_path.ends_with(".pptx"sv) || lower_path.ends_with(".odp"sv) || lower_path.ends_with(".presenter"sv)) {
        return OfficeTarget {
            .executable = "/bin/Presenter"sv,
            .button_label = "Presentation"sv,
            .description = "Slides and presentations"sv,
        };
    }

    return {};
}

static void open_target(GUI::Window* window, StringView executable, Optional<StringView> file)
{
    if (file.has_value())
        GUI::Process::spawn_or_show_error(window, executable, Array { file.value() });
    else
        GUI::Process::spawn_or_show_error(window, executable, {});
}

static void add_launch_button(GUI::Widget& parent, GUI::Window& window, StringView title, StringView executable, Optional<ByteString> const& real_path = {})
{
    auto& button = parent.add<GUI::Button>(MUST(String::from_utf8(title)));
    button.set_fixed_height(30);
    button.on_click = [&window, executable, real_path](auto) {
        if (real_path.has_value())
            open_target(window.ptr(), executable, real_path.value().bytes_as_string_view());
        else
            open_target(window.ptr(), executable, {});
        GUI::Application::the()->quit();
    };
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath unix proc exec"));

    ByteString path;
    Core::ArgsParser args_parser;
    args_parser.add_positional_argument(path, "Optional office file to open", "path", Core::ArgsParser::Required::No);
    args_parser.parse(arguments);

    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath proc exec"));
    TRY(Core::System::unveil("/bin/TextEditor", "x"));
    TRY(Core::System::unveil("/bin/Spreadsheet", "x"));
    TRY(Core::System::unveil("/bin/Presenter", "x"));
    TRY(Core::System::unveil("/bin/CodeIt", "x"));
    TRY(Core::System::unveil("/usr/local/bin/python3", "x"));
    TRY(Core::System::unveil("/bin/Terminal", "x"));
    TRY(Core::System::unveil("/bin/FileManager", "x"));
    TRY(Core::System::unveil("/bin/Browser", "x"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    Optional<ByteString> real_path;
    if (!path.is_empty()) {
        if (auto maybe_real_path = FileSystem::real_path(path); !maybe_real_path.is_error())
            real_path = maybe_real_path.release_value();
        else
            real_path = path;

        if (auto target = target_for_path(real_path.value()); target.has_value()) {
            open_target(nullptr, target->executable, real_path.value().bytes_as_string_view());
            return 0;
        }
    }

    auto window = GUI::Window::construct();
    window->set_title("MixOffice");
    window->resize(540, 380);
    window->set_resizable(false);
    auto app_icon = GUI::Icon::default_icon("app-spreadsheet"sv);
    window->set_icon(app_icon.bitmap_for_size(16));

    auto main_widget = window->set_main_widget<GUI::Widget>();
    main_widget->set_fill_with_background_color(true);
    main_widget->set_layout<GUI::VerticalBoxLayout>(8, 10);

    auto& title_label = main_widget->add<GUI::Label>("MixOffice\n[Writer | Spreadsheet | Presentation | Developer Tools]"_string);
    title_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);
    title_label.set_fixed_height(36);

    if (real_path.has_value()) {
        auto filename = LexicalPath(real_path.value()).basename();
        auto& notice = main_widget->add<GUI::Label>(MUST(String::formatted("No direct mapping for '{}'. Choose an app below.", filename)));
        notice.set_text_wrapping(Gfx::TextWrapping::Wrap);
    }

    auto& office_label = main_widget->add<GUI::Label>("Office Apps"_string);
    office_label.set_fixed_height(18);
    office_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    add_launch_button(*main_widget, *window, "Writer — text and document editing"sv, "/bin/TextEditor"sv, real_path);
    add_launch_button(*main_widget, *window, "Spreadsheet — tables and calculations"sv, "/bin/Spreadsheet"sv, real_path);
    add_launch_button(*main_widget, *window, "Presentation — slides and decks"sv, "/bin/Presenter"sv, real_path);

    auto& productivity_label = main_widget->add<GUI::Label>("Preinstalled Productivity Apps"_string);
    productivity_label.set_fixed_height(18);
    productivity_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    add_launch_button(*main_widget, *window, "CodeIt — integrated developer editor"sv, "/bin/CodeIt"sv);
    add_launch_button(*main_widget, *window, "Python 3 REPL — real CPython shell"sv, "/usr/local/bin/python3"sv);
    add_launch_button(*main_widget, *window, "Terminal — command line tools"sv, "/bin/Terminal"sv);
    add_launch_button(*main_widget, *window, "FileManager — files and folders"sv, "/bin/FileManager"sv);
    add_launch_button(*main_widget, *window, "Browser — online docs and collaboration"sv, "/bin/Browser"sv);

    auto& close_button = main_widget->add<GUI::Button>("Close"_string);
    close_button.set_fixed_height(28);
    close_button.on_click = [](auto) {
        GUI::Application::the()->quit();
    };

    window->show();
    return app->exec();
}
