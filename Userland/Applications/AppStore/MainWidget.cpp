/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "MainWidget.h"
#include <AK/LexicalPath.h>
#include <AK/Optional.h>
#include <AK/QuickSort.h>
#include <AK/Stream.h>
#include <LibCore/File.h>
#include <LibCore/InputBufferedFile.h>
#include <LibCore/Proxy.h>
#include <LibCore/StandardPaths.h>
#include <LibCore/System.h>
#include <LibFileSystem/FileSystem.h>
#include <LibGUI/BoxLayout.h>
#include <LibGUI/Button.h>
#include <LibGUI/Clipboard.h>
#include <LibGUI/FilteringProxyModel.h>
#include <LibGUI/Label.h>
#include <LibGUI/MessageBox.h>
#include <LibGUI/Process.h>
#include <LibGUI/SortingProxyModel.h>
#include <LibGUI/Splitter.h>
#include <LibGUI/Statusbar.h>
#include <LibGUI/TableView.h>
#include <LibGUI/TextBox.h>
#include <LibGUI/TextEditor.h>
#include <LibHTTP/HttpResponse.h>
#include <LibMarkdown/Document.h>
#include <LibMarkdown/Table.h>
#include <LibMarkdown/Visitor.h>
#include <LibProtocol/Request.h>
#include <LibProtocol/RequestClient.h>
#include <LibURL/URL.h>

namespace AppStore {

static constexpr auto system_ports_index_path = "/usr/Ports/AvailablePorts.md"sv;
static constexpr auto installed_ports_database_path = "/usr/Ports/installed.db"sv;
static constexpr auto remote_ports_index_url = "https://raw.githubusercontent.com/SerenityOS/serenity/master/Ports/AvailablePorts.md"sv;

namespace {

class MarkdownTableFinder final : public Markdown::Visitor {
public:
    static MarkdownTableFinder analyze(Markdown::Document const& document)
    {
        MarkdownTableFinder finder;
        document.walk(finder);
        return finder;
    }

    Vector<Markdown::Table const*> const& tables() const { return m_tables; }

private:
    virtual RecursionDecision visit(Markdown::Table const& table) override
    {
        if (m_tables.size() >= 1)
            return RecursionDecision::Break;
        m_tables.append(&table);
        return RecursionDecision::Recurse;
    }

    Vector<Markdown::Table const*> m_tables;
};

Optional<Markdown::Table::Column const&> get_column_in_table(Markdown::Table const& ports_table, StringView column_name)
{
    for (auto const& column : ports_table.columns()) {
        if (column_name == column.header.render_for_terminal())
            return column;
    }
    return {};
}

ErrorOr<String> extract_port_name_from_column(Markdown::Table::Column const& column, size_t row_index)
{
    struct TextNodeFinder final : public Markdown::Visitor {
        virtual RecursionDecision visit(Markdown::Text::LinkNode const& node) override
        {
            text_node = node.text.ptr();
            return RecursionDecision::Break;
        }

        Markdown::Text::Node* text_node { nullptr };
    };

    TextNodeFinder visitor;
    column.rows[row_index].walk(visitor);
    if (!visitor.text_node)
        return Error::from_string_literal("Could not extract package name from ports table");

    StringBuilder builder;
    visitor.text_node->render_for_raw_print(builder);
    return builder.to_string();
}

String build_details_text(PackageInfo const& package)
{
    StringBuilder builder;
    builder.appendff("Latest version: {}\n", package.version);
    builder.appendff("Status: {}\n", package_status_text(package));
    builder.appendff("Website: {}\n", package.website);

    if (package.has_port_directory)
        builder.appendff("Port directory: {}\n", package.port_directory);
    else
        builder.append("Port directory: not present in this system image\n"sv);

    builder.append('\n');
    if (package.has_port_directory) {
        builder.append("Install or update via the existing ports system:\n"sv);
        builder.appendff("cd {} && pls ./package.sh\n", package.port_directory);
    } else {
        builder.append("One-click install is unavailable because this image does not include the corresponding /usr/Ports entry.\n"sv);
        builder.append("If you add the ports tree to /usr/Ports, this button will start working immediately.\n"sv);
    }

    builder.append('\n');
    builder.append("This App Store is currently a native front-end for the ports index and installed.db.\n"sv);
    return MUST(builder.to_string());
}

ErrorOr<String> build_install_command(PackageInfo const& package)
{
    return String::formatted("cd {} && pls ./package.sh", package.port_directory);
}

}

ErrorOr<void> MainWidget::initialize()
{
    set_fill_with_background_color(true);
    set_layout<GUI::VerticalBoxLayout>(6);

    auto& toolbar = add<GUI::Widget>();
    toolbar.set_layout<GUI::HorizontalBoxLayout>(6);
    toolbar.set_fixed_height(32);

    auto& search_label = toolbar.add<GUI::Label>("Search:"_string);
    search_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);
    search_label.set_fixed_width(54);

