// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ===========================================================================
// IMPORTANT: These appear as python doc strings.
//            Additionally, they are auto-documented using sphinx-docs
//            Hence the doc strings use reStructuredText markup
// ===========================================================================
class NodeAttrDoc {
public:
    NodeAttrDoc() = delete;

    static const char* flag_type_doc();
    static const char* sortable_attribute_type_doc();
    static const char* plain_zombie_doc();
    static const char* variable_doc();
    static const char* variable_list_doc();
    static const char* zombie_doc();
    static const char* zombie_type_doc();
    static const char* zombie_user_action_type_doc();
    static const char* child_cmd_type_doc();
    static const char* label_doc();
    static const char* limit_doc();
    static const char* inlimit_doc();
    static const char* event_doc();
    static const char* meter_doc();
    static const char* queue_doc();
    static const char* generic_doc();
    static const char* date_doc();
    static const char* day_doc();
    static const char* days_enum_doc();
    static const char* time_doc();
    static const char* today_doc();
    static const char* late_doc();
    static const char* autocancel_doc();
    static const char* autoarchive_doc();
    static const char* autorestore_doc();

    ///
    /// @brief Returns the docstring of the class Permissions.
    ///
    /// @return The docstring, a static string.
    ///
    static const char* permissions_doc();

    ///
    /// @brief Returns the docstring of Permissions.add_to.
    ///
    /// @return The docstring, a static string.
    ///
    static const char* permissions_add_to_doc();

    ///
    /// @brief Returns the docstring of Permissions.entries.
    ///
    /// @return The docstring, a static string.
    ///
    static const char* permissions_entries_doc();

    ///
    /// @brief Returns the docstring of Permissions.roles.
    ///
    /// @return The docstring, a static string.
    ///
    static const char* permissions_roles_doc();

    ///
    /// @brief Returns the docstring of Permissions.sticky.
    ///
    /// @return The docstring, a static string.
    ///
    static const char* permissions_sticky_doc();

    ///
    /// @brief Returns the docstring of Permissions.to_cli.
    ///
    /// @return The docstring, a static string.
    ///
    static const char* permissions_to_cli_doc();

    ///
    /// @brief Returns the docstring of Permissions.to_json.
    ///
    /// @return The docstring, a static string.
    ///
    static const char* permissions_to_json_doc();
    static const char* repeat_doc();
    static const char* repeat_date_doc();
    static const char* repeat_datetime_doc();
    static const char* repeat_date_list_doc();
    static const char* repeat_datetimelist_doc();
    static const char* repeat_integer_doc();
    static const char* repeat_enumerated_doc();
    static const char* repeat_string_doc();
    static const char* repeat_day_doc();
    static const char* cron_doc();
    static const char* clock_doc();
    static const char* aviso_doc();
    static const char* mirror_doc();
};
