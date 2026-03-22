/*
 * Copyright (c) 2026, OpenAI
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "PackageModel.h"
#include <AK/FuzzyMatch.h>

namespace AppStore {

NonnullRefPtr<PackageModel> PackageModel::create()
{
    return adopt_ref(*new PackageModel);
}

static bool package_matches_filter(PackageInfo const& package, PackageModel::FilterMode filter_mode)
{
    switch (filter_mode) {
    case PackageModel::FilterMode::All:
        return true;
    case PackageModel::FilterMode::Installed:
        return package.is_installed;
    case PackageModel::FilterMode::Updates:
        return package.has_update;
    }
    VERIFY_NOT_REACHED();
}

String package_status_text(PackageInfo const& package)
{
    if (!package.is_installed)
        return "Available"_string;
    if (package.has_update)
        return MUST(String::formatted("Installed ({}) - update available", package.installed_version));
    return MUST(String::formatted("Installed ({})", package.installed_version));
}

void PackageModel::set_packages(Vector<PackageInfo> packages)
{
    m_packages = move(packages);
    did_update();
}

void PackageModel::set_filter_mode(FilterMode filter_mode)
{
    if (m_filter_mode == filter_mode)
        return;
    m_filter_mode = filter_mode;
    did_update();
}

PackageInfo const* PackageModel::package_at(int row) const
{
    if (row < 0 || static_cast<size_t>(row) >= m_packages.size())
        return nullptr;
    return &m_packages[row];
}

int PackageModel::row_count(GUI::ModelIndex const& index) const
{
    if (index.is_valid())
        return 0;
    return m_packages.size();
}

ErrorOr<String> PackageModel::column_name(int column) const
{
    switch (column) {
    case Column::Name:
        return "Package"_string;
    case Column::Version:
        return "Version"_string;
    case Column::Status:
        return "Status"_string;
    default:
        VERIFY_NOT_REACHED();
    }
}

GUI::ModelIndex PackageModel::index(int row, int column, GUI::ModelIndex const& parent) const
{
    if (parent.is_valid())
        return {};
    auto const* package = package_at(row);
    if (!package)
        return {};
    return create_index(row, column, package);
}

GUI::Variant PackageModel::data(GUI::ModelIndex const& index, GUI::ModelRole role) const
{
    auto const* package = package_at(index.row());
    if (!package)
        return {};

    if (role == GUI::ModelRole::Display) {
        switch (index.column()) {
        case Column::Name:
            return package->name;
        case Column::Version:
            return package->version;
        case Column::Status:
            return package_status_text(*package);
        default:
            VERIFY_NOT_REACHED();
        }
    }

    if (role == GUI::ModelRole::Sort) {
        if (index.column() == Column::Status) {
            if (package->has_update)
                return 0;
            if (package->is_installed)
                return 1;
            return 2;
        }
        return data(index, GUI::ModelRole::Display);
    }

    return {};
}

GUI::Model::MatchResult PackageModel::data_matches(GUI::ModelIndex const& index, GUI::Variant const& term) const
{
    auto const* package = package_at(index.row());
    if (!package)
        return { TriState::False };

    if (!package_matches_filter(*package, m_filter_mode))
        return { TriState::False };

    auto needle = term.as_string();
    if (needle.is_empty())
        return { TriState::True };

    auto haystack = MUST(String::formatted("{} {} {} {}", package->name, package->version, package->website, package_status_text(*package)));
    auto match = fuzzy_match(needle, haystack);
    if (match.score > 0)
        return { TriState::True, match.score };
    return { TriState::False };
}

}