    m_search_box = toolbar.add<GUI::TextBox>();
    m_search_box->set_placeholder("Find ports by name, site, or status");

    auto& filter_label = toolbar.add<GUI::Label>("Show:"_string);
    filter_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);
    filter_label.set_fixed_width(42);

    m_all_filter_button = toolbar.add<GUI::Button>("All"_string);
    m_installed_filter_button = toolbar.add<GUI::Button>("Installed"_string);
    m_updates_filter_button = toolbar.add<GUI::Button>("Updates"_string);

    m_all_filter_button->set_checkable(true);
    m_all_filter_button->set_button_style(Gfx::ButtonStyle::Coolbar);
    m_installed_filter_button->set_checkable(true);
    m_installed_filter_button->set_button_style(Gfx::ButtonStyle::Coolbar);
    m_updates_filter_button->set_checkable(true);
    m_updates_filter_button->set_button_style(Gfx::ButtonStyle::Coolbar);
    sync_filter_buttons();

    m_reload_button = toolbar.add<GUI::Button>("Reload"_string);
    m_fetch_button = toolbar.add<GUI::Button>("Fetch Latest"_string);
    m_update_all_button = toolbar.add<GUI::Button>("Update All"_string);
    m_reload_button->set_fixed_width(90);
    m_fetch_button->set_fixed_width(110);
    m_update_all_button->set_fixed_width(110);

    auto& splitter = add<GUI::HorizontalSplitter>();
    splitter.set_opportunistic_resizee(GUI::Splitter::OpportunisticResizee::Second);

    auto& list_panel = splitter.add<GUI::Widget>();
    list_panel.set_layout<GUI::VerticalBoxLayout>(4);

    auto& list_label = list_panel.add<GUI::Label>("Available Packages"_string);
    list_label.set_text_alignment(Gfx::TextAlignment::CenterLeft);
    list_label.set_fixed_height(20);

    m_table_view = list_panel.add<GUI::TableView>();
    m_table_view->set_should_hide_unnecessary_scrollbars(true);
    m_table_view->set_selection_behavior(GUI::AbstractView::SelectionBehavior::SelectRows);
    m_table_view->set_selection_mode(GUI::AbstractView::SelectionMode::SingleSelection);
    m_table_view->set_key_column(PackageModel::Column::Name);

    auto& details_panel = splitter.add<GUI::Widget>();
    details_panel.set_layout<GUI::VerticalBoxLayout>(6);

    m_title_label = details_panel.add<GUI::Label>("Select a package"_string);
    m_title_label->set_text_alignment(Gfx::TextAlignment::CenterLeft);
    m_title_label->set_fixed_height(24);

    m_source_label = details_panel.add<GUI::Label>(""_string);
    m_source_label->set_text_alignment(Gfx::TextAlignment::CenterLeft);
    m_source_label->set_fixed_height(20);

    m_details_box = details_panel.add<GUI::TextEditor>();
    m_details_box->set_mode(GUI::TextEditor::Mode::ReadOnly);
    m_details_box->set_wrapping_mode(GUI::TextEditor::WrappingMode::WrapAtWords);
    m_details_box->set_gutter_visible(false);
    m_details_box->set_ruler_visible(false);
    m_details_box->set_cursor_line_highlighting(false);
    m_details_box->set_text("Pick a package on the left to inspect it.\n\nThe install button uses the existing ports system and opens a terminal session for the selected package.");

    auto& button_row = details_panel.add<GUI::Widget>();
    button_row.set_layout<GUI::HorizontalBoxLayout>(6);
    button_row.set_fixed_height(32);

    m_install_button = button_row.add<GUI::Button>("Install"_string);
    m_copy_command_button = button_row.add<GUI::Button>("Copy Command"_string);
    m_website_button = button_row.add<GUI::Button>("Open Website"_string);
    m_open_folder_button = button_row.add<GUI::Button>("Open Port Folder"_string);

    m_statusbar = add<GUI::Statusbar>();

    m_package_model = PackageModel::create();
    m_filtering_model = TRY(GUI::FilteringProxyModel::create(*m_package_model, GUI::FilteringProxyModel::FilteringOptions::SortByScore));
    m_sorting_model = TRY(GUI::SortingProxyModel::create(*m_filtering_model));
    m_sorting_model->sort(PackageModel::Column::Name, GUI::SortOrder::Ascending);

    m_table_view->set_model(m_sorting_model);
    m_table_view->set_column_width(PackageModel::Column::Name, 180);
    m_table_view->set_column_width(PackageModel::Column::Version, 90);
    m_table_view->set_column_width(PackageModel::Column::Status, 220);

    m_search_box->on_change = [this] {
        m_filtering_model->set_filter_term(m_search_box->text());
        if (!m_table_view->selection().first().is_valid() && m_sorting_model->row_count() > 0)
            m_table_view->set_cursor(m_sorting_model->index(0, 0, {}), GUI::AbstractView::SelectionUpdate::Set);
        update_details();
    };

    m_table_view->on_selection_change = [this] {
        update_details();
    };

    m_table_view->on_activation = [this](auto const&) {
        install_selected_package();
    };

    m_all_filter_button->on_click = [this](auto) {
        set_filter_mode(PackageModel::FilterMode::All);
    };

    m_installed_filter_button->on_click = [this](auto) {
        set_filter_mode(PackageModel::FilterMode::Installed);
    };

    m_updates_filter_button->on_click = [this](auto) {
        set_filter_mode(PackageModel::FilterMode::Updates);
    };

    m_reload_button->on_click = [this](auto) {
        reload_packages();
    };

    m_fetch_button->on_click = [this](auto) {
        fetch_latest_packages();
    };

    m_update_all_button->on_click = [this](auto) {
        update_all_packages();
    };

    m_install_button->on_click = [this](auto) {
        install_selected_package();
    };

    m_copy_command_button->on_click = [this](auto) {
        copy_selected_install_command();
    };

    m_website_button->on_click = [this](auto) {
        open_selected_website();
    };

    m_open_folder_button->on_click = [this](auto) {
        open_selected_port_directory();
    };

    return load_packages_from_best_source(true);
}

