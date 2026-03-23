/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/CharacterTypes.h>
#include <AK/LexicalPath.h>
#include <AK/StringBuilder.h>
#include <AK/Try.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/ConfigFile.h>
#include <LibCore/File.h>
#include <LibCore/Process.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <LibGUI/Application.h>
#include <LibGUI/MessageBox.h>
#include <LibMain/Main.h>
#include <errno.h>
#include <sys/wait.h>

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

static ByteString sanitize_name_for_path(ByteString name)
{
    name = name.trim_whitespace();
    if (name.is_empty())
        return "InstalledMixApp";
    StringBuilder builder;
    for (auto ch : name) {
        if (is_ascii_alphanumeric(ch) || ch == '-' || ch == '_')
            builder.append(ch);
        else
            builder.append('_');
    }
    return builder.to_byte_string();
}

static ErrorOr<void> ensure_directory_exists(ByteString const& path)
{
    if (FileSystem::exists(path))
        return {};
    if (auto result = Core::System::mkdir(path, 0755); result.is_error() && result.error().code() != EEXIST)
        return result.release_error();
    return {};
}

struct InstallationResult {
    ByteString installed_mixapp_path;
    ByteString app_name;
};

static ErrorOr<void> write_desktop_entry(ByteString const& entry_path, ByteString const& app_name, ByteString const& installed_mixapp_path)
{
    StringBuilder builder;
    TRY(builder.try_append("[App]\n"sv));
    TRY(builder.try_appendff("Name={}\n", app_name));
    TRY(builder.try_append("Executable=/bin/MixAppRunner\n"sv));
    TRY(builder.try_appendff("Arguments={}\n", installed_mixapp_path));
    TRY(builder.try_append("Category=Applications\n"sv));
    TRY(builder.try_append("Description=Installed MixApp launcher\n"sv));

    auto file = TRY(Core::File::open(entry_path, Core::File::OpenMode::Write | Core::File::OpenMode::Truncate));
    TRY(file->write_until_depleted(builder.string_view().bytes()));
    return {};
}

static ErrorOr<InstallationResult> install_mixapp_package(ByteString const& mixapp_path)
{
    auto config = TRY(Core::ConfigFile::open(mixapp_path));
    auto app_name = sanitize_name_for_path(config->read_entry("App", "Name"));
    auto source_directory = LexicalPath(mixapp_path).dirname();

    auto applications_root = "/home/anon/Applications"sv;
    auto install_root = "/home/anon/Applications/MixApps"sv;
    TRY(ensure_directory_exists(applications_root));
    TRY(ensure_directory_exists(install_root));

    auto destination_directory = LexicalPath::join(install_root, app_name).string();
    TRY(FileSystem::copy_file_or_directory(destination_directory, source_directory, FileSystem::RecursionMode::Allowed, FileSystem::LinkMode::Disallowed, FileSystem::AddDuplicateFileMarker::No));
    auto installed_mixapp_path = LexicalPath::join(destination_directory, LexicalPath(mixapp_path).basename()).string();

    auto desktop_root = "/home/anon/Desktop"sv;
    TRY(ensure_directory_exists(desktop_root));
    auto desktop_entry_name = ByteString::formatted("{}.af", app_name);
    TRY(write_desktop_entry(LexicalPath::join(desktop_root, desktop_entry_name).string(), app_name, installed_mixapp_path));
    TRY(write_desktop_entry(LexicalPath::join(destination_directory, desktop_entry_name).string(), app_name, installed_mixapp_path));

    return InstallationResult {
        .installed_mixapp_path = installed_mixapp_path,
        .app_name = app_name,
    };
}

