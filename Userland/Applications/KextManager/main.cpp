/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Array.h>
#include <AK/LexicalPath.h>
#include <LibCore/DirIterator.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <LibGUI/Application.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/Label.h>
#include <LibGUI/MessageBox.h>
#include <LibGUI/Process.h>
#include <LibGUI/TextEditor.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

static ErrorOr<ByteString> read_bundle_id(ByteString const& kext_path)
{
    auto plist_path = LexicalPath::join(kext_path, "Contents", "Info.plist").string();
    auto file = TRY(Core::File::open(plist_path, Core::File::OpenMode::Read));
    auto contents = TRY(file->read_until_eof());
    auto text = StringView(contents);

    auto key_pos = text.find("CFBundleIdentifier"sv);
    if (!key_pos.has_value())
        return ByteString("(missing CFBundleIdentifier)");

    auto key_tail = text.substring_view(key_pos.value());
    auto string_tag_start = key_tail.find("<string>"sv);
    auto string_tag_end = key_tail.find("</string>"sv);
    if (!string_tag_start.has_value() || !string_tag_end.has_value() || string_tag_end.value() <= string_tag_start.value() + 8)
        return ByteString("(unable to parse identifier)");

    auto value = key_tail.substring_view(string_tag_start.value() + 8, string_tag_end.value() - string_tag_start.value() - 8).trim_whitespace();
    return value.to_byte_string();
}

static ErrorOr<ByteString> read_plist_value(ByteString const& kext_path, StringView key)
{
    auto plist_path = LexicalPath::join(kext_path, "Contents", "Info.plist").string();
    auto file = TRY(Core::File::open(plist_path, Core::File::OpenMode::Read));
    auto contents = TRY(file->read_until_eof());
    auto text = StringView(contents);

    auto key_pos = text.find(key);
    if (!key_pos.has_value())
        return ByteString("(missing key)");

    auto key_tail = text.substring_view(key_pos.value());
    auto string_tag_start = key_tail.find("<string>"sv);
    auto string_tag_end = key_tail.find("</string>"sv);
    if (!string_tag_start.has_value() || !string_tag_end.has_value() || string_tag_end.value() <= string_tag_start.value() + 8)
        return ByteString("(unable to parse)");

    auto value = key_tail.substring_view(string_tag_start.value() + 8, string_tag_end.value() - string_tag_start.value() - 8).trim_whitespace();
    return value.to_byte_string();
}

static ErrorOr<ByteString> inspect_macho_header(ByteString const& kext_path)
{
    auto executable_name = TRY(read_plist_value(kext_path, "CFBundleExecutable"sv));
    if (executable_name.is_empty() || executable_name.starts_with('('))
        return ByteString("Mach-O: executable not declared");

    auto binary_path = LexicalPath::join(kext_path, "Contents", "MacOS", executable_name).string();
    auto file = TRY(Core::File::open(binary_path, Core::File::OpenMode::Read));
    auto bytes = TRY(file->read_some(4));
    if (bytes.size() < 4)
        return ByteString("Mach-O: binary too small");

    u32 magic = (static_cast<u32>(bytes[0]) << 24) | (static_cast<u32>(bytes[1]) << 16) | (static_cast<u32>(bytes[2]) << 8) | static_cast<u32>(bytes[3]);
    if (magic == 0xcffaedfe)
        return ByteString("Mach-O: 64-bit (little-endian)");
    if (magic == 0xfeedfacf)
        return ByteString("Mach-O: 64-bit (big-endian)");
    if (magic == 0xcafebabe)
        return ByteString("Mach-O: universal/fat binary");
    return ByteString::formatted("Mach-O: unknown magic {:#08x}", magic);
}

static ErrorOr<ByteString> scan_driver_kext_folder()
{
    StringBuilder builder;
    size_t count = 0;

    if (!FileSystem::exists("/driverkext"sv)) {
        builder.append("/driverkext does not exist.\n");
        return builder.to_byte_string();
    }

    Core::DirIterator iterator("/driverkext"sv, Core::DirIterator::Flags::SkipDots);
    while (iterator.has_next()) {
        auto name = iterator.next_path();
        if (!name.ends_with(".kext"sv, CaseSensitivity::CaseInsensitive))
            continue;

        auto path = LexicalPath::join("/driverkext"sv, name).string();
        auto bundle_id = read_bundle_id(path);
        auto macho_info = inspect_macho_header(path);
        if (bundle_id.is_error())
            builder.appendff("{}  ->  {} | {}\n", path, "(Info.plist not readable)", macho_info.is_error() ? "(Mach-O unreadable)"sv : macho_info.value());
        else
            builder.appendff("{}  ->  {} | {}\n", path, bundle_id.value(), macho_info.is_error() ? "(Mach-O unreadable)"sv : macho_info.value());
        ++count;
    }

    if (count == 0)
        builder.append("No .kext bundles found in /driverkext\n");
    else
        builder.appendff("\nFound {} kext bundle(s).\n", count);

    return builder.to_byte_string();
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath unix proc exec"));
    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath proc exec"));
    TRY(Core::System::unveil("/driverkext", "r"));
    TRY(Core::System::unveil("/bin/FileManager", "x"));
    TRY(Core::System::unveil("/bin/CompatibilityAssistant", "x"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto window = GUI::Window::construct();
    window->set_title("KextManager");
    window->resize(720, 420);

    auto& main_widget = window->set_main_widget<GUI::Widget>();
    main_widget.set_fill_with_background_color(true);
    main_widget.set_layout<GUI::VerticalBoxLayout>(8, 8);

    auto& header = main_widget.add<GUI::Label>("Drop .kext bundles into /driverkext and click Rescan."_string);
    header.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    auto& output = main_widget.add<GUI::TextEditor>();
    output.set_mode(GUI::TextEditor::Mode::ReadOnly);
    output.set_should_hide_unnecessary_scrollbars(true);

    auto refresh_output = [&]() {
        auto scan_result = scan_driver_kext_folder();
        if (scan_result.is_error())
            output.set_text(MUST(String::formatted("Scan failed: {}", scan_result.error())));
        else
            output.set_text(MUST(String::from_byte_string(scan_result.release_value())));
    };

    auto& button_row = main_widget.add<GUI::Widget>();
    button_row.set_layout<GUI::HorizontalBoxLayout>(GUI::Margins {}, 6);

    auto& rescan_button = button_row.add<GUI::Button>("Rescan"_string);
    rescan_button.on_click = [&](auto) {
        refresh_output();
    };

    auto& open_folder_button = button_row.add<GUI::Button>("Open /driverkext"_string);
    open_folder_button.on_click = [&](auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/FileManager"sv, Array { "/driverkext"sv });
    };

    auto& inspect_button = button_row.add<GUI::Button>("Inspect with CompatibilityAssistant"_string);
    inspect_button.on_click = [&](auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/CompatibilityAssistant"sv, Array { "/driverkext"sv });
    };

    refresh_output();
    window->show();
    return app->exec();
}