void MainWidget::set_filter_mode(PackageModel::FilterMode filter_mode)
{
    m_package_model->set_filter_mode(filter_mode);
    sync_filter_buttons();
    m_sorting_model->sort(PackageModel::Column::Name, GUI::SortOrder::Ascending);
    if (m_sorting_model->row_count() > 0)
        m_table_view->set_cursor(m_sorting_model->index(0, 0, {}), GUI::AbstractView::SelectionUpdate::Set);
    else
        m_table_view->clear_selection();
    update_details();
}

void MainWidget::reload_packages()
{
    if (auto result = load_packages_from_best_source(false); result.is_error())
        show_error("Reload failed"sv, MUST(result.error().to_string()).bytes_as_string_view());
}

void MainWidget::fetch_latest_packages()
{
    if (m_active_request)
        return;

    if (auto result = ensure_cache_directory_exists(); result.is_error()) {
        show_error("App Store"sv, MUST(result.error().to_string()).bytes_as_string_view());
        return;
    }

    auto target_path = cache_file_path();
    auto output_file_or_error = Core::File::open(target_path, Core::File::OpenMode::Write | Core::File::OpenMode::Truncate, 0644);
    if (output_file_or_error.is_error()) {
        show_error("App Store"sv, MUST(output_file_or_error.error().to_string()).bytes_as_string_view());
        return;
    }

    if (!m_request_client) {
        auto request_client_or_error = Protocol::RequestClient::try_create();
        if (request_client_or_error.is_error()) {
            show_error("App Store"sv, MUST(request_client_or_error.error().to_string()).bytes_as_string_view());
            return;
        }
        m_request_client = request_client_or_error.release_value();
    }

    m_fetch_output_file = output_file_or_error.release_value();

    HTTP::HeaderMap request_headers;
    Core::ProxyData proxy_data {};
    auto url = URL::URL(remote_ports_index_url);
    m_active_request = m_request_client->start_request("GET", url, request_headers, ReadonlyBytes {}, proxy_data);

    update_status_text("Fetching latest package index..."_string);
    m_fetch_button->set_enabled(false);

    auto on_data_received = [this](ReadonlyBytes data) {
        if (m_fetch_output_file)
            m_fetch_output_file->write_until_depleted(data).release_value_but_fixme_should_propagate_errors();
    };

    auto on_finished = [this, target_path = move(target_path)](bool success, auto) {
        m_active_request = nullptr;
        m_fetch_output_file = nullptr;
        m_fetch_button->set_enabled(true);

        if (!success) {
            update_status_text("Fetching latest package index failed."_string);
            show_error("App Store"sv, "Could not download the ports index.");
            return;
        }

        if (auto result = load_packages_from_path(target_path, "Cached online index"sv); result.is_error()) {
            show_error("App Store"sv, MUST(result.error().to_string()).bytes_as_string_view());
            return;
        }
        update_status_text("Latest package index downloaded."_string);
    };

    m_active_request->set_unbuffered_request_callbacks({}, move(on_data_received), move(on_finished));
}

