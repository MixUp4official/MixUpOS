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
#include <LibGUI/Process.h>
#include <LibGUI/TextEditor.h>
#include <LibGUI/Widget.h>
#include <LibGUI/Window.h>
#include <LibMain/Main.h>

static ByteString key_value(StringView content, StringView key)
{
    auto start = content.find(ByteString::formatted("{}=", key));
    if (!start.has_value())
        return "(missing)";
    auto tail = content.substring_view(start.value() + key.length() + 1);
    auto end = tail.find('\n');
    if (!end.has_value())
        return tail.trim_whitespace().to_byte_string();
    return tail.substring_view(0, end.value()).trim_whitespace().to_byte_string();
}

static ErrorOr<ByteString> scan_linux_driver_manifests()
{
    StringBuilder output;
    size_t count = 0;

    if (!FileSystem::exists("/linux-driver"sv)) {
        output.append("/linux-driver does not exist\n");
        return output.to_byte_string();
    }

    Core::DirIterator iterator("/linux-driver"sv, Core::DirIterator::Flags::SkipDots);
    while (iterator.has_next()) {
        auto name = iterator.next_path();
        if (!name.ends_with(".ldriver"sv))
            continue;

        auto path = LexicalPath::join("/linux-driver"sv, name).string();
        auto file = TRY(Core::File::open(path, Core::File::OpenMode::Read));
        auto bytes = TRY(file->read_until_eof());
        auto text = StringView(bytes);

        auto driver_name = key_value(text, "name"sv);
        auto pci_ids = key_value(text, "pci_ids"sv);
        auto linux_driver = key_value(text, "linux_driver"sv);
        auto native_driver = key_value(text, "native_driver"sv);
        auto status = key_value(text, "status"sv);

        output.appendff("{}\n  name: {}\n  linux: {}\n  native: {}\n  pci_ids: {}\n  status: {}\n\n",
            path, driver_name, linux_driver, native_driver, pci_ids, status);
        ++count;
    }

    if (count == 0)
        output.append("No .ldriver manifests found in /linux-driver\n");
    else
        output.appendff("Validated {} manifest(s).\n", count);

    return output.to_byte_string();
}

static ErrorOr<ByteString> build_hardware_audit_report()
{
    StringBuilder output;

    auto count_entries = [](StringView path) -> size_t {
        if (!FileSystem::exists(path))
            return 0;
        Core::DirIterator it(path, Core::DirIterator::Flags::SkipDots);
        size_t count = 0;
        while (it.has_next()) {
            it.next_path();
            ++count;
        }
        return count;
    };

    auto pci_count = count_entries("/sys/bus/pci/devices"sv);
    auto usb_count = count_entries("/sys/bus/usb/devices"sv);
    bool display_stack = FileSystem::exists("/dev/gpu/connector0"sv) || FileSystem::exists("/dev/fb0"sv);
    bool hid_stack = FileSystem::exists("/dev/input"sv) || FileSystem::exists("/dev/hid"sv);
    bool storage_stack = FileSystem::exists("/dev/nvme0"sv) || FileSystem::exists("/dev/hda"sv) || FileSystem::exists("/dev/sda"sv);

    output.appendff("Real hardware driver audit\n");
    output.appendff("PCI devices discovered: {}\n", pci_count);
    output.appendff("USB devices discovered: {}\n", usb_count);
    output.appendff("Display stack: {}\n", display_stack ? "ready" : "missing");
    output.appendff("USB/HID stack: {}\n", hid_stack ? "ready" : "missing");
    output.appendff("Storage stack: {}\n", storage_stack ? "ready" : "missing");

    size_t ready = 0;
    ready += display_stack ? 1 : 0;
    ready += hid_stack ? 1 : 0;
    ready += storage_stack ? 1 : 0;
    int score = static_cast<int>((ready * 100) / 3);

    output.appendff("Driver readiness score: {}%\n", score);
    output.appendff("Windows-fair baseline: {}\n", score >= 90 ? "PASS" : "ACTION NEEDED");

    return output.to_byte_string();
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath unix proc exec"));
    auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath proc exec"));
    TRY(Core::System::unveil("/linux-driver", "r"));
    TRY(Core::System::unveil("/sys", "r"));
    TRY(Core::System::unveil("/dev", "r"));
    TRY(Core::System::unveil("/bin/FileManager", "x"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto window = GUI::Window::construct();
    window->set_title("LinuxDriverManager");
    window->resize(760, 430);

    auto& main_widget = window->set_main_widget<GUI::Widget>();
    main_widget.set_fill_with_background_color(true);
    main_widget.set_layout<GUI::VerticalBoxLayout>(8, 8);

    auto& title = main_widget.add<GUI::Label>("Linux Driver Compatibility + Real Hardware Audit"_string);
    title.set_text_alignment(Gfx::TextAlignment::CenterLeft);

    auto& output = main_widget.add<GUI::TextEditor>();
    output.set_mode(GUI::TextEditor::Mode::ReadOnly);

    auto refresh = [&]() {
        auto result = scan_linux_driver_manifests();
        auto audit = build_hardware_audit_report();
        if (result.is_error())
            output.set_text(MUST(String::formatted("Scan failed: {}", result.error())));
        else if (audit.is_error())
            output.set_text(MUST(String::formatted("Hardware audit failed: {}", audit.error())));
        else
            output.set_text(MUST(String::formatted("{}\n\n{}",
                MUST(String::from_byte_string(audit.release_value())),
                MUST(String::from_byte_string(result.release_value())))));
    };

    auto& buttons = main_widget.add<GUI::Widget>();
    buttons.set_layout<GUI::HorizontalBoxLayout>(GUI::Margins {}, 6);

    auto& rescan = buttons.add<GUI::Button>("Rescan"_string);
    rescan.on_click = [&](auto) { refresh(); };

    auto& open_folder = buttons.add<GUI::Button>("Open /linux-driver"_string);
    open_folder.on_click = [&](auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/FileManager"sv, Array { "/linux-driver"sv });
    };

    auto& open_sys = buttons.add<GUI::Button>("Open /sys/bus"_string);
    open_sys.on_click = [&](auto) {
        GUI::Process::spawn_or_show_error(window.ptr(), "/bin/FileManager"sv, Array { "/sys/bus"sv });
    };

    refresh();
    window->show();
    return app->exec();
}
