/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/String.h>
#include <AK/Vector.h>
#include <LibGUI/Model.h>

namespace AppStore {

struct PackageInfo {
    String name;
    String version;
    String website;
    String installed_version;
    String port_directory;
    bool is_installed { false };
    bool has_update { false };
    bool has_port_directory { false };
};

String package_status_text(PackageInfo const&);

class PackageModel final : public GUI::Model {
public:
    enum class FilterMode {
        All,
        Installed,
        Updates,
    };

    enum Column {
        Name,
        Version,
        Status,
        __Count
    };

    virtual ~PackageModel() override = default;

    static NonnullRefPtr<PackageModel> create();
    void set_packages(Vector<PackageInfo>);
    void set_filter_mode(FilterMode);
    FilterMode filter_mode() const { return m_filter_mode; }
    PackageInfo const* package_at(int row) const;

    virtual int row_count(GUI::ModelIndex const& = GUI::ModelIndex()) const override;
    virtual int column_count(GUI::ModelIndex const& = GUI::ModelIndex()) const override { return Column::__Count; }
    virtual ErrorOr<String> column_name(int column) const override;
    virtual GUI::ModelIndex index(int row, int column = 0, GUI::ModelIndex const& parent = GUI::ModelIndex()) const override;
    virtual GUI::Variant data(GUI::ModelIndex const&, GUI::ModelRole = GUI::ModelRole::Display) const override;
    virtual GUI::Model::MatchResult data_matches(GUI::ModelIndex const&, GUI::Variant const&) const override;

private:
    PackageModel() = default;

    Vector<PackageInfo> m_packages;
    FilterMode m_filter_mode { FilterMode::All };
};

}