ErrorOr<void> MainWidget::load_packages_from_best_source(bool allow_network_fetch_if_missing)
{
    if (FileSystem::exists(system_ports_index_path))
        return load_packages_from_path(system_ports_index_path, "System ports index"sv);

    auto cache_path = cache_file_path();
    if (FileSystem::exists(cache_path))
        return load_packages_from_path(cache_path, "Cached online index"sv);

    m_package_model->set_packages({});
    update_details();
    update_status_text("No package index was found locally."_string);

    if (allow_network_fetch_if_missing)
        fetch_latest_packages();

    return {};
}

ErrorOr<void> MainWidget::load_packages_from_path(StringView path, StringView source_name)
{
    auto selected_name = selected_package() ? selected_package()->name : String {};
    auto packages = TRY(read_packages_from_markdown(path));
    TRY(apply_installed_status(packages));
    quick_sort(packages, [](auto const& a, auto const& b) {
        return a.name < b.name;
    });

    m_last_source_name = TRY(String::from_utf8(source_name));
    m_last_source_path = TRY(String::from_utf8(path));

    m_package_model->set_packages(move(packages));
    if (m_search_box)
        m_filtering_model->set_filter_term(m_search_box->text());
    else
        m_filtering_model->set_filter_term({});
    m_sorting_model->sort(PackageModel::Column::Name, GUI::SortOrder::Ascending);

    if (!selected_name.is_empty())
        select_package_by_name(selected_name);

    if (!m_table_view->selection().first().is_valid() && m_sorting_model->row_count() > 0)
        m_table_view->set_cursor(m_sorting_model->index(0, 0, {}), GUI::AbstractView::SelectionUpdate::Set);

    update_details();
    return {};
}

