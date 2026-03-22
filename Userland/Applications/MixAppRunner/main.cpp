/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/LexicalPath.h>
#include <AK/StringBuilder.h>
#include <AK/Try.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/ConfigFile.h>
#include <LibCore/Process.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <LibGUI/Application.h>
#include <LibGUI/MessageBox.h>
#include <LibMain/Main.h>

static ByteString resolve_manifest_path(ByteString const& package_root, ByteString const& path)
{
    if (path.is_empty())
        return package_root;

    LexicalPath lexical_path(path);
    if (lexical_path.is_absolute())
        return path;
    return LexicalPath::join(package_root, path).string();
}

static ErrorOr<ByteString> shell_quote(StringView text)
{
    StringBuilder builder;
    TRY(builder.try_append('\''));
    for (auto ch : text) {
        if (ch == '\'')
            TRY(builder.try_append("'\\''"sv));
        else
            TRY(builder.try_append(ch));
    }
    TRY(builder.try_append('\''));
    return builder.to_byte_string();
}

static ErrorOr<ByteString> build_terminal_command(ByteString const& executable, Vector<ByteString> const& arguments, ByteString const& working_directory, bool requires_root)
{
    StringBuilder builder;
    TRY(builder.try_append("cd "sv));
    TRY(builder.try_append(TRY(shell_quote(working_directory))));
    TRY(builder.try_append(" && "sv));
    if (requires_root)
        TRY(builder.try_append("/bin/pls "sv));
    TRY(builder.try_append(TRY(shell_quote(executable))));
    for (auto const& argument : arguments) {
        TRY(builder.try_append(' '));
        TRY(builder.try_append(TRY(shell_quote(argument))));
    }
    return builder.to_byte_string();
}

static Vector<ByteString> read_mixapp_arguments(Core::ConfigFile const& config)
{
    Vector<ByteString> arguments;

    auto inline_arguments = config.read_entry("App", "Arguments").trim_whitespace().split(' ', SplitBehavior::SkipEmpty);
    for (auto const& argument : inline_arguments)
        arguments.append(argument);

    for (size_t index = 1;; ++index) {
        auto key = ByteString::formatted("Argument{}", index);
        auto value = config.read_entry("App", key).trim_whitespace();
        if (value.is_empty())
            break;
        arguments.append(value);
    }

    return arguments;
}

static ErrorOr<void> launch_mixapp(ByteString const& mixapp_path)
{
    auto config = TRY(Core::ConfigFile::open(mixapp_path));

    auto executable = config->read_entry("App", "Executable").trim_whitespace();
    if (config->read_entry("App", "Name").trim_whitespace().is_empty() || executable.is_empty())
        return Error::from_string_literal("Invalid .mixapp file: missing [App] Name or Executable");

    auto package_root = LexicalPath(mixapp_path).dirname();
    auto resolved_executable = resolve_manifest_path(package_root, executable);
    auto working_directory = resolve_manifest_path(package_root, config->read_entry("App", "WorkingDirectory").trim_whitespace());
    auto arguments = read_mixapp_arguments(*config);
    bool run_in_terminal = config->read_bool_entry("App", "RunInTerminal", false);
    bool requires_root = config->read_bool_entry("App", "RequiresRoot", false);

    if (!FileSystem::exists(resolved_executable))
        return Error::from_string_literal("The executable referenced by this .mixapp file does not exist");

    if (run_in_terminal) {
        auto command = TRY(build_terminal_command(resolved_executable, arguments, working_directory, requires_root));
        Vector<StringView, 2> terminal_arguments;
        terminal_arguments.append("-e"sv);
        terminal_arguments.append(command);
        TRY(Core::Process::spawn("/bin/Terminal"sv, terminal_arguments.span(), package_root));
        return {};
    }

    Vector<StringView> argument_views;
    argument_views.ensure_capacity(arguments.size() + 1);
    if (requires_root)
        argument_views.append(resolved_executable);
    for (auto const& argument : arguments)
        argument_views.append(argument);

    if (requires_root)
        TRY(Core::Process::spawn("/bin/Escalator"sv, argument_views.span(), working_directory));
    else
        TRY(Core::Process::spawn(resolved_executable, argument_views.span(), working_directory));

    return {};
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath unix proc exec"));

    ByteString mixapp_path;
    Core::ArgsParser args_parser;
    args_parser.set_general_help("Launch a portable MixUpOS .mixapp package.");
    args_parser.add_positional_argument(mixapp_path, "Path to the .mixapp file", "path", Core::ArgsParser::Required::Yes);
    args_parser.parse(arguments);

    if (auto path = FileSystem::real_path(mixapp_path); !path.is_error())
        mixapp_path = path.release_value();

    [[maybe_unused]] auto app = TRY(GUI::Application::create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd cpath rpath proc exec"));
    TRY(Core::System::unveil("/bin", "x"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(LexicalPath(mixapp_path).dirname(), "rx"));
    TRY(Core::System::unveil(nullptr, nullptr));

    if (auto result = launch_mixapp(mixapp_path); result.is_error()) {
        GUI::MessageBox::show_error(nullptr, ByteString::formatted("Failed to launch {}:\n{}", mixapp_path, result.error()));
        return 1;
    }

    return 0;
}
