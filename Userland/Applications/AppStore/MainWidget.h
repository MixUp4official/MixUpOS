/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/ByteString.h>
#include "PackageModel.h"
#include <AK/OwnPtr.h>
#include <AK/Optional.h>
#include <AK/RefPtr.h>
#include <LibGUI/Widget.h>

namespace Core {
class File;
}

namespace GUI {
class Button;
class FilteringProxyModel;
class Label;
class SortingProxyModel;
class Statusbar;
class TableView;
class TextBox;
class TextEditor;
}

namespace Protocol {
class Request;
class RequestClient;
}

namespace AppStore {

class MainWidget final : public GUI::Widget {
    C_OBJECT(MainWidget)

public:
    virtual ~MainWidget() override = default;

    ErrorOr<void> initialize();
    void reload_packages();
    void fetch_latest_packages();
    void set_filter_mode(PackageModel::FilterMode);
    void update_all_packages();

private:
    MainWidget() = default;

    ErrorOr<void> load_packages_from_best_source(bool allow_network_fetch_if_missing = true);
    ErrorOr<void> load_packages_from_path(StringView path, StringView source_name);
    ErrorOr<Vector<PackageInfo>> read_packages_from_markdown(StringView path);
    ErrorOr<void> apply_installed_status(Vector<PackageInfo>&);
    ErrorOr<void> ensure_cache_directory_exists();
    ErrorOr<ByteString> cache_directory() const;
    ByteString cache_file_path() const;
    PackageInfo const* selected_package() const;
    size_t available_update_count() const;
    ErrorOr<String> build_update_all_command() const;
    void update_details();
    void update_status_text(Optional<String> = {});
    void select_package_by_name(StringView);
    void sync_filter_buttons();
    void update_actions();
    void open_selected_website();
    void open_selected_port_directory();
    void install_selected_package();
    void copy_selected_install_command();
    void show_error(StringView title, StringView message);

    RefPtr<PackageModel> m_package_model;
    RefPtr<GUI::FilteringProxyModel> m_filtering_model;
    RefPtr<GUI::SortingProxyModel> m_sorting_model;

    RefPtr<GUI::TextBox> m_search_box;
    RefPtr<GUI::TableView> m_table_view;
    RefPtr<GUI::Label> m_title_label;
    RefPtr<GUI::Label> m_source_label;
    RefPtr<GUI::TextEditor> m_details_box;
    RefPtr<GUI::Button> m_all_filter_button;
    RefPtr<GUI::Button> m_installed_filter_button;
    RefPtr<GUI::Button> m_updates_filter_button;
    RefPtr<GUI::Button> m_reload_button;
    RefPtr<GUI::Button> m_fetch_button;
    RefPtr<GUI::Button> m_update_all_button;
    RefPtr<GUI::Button> m_install_button;
    RefPtr<GUI::Button> m_copy_command_button;
    RefPtr<GUI::Button> m_website_button;
    RefPtr<GUI::Button> m_open_folder_button;
    RefPtr<GUI::Statusbar> m_statusbar;

    RefPtr<Protocol::RequestClient> m_request_client;
    RefPtr<Protocol::Request> m_active_request;
    OwnPtr<Core::File> m_fetch_output_file;

    String m_last_source_name;
    String m_last_source_path;
};

}