ErrorOr<Vector<PackageInfo>> MainWidget::read_packages_from_markdown(StringView path)
{
    auto available_ports_file = TRY(Core::File::open(path, Core::File::OpenMode::Read));
    auto content_buffer = TRY(available_ports_file->read_until_eof());
    auto document = Markdown::Document::parse(StringView(content_buffer));
    auto finder = MarkdownTableFinder::analyze(*document);
    if (finder.tables().size() != 1)
        return Error::from_string_literal("Invalid package index: expected one Markdown table");

    auto const& table = *finder.tables().first();
    auto maybe_port_name_column = get_column_in_table(table, "Port"sv);
    auto maybe_port_version_column = get_column_in_table(table, "Version"sv);
    auto maybe_port_website_column = get_column_in_table(table, "Website"sv);

    if (!maybe_port_name_column.has_value() || !maybe_port_version_column.has_value() || !maybe_port_website_column.has_value())
        return Error::from_string_literal("The package index is missing required columns");

    auto const& port_name_column = maybe_port_name_column.release_value();
    auto const& port_version_column = maybe_port_version_column.release_value();
    auto const& port_website_column = maybe_port_website_column.release_value();

    Vector<PackageInfo> packages;
    TRY(packages.try_ensure_capacity(port_name_column.rows.size()));

    for (size_t i = 0; i < port_name_column.rows.size(); ++i) {
        auto name = TRY(extract_port_name_from_column(port_name_column, i));
        auto version = TRY(String::from_byte_string(port_version_column.rows[i].render_for_terminal()));
        auto website = TRY(String::from_byte_string(port_website_column.rows[i].render_for_terminal()));

        if (version.is_empty())
            version = "n/a"_string;
        if (website.is_empty())
            website = "n/a"_string;

        PackageInfo info;
        info.name = move(name);
        info.version = move(version);
        info.website = move(website);
        info.port_directory = MUST(String::formatted("/usr/Ports/{}", info.name));
        auto package_script_path = MUST(String::formatted("{}/package.sh", info.port_directory));
        info.has_port_directory = FileSystem::is_directory(info.port_directory.bytes_as_string_view())
            && FileSystem::exists(package_script_path.bytes_as_string_view());

        TRY(packages.try_append(move(info)));
    }

    return packages;
}

ErrorOr<void> MainWidget::apply_installed_status(Vector<PackageInfo>& packages)
{
    HashMap<String, String> installed_versions;

    if (FileSystem::exists(installed_ports_database_path)) {
        auto installed_db_file = TRY(Core::File::open(installed_ports_database_path, Core::File::OpenMode::Read));
        auto buffered_file = TRY(Core::InputBufferedFile::create(move(installed_db_file)));
        auto buffer = TRY(ByteBuffer::create_uninitialized(PAGE_SIZE));

        while (TRY(buffered_file->can_read_line())) {
            auto line = TRY(buffered_file->read_line(buffer));
            if (line.is_empty())
                continue;

            auto parts = line.split_view(' ');
            if (parts.size() < 3)
                continue;

            if (parts[0] != "auto"sv && parts[0] != "manual"sv)
                continue;

            auto name = TRY(String::from_utf8(parts[1]));
            auto version = TRY(String::from_utf8(parts[2]));
            TRY(installed_versions.try_set(move(name), move(version)));
        }
    }

    for (auto& package : packages) {
        auto it = installed_versions.find(package.name);
        if (it == installed_versions.end())
            continue;

        package.is_installed = true;
        package.installed_version = it->value;
        package.has_update = package.installed_version != package.version;
    }

    return {};
}

