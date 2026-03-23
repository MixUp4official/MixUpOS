/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/LexicalPath.h>
#include <AK/StringBuilder.h>
#include <AK/Try.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <LibMain/Main.h>

static ByteString default_output_path(ByteString const& input_path)
{
    LexicalPath lexical_path(input_path);
    auto stem = lexical_path.title();
    if (stem.ends_with(".exe"sv, CaseSensitivity::CaseInsensitive))
        stem = stem.substring(0, stem.length() - 4);
    return LexicalPath::join(lexical_path.dirname(), ByteString::formatted("{}.mixapp", stem)).string();
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio rpath cpath wpath"));

    ByteString input_path;
    ByteString output_path;
    bool overwrite = false;

    Core::ArgsParser args_parser;
    args_parser.set_general_help("Create a .mixapp wrapper for a foreign executable such as a Windows .exe file.");
    args_parser.add_option(overwrite, "Overwrite the output file if it already exists", "force", 'f');
    args_parser.add_positional_argument(input_path, "Input executable path", "input");
    args_parser.add_positional_argument(output_path, "Output .mixapp path", "output", Core::ArgsParser::Required::No);
    args_parser.parse(arguments);

    if (auto real_input_path = FileSystem::real_path(input_path); !real_input_path.is_error())
        input_path = real_input_path.release_value();

    if (!FileSystem::exists(input_path))
        return Error::from_string_literal("Input file does not exist");

    if (output_path.is_empty())
        output_path = default_output_path(input_path);
    else if (auto real_output_parent = FileSystem::real_path(LexicalPath(output_path).dirname()); !real_output_parent.is_error())
        output_path = LexicalPath::join(real_output_parent.release_value(), LexicalPath(output_path).basename()).string();

    if (FileSystem::exists(output_path) && !overwrite)
        return Error::from_string_literal("Output file already exists (use -f to overwrite)");

    auto package_root = LexicalPath(output_path).dirname();
    auto relative_input_path = LexicalPath::relative_path(input_path, package_root);
    auto app_name = LexicalPath(input_path).title();
    if (app_name.ends_with(".exe"sv, CaseSensitivity::CaseInsensitive))
        app_name = app_name.substring(0, app_name.length() - 4);

    StringBuilder builder;
    TRY(builder.try_append("[App]\n"sv));
    TRY(builder.try_appendff("Name={}\n", app_name));
    TRY(builder.try_append("Type=CompatibilityWrapper\n"sv));
    TRY(builder.try_append("Executable=/bin/CompatibilityAssistant\n"sv));
    TRY(builder.try_append("WorkingDirectory=.\n"sv));
    TRY(builder.try_append("Argument1="sv));
    TRY(builder.try_append(relative_input_path));
    TRY(builder.try_append("\n"sv));
    TRY(builder.try_append("Description=Generated wrapper for a foreign executable\n"sv));

    auto output_file = TRY(Core::File::open(output_path, Core::File::OpenMode::Write | Core::File::OpenMode::Truncate));
    TRY(output_file->write_until_depleted(builder.string_view().bytes()));

    outln("Created {}", output_path);
    return 0;
}
