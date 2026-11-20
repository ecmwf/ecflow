// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/ServerCatalogue.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

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

// Refuses any request for SSL from a server built without SSL support
Check ssl_supported() {
#ifdef ECF_OPENSSL
    constexpr bool supported = true;
#else
    constexpr bool supported = false;
#endif
    return checks::only_when_supported(supported, "this server was built without SSL support");
}

/// Accepts a comma-separated list of environment variable names, possibly empty
Check variable_names() {
    return [](const Value& value) -> std::optional<std::string> {
        const auto& text = std::get<std::string>(value);
        if (text.empty()) {
            return std::nullopt;
        }
        std::size_t start = 0;
        for (;;) {
            const auto end   = text.find(',', start);
            const auto name  = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
            const bool valid = !name.empty() && !std::isdigit(static_cast<unsigned char>(name[0])) &&
                               std::all_of(name.begin(), name.end(), [](char c) {
                                   return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
                               });
            if (!valid) {
                return std::string("expected comma-separated environment variable names");
            }
            if (end == std::string::npos) {
                return std::nullopt;
            }
            start = end + 1;
        }
    };
}

std::vector<Setting> server_settings() {
    const auto required = checks::non_empty();
    // The server holds these integers as int
    const auto int_max = std::int64_t{std::numeric_limits<int>::max()};
    // clang-format off
    return {

        // server_environment.cfg, overridden by the environment

        Setting(
            variable("ECF_HOME"),
            Type::Text,
            Value{Ecf::HOME()},
            Names{
                "ECF_HOME",                          // env_cfg
                "/ecflow/server/variables/ECF_HOME", // ecf_cfg
                "ECF_HOME",                          // env
                std::nullopt                         // cli
            },
            required),

        Setting(
            variable("ECF_PORT"),
            Type::Integer,
            Value{std::int64_t{Ecf::PORT()}},
            Names{
                "ECF_PORT",                          // env_cfg
                "/ecflow/server/variables/ECF_PORT", // ecf_cfg
                "ECF_PORT",                          // env
                "port"                               // cli
            },
            checks::range(1024, 49150)),

        Setting(
            variable("ECF_CHECK"),
            Type::Text,
            Value{Ecf::CHECKPT()},
            Names{
                "ECF_CHECK",                          // env_cfg
                "/ecflow/server/variables/ECF_CHECK", // ecf_cfg
                "ECF_CHECK",                          // env
                std::nullopt                          // cli
            },
            required),

        Setting(
            variable("ECF_CHECKOLD"),
            Type::Text,
            Value{Ecf::BACKUP_CHECKPT()},
            Names{
                "ECF_CHECKOLD",                          // env_cfg
                "/ecflow/server/variables/ECF_CHECKOLD", // ecf_cfg
                "ECF_CHECKOLD",                          // env
                std::nullopt                             // cli
            },
            required),

        Setting(
            variable("ECF_CHECKINTERVAL"),
            Type::Integer,
            Value{std::int64_t{CheckPt::default_interval()}},
            Names{
                "ECF_CHECKINTERVAL",                          // env_cfg
                "/ecflow/server/variables/ECF_CHECKINTERVAL", // ecf_cfg
                "ECF_CHECKINTERVAL",                          // env
                std::nullopt                                  // cli
            },
            checks::range(1, int_max)),

        Setting(
            variable("ECF_LOG"),
            Type::Text,
            Value{Ecf::LOG_FILE()},
            Names{
                "ECF_LOG",                          // env_cfg
                "/ecflow/server/variables/ECF_LOG", // ecf_cfg
                "ECF_LOG",                          // env
                std::nullopt                        // cli
            },
            required),

        Setting(
            variable("ECF_LISTS"),
            Type::Text,
            Value{string_constants::white_list_file},
            Names{
                "ECF_LISTS",                          // env_cfg
                "/ecflow/server/variables/ECF_LISTS", // ecf_cfg
                "ECF_LISTS",                          // env
                std::nullopt                          // cli
            },
            required),

        Setting(
            variable("ECF_PASSWD"),
            Type::Text,
            Value{Ecf::PASSWD_FILE()},
            Names{
                "ECF_PASSWD",                          // env_cfg
                "/ecflow/server/variables/ECF_PASSWD", // ecf_cfg
                "ECF_PASSWD",                          // env
                std::nullopt                           // cli
            },
            required),

        Setting(
            variable("ECF_CUSTOM_PASSWD"),
            Type::Text,
            Value{Ecf::CUSTOM_PASSWD_FILE()},
            Names{
                "ECF_CUSTOM_PASSWD",                          // env_cfg
                "/ecflow/server/variables/ECF_CUSTOM_PASSWD", // ecf_cfg
                "ECF_CUSTOM_PASSWD",                          // env
                std::nullopt                                  // cli
            },
            required),

        Setting(
            variable("ECF_TASK_THRESHOLD"),
            Type::Integer,
            Value{std::int64_t{Ecf::TASK_THRESHOLD()}},
            Names{
                "ECF_TASK_THRESHOLD",                          // env_cfg
                "/ecflow/server/variables/ECF_TASK_THRESHOLD", // ecf_cfg
                "ECF_TASK_THRESHOLD",                          // env
                std::nullopt                                   // cli
            },
            checks::range(1, int_max)),

        Setting(
            variable("ECF_PRUNE_NODE_LOG"),
            Type::Integer,
            Value{std::int64_t{Ecf::PRUNE_NODE_LOG()}},
            Names{
                "ECF_PRUNE_NODE_LOG",                          // env_cfg
                "/ecflow/server/variables/ECF_PRUNE_NODE_LOG", // ecf_cfg
                "ECF_PRUNE_NODE_LOG",                          // env
                std::nullopt                                   // cli
            },
            checks::range(0, int_max)),

        // server_environment.cfg only, possibly overridden by the command line

        Setting(
            variable("ECF_INTERVAL"),
            Type::Integer,
            Value{std::int64_t{Ecf::SUBMIT_INTERVAL()}},
            Names{
                "ECF_INTERVAL",                          // env_cfg
                "/ecflow/server/variables/ECF_INTERVAL", // ecf_cfg
                std::nullopt,                            // env
                "ecfinterval"                            // cli
            },
            checks::range(1, 60)),

        Setting(
            variable("ECF_CHECKMODE"),
            Type::Text,
            Value{Ecf::CHECK_MODE()},
            Names{
                "ECF_CHECKMODE",                          // env_cfg
                "/ecflow/server/variables/ECF_CHECKMODE", // ecf_cfg
                std::nullopt,                             // env
                std::nullopt                              // cli
            },
            checks::one_of({"CHECK_NEVER", "CHECK_ON_TIME", "CHECK_ALWAYS"})),

        Setting(
            variable("ECF_JOB_CMD"),
            Type::Text,
            Value{Ecf::JOB_CMD()},
            Names{
                "ECF_JOB_CMD",                          // env_cfg
                "/ecflow/server/variables/ECF_JOB_CMD", // ecf_cfg
                std::nullopt,                           // env
                std::nullopt                            // cli
            },
            required),

        Setting(
            variable("ECF_KILL_CMD"),
            Type::Text,
            Value{Ecf::KILL_CMD()},
            Names{
                "ECF_KILL_CMD",                          // env_cfg
                "/ecflow/server/variables/ECF_KILL_CMD", // ecf_cfg
                std::nullopt,                            // env
                std::nullopt                             // cli
            },
            required),

        Setting(
            variable("ECF_STATUS_CMD"),
            Type::Text,
            Value{Ecf::STATUS_CMD()},
            Names{
                "ECF_STATUS_CMD",                          // env_cfg
                "/ecflow/server/variables/ECF_STATUS_CMD", // ecf_cfg
                std::nullopt,                              // env
                std::nullopt                               // cli
            },
            required),

        Setting(
            variable("ECF_CHECK_CMD"),
            Type::Text,
            Value{Ecf::CHECK_CMD()},
            Names{
                "ECF_CHECK_CMD",                          // env_cfg
                "/ecflow/server/variables/ECF_CHECK_CMD", // ecf_cfg
                std::nullopt,                             // env
                std::nullopt                              // cli
            },
            required),

        Setting(
            variable("ECF_URL_CMD"),
            Type::Text,
            Value{Ecf::URL_CMD()},
            Names{
                "ECF_URL_CMD",                          // env_cfg
                "/ecflow/server/variables/ECF_URL_CMD", // ecf_cfg
                std::nullopt,                           // env
                std::nullopt                            // cli
            },
            required),

        Setting(
            variable("ECF_URL_BASE"),
            Type::Text,
            Value{Ecf::URL_BASE()},
            Names{
                "ECF_URL_BASE",                          // env_cfg
                "/ecflow/server/variables/ECF_URL_BASE", // ecf_cfg
                std::nullopt,                            // env
                std::nullopt                             // cli
            },
            required),

        Setting(
            variable("ECF_URL"),
            Type::Text,
            Value{Ecf::URL()},
            Names{
                "ECF_URL",                          // env_cfg
                "/ecflow/server/variables/ECF_URL", // ecf_cfg
                std::nullopt,                       // env
                std::nullopt                        // cli
            },
            required),

        Setting(
            variable("ECF_MICRODEF"),
            Type::Text,
            Value{Ecf::MICRO()},
            Names{
                "ECF_MICRODEF",                          // env_cfg
                "/ecflow/server/variables/ECF_MICRODEF", // ecf_cfg
                std::nullopt,                            // env
                std::nullopt                             // cli
            },
            required),

        // Environment only, possibly overridden by the command line

        Setting(
            variable("ECF_SSL"),
            Type::Text,
            std::nullopt,
            Names{
                std::nullopt,                       // env_cfg
                "/ecflow/server/variables/ECF_SSL", // ecf_cfg
                "ECF_SSL",                          // env
                std::nullopt                        // cli
            },
            ssl_supported()),

        Setting(
            variable("ECF_DEBUG_SERVER"),
            Type::Boolean,
            Value{false},
            Names{
                std::nullopt,                                // env_cfg
                "/ecflow/server/variables/ECF_DEBUG_SERVER", // ecf_cfg
                "ECF_DEBUG_SERVER",                          // env
                "debug"                                      // cli
            },
            {},
            always(true)),

        // <host>.<port>.ecf.cfg only, compared with the host of the server at start-up
        Setting(
            variable("ECF_HOST"),
            Type::Text,
            std::nullopt,
            Names{
                std::nullopt,                        // env_cfg
                "/ecflow/server/variables/ECF_HOST", // ecf_cfg
                std::nullopt,                        // env
                std::nullopt                         // cli
            },
            required),

        // Command line only

        Setting(
            Key::for_option("ecflow.server.http"),
            Type::Boolean,
            Value{false},
            Names{
                std::nullopt,          // env_cfg
                "/ecflow/server/http", // ecf_cfg
                std::nullopt,          // env
                "http"                 // cli
            },
            {},
            always(true)),

        Setting(
            Key::for_option("ecflow.server.ipv6"),
            Type::Boolean,
            Value{false},
            Names{
                std::nullopt,          // env_cfg
                "/ecflow/server/ipv6", // ecf_cfg
                std::nullopt,          // env
                "v6"                   // cli
            },
            {},
            always(true)),

        Setting(
            Key::for_option("ecflow.server.ssl"),
            Type::Boolean,
            Value{false},
            Names{
                std::nullopt,         // env_cfg
                "/ecflow/server/ssl", // ecf_cfg
                std::nullopt,         // env
                "ssl"                 // cli
            },
            ssl_supported(),
            always(true)),

        Setting(
            Key::for_option("ecflow.server.job_generation"),
            Type::Boolean,
            Value{true},
            Names{
                std::nullopt,                    // env_cfg
                "/ecflow/server/job_generation", // ecf_cfg
                std::nullopt,                    // env
                "dis_job_gen"                    // cli
            },
            {},
            always(false)),

        // <host>.<port>.ecf.cfg only

        Setting(
            Key::for_option("ecflow.server.log.commands.enabled"),
            Type::Boolean,
            Value{false},
            Names{
                std::nullopt,                          // env_cfg
                "/ecflow/server/log/commands/enabled", // ecf_cfg
                std::nullopt,                          // env
                std::nullopt                           // cli
            }),

        // Ownership: <host>.<port>.ecf.cfg, overridden by the environment, then the command line

        Setting(
            Key::for_option("ecflow.server.ownership.owner"),
            Type::Text,
            std::nullopt, // ownership is enabled by configuring a server Owner
            Names{
                std::nullopt,                     // env_cfg
                "/ecflow/server/ownership/owner", // ecf_cfg
                "ECF_OWNERSHIP_OWNER",            // env
                "ownership_owner"                 // cli
            },
            required),

        Setting(
            Key::for_option("ecflow.server.ownership.path"),
            Type::Text,
            Value{std::string("/usr/local/bin:/usr/bin:/bin")},
            Names{
                std::nullopt,                    // env_cfg
                "/ecflow/server/ownership/path", // ecf_cfg
                "ECF_OWNERSHIP_PATH",            // env
                "ownership_path"                 // cli
            },
            required),

        Setting(
            Key::for_option("ecflow.server.ownership.environment"),
            Type::Text,
            Value{std::string()},
            Names{
                std::nullopt,                           // env_cfg
                "/ecflow/server/ownership/environment", // ecf_cfg
                "ECF_OWNERSHIP_ENVIRONMENT",            // env
                "ownership_environment"                 // cli
            },
            variable_names()),
    };
    // clang-format on
}

} // namespace

const Catalogue& server_catalogue() {
    static const Catalogue catalogue(server_settings());
    return catalogue;
}

} // namespace ecf::cfg