ErrorOr<void> MainWidget::ensure_cache_directory_exists()
{
    auto data_directory = Core::StandardPaths::data_directory();
    auto directory = TRY(cache_directory());

    if (!FileSystem::exists(data_directory))
        TRY(Core::System::mkdir(data_directory, 0755));
    if (!FileSystem::exists(directory))
        TRY(Core::System::mkdir(directory, 0755));

    return {};
}

ErrorOr<ByteString> MainWidget::cache_directory() const
{
    return ByteString::formatted("{}/AppStore", Core::StandardPaths::data_directory());
}

ByteString MainWidget::cache_file_path() const
{
    return ByteString::formatted("{}/AppStore/AvailablePorts.md", Core::StandardPaths::data_directory());
}

PackageInfo const* MainWidget::selected_package() const
{
    if (!m_table_view)
        return nullptr;

    auto selected_index = m_table_view->selection().first();
    if (!selected_index.is_valid())
        return nullptr;

    auto filtered_index = m_sorting_model->map_to_source(selected_index);
    if (!filtered_index.is_valid())
        return nullptr;

    auto source_index = m_filtering_model->map(filtered_index);
    if (!source_index.is_valid())
        return nullptr;

    return static_cast<PackageInfo const*>(source_index.internal_data());
}

size_t MainWidget::available_update_count() const
{
    if (!m_package_model)
        return 0;

    size_t updates = 0;
    for (int row = 0; row < m_package_model->row_count(); ++row) {
        auto const* package = m_package_model->package_at(row);
        if (!package)
            continue;
        if (package->has_update && package->has_port_directory)
            ++updates;
    }
    return updates;
}

ErrorOr<String> MainWidget::build_update_all_command() const
{
    StringBuilder builder;
    bool first_command = true;

    for (int row = 0; row < m_package_model->row_count(); ++row) {
        auto const* package = m_package_model->package_at(row);
        if (!package || !package->has_update || !package->has_port_directory)
            continue;

        if (!first_command)
            TRY(builder.try_append(" && "sv));
        TRY(builder.try_appendff("cd {} && pls ./package.sh", package->port_directory));
        first_command = false;
    }

    if (first_command)
        return Error::from_string_literal("No installable updates are available");

    return builder.to_string();
}

void MainWidget::update_details()
{
    auto const* package = selected_package();
    if (!package) {
        m_title_label->set_text("Select a package"_string);
        if (m_last_source_path.is_empty())
            m_source_label->set_text("No package index loaded"_string);
        else
            m_source_label->set_text(MUST(String::formatted("{}: {}", m_last_source_name, m_last_source_path)));
        m_details_box->set_text("Pick a package on the left to inspect it.");
        update_actions();
        update_status_text();
        return;
    }

    m_title_label->set_text(package->name);
    m_source_label->set_text(MUST(String::formatted("{}: {}", m_last_source_name, m_last_source_path)));
    m_details_box->set_text(build_details_text(*package));
    update_actions();
    update_status_text();
}

void MainWidget::update_status_text(Optional<String> override)
{
    if (override.has_value()) {
        m_statusbar->set_text(override.release_value());
        return;
    }

    auto visible_packages = m_sorting_model ? m_sorting_model->row_count() : 0;
    auto updates = available_update_count();
    StringView filter_name = "all packages"sv;
    switch (m_package_model->filter_mode()) {
    case PackageModel::FilterMode::All:
        filter_name = "all packages"sv;
        break;
    case PackageModel::FilterMode::Installed:
        filter_name = "installed packages"sv;
        break;
    case PackageModel::FilterMode::Updates:
        filter_name = "updates"sv;
        break;
    }
    auto const* package = selected_package();
    if (!package) {
        m_statusbar->set_text(MUST(String::formatted("{} {} visible, {} updates available", visible_packages, filter_name, updates)));
        return;
    }

    m_statusbar->set_text(MUST(String::formatted("{} {} visible, {} updates available, selected: {}", visible_packages, filter_name, updates, package_status_text(*package))));
}