static ErrorOr<ByteString> extract_html_zip_package(ByteString const& package_root, ByteString const& source_zip, ByteString const& app_name, ByteString const& entry_point)
{
    auto extract_root = LexicalPath::join(package_root, ".mixapp-extracted", sanitize_name_for_path(app_name)).string();
    TRY(ensure_directory_exists(LexicalPath::join(package_root, ".mixapp-extracted"sv).string()));
    TRY(ensure_directory_exists(extract_root));

    Vector<ByteString> unzip_arguments;
    unzip_arguments.append("-o"_string);
    unzip_arguments.append(source_zip);
    unzip_arguments.append("-d"_string);
    unzip_arguments.append(extract_root);

    auto unzip_pid = TRY(Core::Process::spawn("/bin/unzip"sv, unzip_arguments.span(), {}, Core::Process::KeepAsChild::Yes));
    auto [_, status] = TRY(Core::System::waitpid(unzip_pid, 0));
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return Error::from_string_literal("Failed to extract HTML zip package with /bin/unzip");

    auto resolved_entry = LexicalPath::join(extract_root, entry_point.is_empty() ? "index.html"sv : entry_point).string();
    if (!FileSystem::exists(resolved_entry))
        return Error::from_string_literal("Extracted HTML zip package is missing its entry point file");
    return resolved_entry;
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
    auto name = config->read_entry("App", "Name").trim_whitespace();
    auto type = config->read_entry("App", "Type").trim_whitespace();
    auto source = config->read_entry("App", "Source").trim_whitespace();
    auto entry_point = config->read_entry("App", "EntryPoint").trim_whitespace();
    if (name.is_empty())
        return Error::from_string_literal("Invalid .mixapp file: missing [App] Name");

    auto package_root = LexicalPath(mixapp_path).dirname();
    bool is_html_zip_package = type.equals_ignoring_ascii_case("HtmlZipPackage"sv);
    if (executable.is_empty()) {
        if (type.equals_ignoring_ascii_case("PythonScript"sv)) {
            executable = "/bin/python3";
            if (source.is_empty())
                return Error::from_string_literal("Invalid .mixapp file: Type=PythonScript requires Source when Executable is omitted");
        } else if (type.equals_ignoring_ascii_case("HtmlDocument"sv)) {
            executable = "/bin/Browser";
            if (source.is_empty())
                return Error::from_string_literal("Invalid .mixapp file: Type=HtmlDocument requires Source when Executable is omitted");
        } else if (type.equals_ignoring_ascii_case("PythonZipPackage"sv)) {
            executable = "/bin/python3";
            if (source.is_empty())
                return Error::from_string_literal("Invalid .mixapp file: Type=PythonZipPackage requires Source when Executable is omitted");
        } else if (type.equals_ignoring_ascii_case("JavaJar"sv)) {
            executable = "/bin/java";
            if (source.is_empty())
                return Error::from_string_literal("Invalid .mixapp file: Type=JavaJar requires Source when Executable is omitted");
        } else if (is_html_zip_package) {
            executable = "/bin/Browser";
            if (source.is_empty())
                return Error::from_string_literal("Invalid .mixapp file: Type=HtmlZipPackage requires Source when Executable is omitted");
        } else {
            return Error::from_string_literal("Invalid .mixapp file: missing [App] Executable");
        }
    }

    auto resolved_executable = resolve_manifest_path(package_root, executable);
    auto working_directory = resolve_manifest_path(package_root, config->read_entry("App", "WorkingDirectory").trim_whitespace());
    auto parsed_arguments = read_mixapp_arguments(*config);
    Vector<ByteString> arguments;
    if (type.equals_ignoring_ascii_case("JavaJar"sv))
        arguments.append("-jar"_string);
    if (!source.is_empty()) {
        auto resolved_source = resolve_manifest_path(package_root, source);
        if (is_html_zip_package)
            arguments.append(TRY(extract_html_zip_package(package_root, resolved_source, name, entry_point)));
        else
            arguments.append(resolved_source);
    }
    arguments.extend(move(parsed_arguments));
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
    TRY(Core::System::unveil("/home/anon/Applications", "crwx"));
    TRY(Core::System::unveil("/res", "r"));
    TRY(Core::System::unveil(LexicalPath(mixapp_path).dirname(), "rx"));
    TRY(Core::System::unveil(nullptr, nullptr));

    auto install_prompt_result = GUI::MessageBox::show(nullptr,
        "Install this .mixapp into /home/anon/Applications/MixApps before launching?\n\nYes = Install + Launch\nNo = Launch once",
        "MixApp Installer"sv,
        GUI::MessageBox::Type::Question,
        GUI::MessageBox::InputType::YesNoCancel);
    if (install_prompt_result == GUI::MessageBox::ExecResult::Cancel)
        return 0;
    if (install_prompt_result == GUI::MessageBox::ExecResult::Yes) {
        auto installed_mixapp_path = install_mixapp_package(mixapp_path);
        if (installed_mixapp_path.is_error()) {
            GUI::MessageBox::show_error(nullptr, ByteString::formatted("Failed to install {}:\n{}", mixapp_path, installed_mixapp_path.error()));
            return 1;
        }
        auto installation = installed_mixapp_path.release_value();
        mixapp_path = installation.installed_mixapp_path;
    }

    if (auto result = launch_mixapp(mixapp_path); result.is_error()) {
        GUI::MessageBox::show_error(nullptr, ByteString::formatted("Failed to launch {}:\n{}", mixapp_path, result.error()));
        return 1;
    }

    return 0;
}
