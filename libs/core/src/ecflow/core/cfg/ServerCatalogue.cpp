// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/ServerCatalogue.hpp"

#include "ecflow/core/CheckPt.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Str.hpp"

namespace ecf::cfg {

namespace {

Key variable(const std::string& name) {
    return Key::for_variable("ecflow.server.variables", name);
}

// A boolean whose value is decided by the presence of its input, whatever the text
Parse always(bool value) {
    return [value](std::string_view) { return Value{value}; };
}

std::vector<Setting> server_settings() {
    const auto required = checks::non_empty();
    // clang-format off
    return {

        // server_environment.cfg, overridden by the environment

        Setting(
            variable("ECF_HOME"),
            Type::Text,
            Value{Ecf::HOME()},
            Names{"ECF_HOME", "ECF_HOME", std::nullopt},
            required),

        Setting(
            variable("ECF_PORT"),
            Type::Integer,
            Value{std::int64_t{Ecf::PORT()}},
            Names{"ECF_PORT", "ECF_PORT", "port"},
            checks::range(1024, 49150)),

        Setting(
            variable("ECF_CHECK"),
            Type::Text,
            Value{Ecf::CHECKPT()},
            Names{"ECF_CHECK", "ECF_CHECK", std::nullopt},
            required),

        Setting(
            variable("ECF_CHECKOLD"),
            Type::Text,
            Value{Ecf::BACKUP_CHECKPT()},
            Names{"ECF_CHECKOLD", "ECF_CHECKOLD", std::nullopt},
            required),

        Setting(
            variable("ECF_CHECKINTERVAL"),
            Type::Integer,
            Value{std::int64_t{CheckPt::default_interval()}},
            Names{"ECF_CHECKINTERVAL", "ECF_CHECKINTERVAL", std::nullopt},
            checks::at_least(1)),

        Setting(
            variable("ECF_LOG"),
            Type::Text,
            Value{Ecf::LOG_FILE()},
            Names{"ECF_LOG", "ECF_LOG", std::nullopt},
            required),

        Setting(
            variable("ECF_LISTS"),
            Type::Text,
            Value{string_constants::white_list_file},
            Names{"ECF_LISTS", "ECF_LISTS", std::nullopt},
            required),

        Setting(
            variable("ECF_PASSWD"),
            Type::Text,
            Value{Ecf::PASSWD_FILE()},
            Names{"ECF_PASSWD", "ECF_PASSWD", std::nullopt},
            required),

        Setting(
            variable("ECF_CUSTOM_PASSWD"),
            Type::Text,
            Value{Ecf::CUSTOM_PASSWD_FILE()},
            Names{"ECF_CUSTOM_PASSWD", "ECF_CUSTOM_PASSWD", std::nullopt},
            required),

        Setting(
            variable("ECF_TASK_THRESHOLD"),
            Type::Integer,
            Value{std::int64_t{Ecf::TASK_THRESHOLD()}},
            Names{"ECF_TASK_THRESHOLD", "ECF_TASK_THRESHOLD", std::nullopt},
            checks::at_least(1)),

        Setting(
            variable("ECF_PRUNE_NODE_LOG"),
            Type::Integer,
            Value{std::int64_t{Ecf::PRUNE_NODE_LOG()}},
            Names{"ECF_PRUNE_NODE_LOG", "ECF_PRUNE_NODE_LOG", std::nullopt},
            checks::at_least(0)),

        // server_environment.cfg only, possibly overridden by the command line

        Setting(
            variable("ECF_INTERVAL"),
            Type::Integer,
            Value{std::int64_t{Ecf::SUBMIT_INTERVAL()}},
            Names{"ECF_INTERVAL", std::nullopt, "ecfinterval"},
            checks::range(1, 60)),

        Setting(
            variable("ECF_CHECKMODE"),
            Type::Text,
            Value{Ecf::CHECK_MODE()},
            Names{"ECF_CHECKMODE", std::nullopt, std::nullopt},
            checks::one_of({"CHECK_NEVER", "CHECK_ON_TIME", "CHECK_ALWAYS"})),

        Setting(
            variable("ECF_JOB_CMD"),
            Type::Text,
            Value{Ecf::JOB_CMD()},
            Names{"ECF_JOB_CMD", std::nullopt, std::nullopt},
            required),

        Setting(
            variable("ECF_KILL_CMD"),
            Type::Text,
            Value{Ecf::KILL_CMD()},
            Names{"ECF_KILL_CMD", std::nullopt, std::nullopt},
            required),

        Setting(
            variable("ECF_STATUS_CMD"),
            Type::Text,
            Value{Ecf::STATUS_CMD()},
            Names{"ECF_STATUS_CMD", std::nullopt, std::nullopt},
            required),

        Setting(
            variable("ECF_CHECK_CMD"),
            Type::Text,
            Value{Ecf::CHECK_CMD()},
            Names{"ECF_CHECK_CMD", std::nullopt, std::nullopt},
            required),

        Setting(
            variable("ECF_URL_CMD"),
            Type::Text,
            Value{Ecf::URL_CMD()},
            Names{"ECF_URL_CMD", std::nullopt, std::nullopt},
            required),

        Setting(
            variable("ECF_URL_BASE"),
            Type::Text,
            Value{Ecf::URL_BASE()},
            Names{"ECF_URL_BASE", std::nullopt, std::nullopt},
            required),

        Setting(
            variable("ECF_URL"),
            Type::Text,
            Value{Ecf::URL()},
            Names{"ECF_URL", std::nullopt, std::nullopt},
            required),

        Setting(
            variable("ECF_MICRODEF"),
            Type::Text,
            Value{Ecf::MICRO()},
            Names{"ECF_MICRODEF", std::nullopt, std::nullopt},
            required),

        // Environment only, possibly overridden by the command line

        Setting(
            variable("ECF_SSL"),
            Type::Text,
            std::nullopt,
            Names{std::nullopt, "ECF_SSL", std::nullopt}),

        Setting(
            variable("ECF_DEBUG_SERVER"),
            Type::Boolean,
            Value{false},
            Names{std::nullopt, "ECF_DEBUG_SERVER", "debug"},
            {},
            always(true)),

        // Command line only

        Setting(
            Key::for_option("ecflow.server.http"),
            Type::Boolean,
            Value{false},
            Names{std::nullopt, std::nullopt, "http"},
            {},
            always(true)),

        Setting(
            Key::for_option("ecflow.server.ipv6"),
            Type::Boolean,
            Value{false},
            Names{std::nullopt, std::nullopt, "v6"},
            {},
            always(true)),

        Setting(
            Key::for_option("ecflow.server.ssl"),
            Type::Boolean,
            Value{false},
            Names{std::nullopt, std::nullopt, "ssl"},
            {},
            always(true)),

        Setting(
            Key::for_option("ecflow.server.job_generation"),
            Type::Boolean,
            Value{true},
            Names{std::nullopt, std::nullopt, "dis_job_gen"},
            {},
            always(false)),
    };
    // clang-format on
}

} // namespace

const Catalogue& server_catalogue() {
    static const Catalogue catalogue(server_settings());
    return catalogue;
}

} // namespace ecf::cfg