void MainWidget::select_package_by_name(StringView name)
{
    if (!m_sorting_model || name.is_empty())
        return;

    for (int row = 0; row < m_sorting_model->row_count(); ++row) {
        auto index = m_sorting_model->index(row, PackageModel::Column::Name, {});
        if (!index.is_valid())
            continue;
        if (index.data().to_string() == name) {
            m_table_view->set_cursor(index, GUI::AbstractView::SelectionUpdate::Set);
            return;
        }
    }
}

void MainWidget::update_actions()
{
    auto const* package = selected_package();
    auto has_package = package != nullptr;
    auto has_updates = available_update_count() > 0;

    m_update_all_button->set_enabled(has_updates);
    m_install_button->set_enabled(has_package && package->has_port_directory);
    m_copy_command_button->set_enabled(has_package && package->has_port_directory);
    m_website_button->set_enabled(has_package && package->website != "n/a"sv);
    m_open_folder_button->set_enabled(has_package && package->has_port_directory);

    if (!package) {
        m_install_button->set_text("Install"_string);
        return;
    }

    if (!package->is_installed)
        m_install_button->set_text("Install"_string);
    else if (package->has_update)
        m_install_button->set_text("Update"_string);
    else
        m_install_button->set_text("Reinstall"_string);
}

void MainWidget::sync_filter_buttons()
{
    auto filter_mode = m_package_model ? m_package_model->filter_mode() : PackageModel::FilterMode::All;
    if (m_all_filter_button)
        m_all_filter_button->set_checked(filter_mode == PackageModel::FilterMode::All);
    if (m_installed_filter_button)
        m_installed_filter_button->set_checked(filter_mode == PackageModel::FilterMode::Installed);
    if (m_updates_filter_button)
        m_updates_filter_button->set_checked(filter_mode == PackageModel::FilterMode::Updates);
}

void MainWidget::open_selected_website()
{
    auto const* package = selected_package();
    if (!package || package->website == "n/a"sv)
        return;

    GUI::Process::spawn_or_show_error(window(), "/bin/Browser"sv, Array { package->website.bytes_as_string_view() });
}

void MainWidget::open_selected_port_directory()
{
    auto const* package = selected_package();
    if (!package || !package->has_port_directory)
        return;

    GUI::Process::spawn_or_show_error(window(), "/bin/FileManager"sv, Array { package->port_directory.bytes_as_string_view() });
}

void MainWidget::install_selected_package()
{
    auto const* package = selected_package();
    if (!package)
        return;

    if (!package->has_port_directory) {
        show_error("App Store"sv, "This package does not have a usable /usr/Ports entry in the current image.");
        return;
    }

    auto command = MUST(build_install_command(*package));
    GUI::Process::spawn_or_show_error(window(), "/bin/Terminal"sv, Array { "-k"sv, "-e"sv, command.bytes_as_string_view() });
}

void MainWidget::update_all_packages()
{
    auto command = build_update_all_command();
    if (command.is_error()) {
        show_error("App Store"sv, MUST(command.error().to_string()).bytes_as_string_view());
        return;
    }

    GUI::Process::spawn_or_show_error(window(), "/bin/Terminal"sv, Array { "-k"sv, "-e"sv, command.value().bytes_as_string_view() });
}

void MainWidget::copy_selected_install_command()
{
    auto const* package = selected_package();
    if (!package || !package->has_port_directory)
        return;

    auto command = MUST(build_install_command(*package));
    GUI::Clipboard::the().set_plain_text(command);
    update_status_text(MUST(String::formatted("Copied install command for {}", package->name)));
}

void MainWidget::show_error(StringView title, StringView message)
{
    GUI::MessageBox::show(window(), message, title, GUI::MessageBox::Type::Error);
}

}
