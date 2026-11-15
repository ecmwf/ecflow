// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"

namespace ecf::test {

///
/// @brief Returns the paths of every node of the definitions.
///
/// @param[in] defs The definitions
/// @return The absolute paths of the suites and of every node below them.
///
inline std::set<std::string> node_paths(const Defs& defs) {
    std::set<std::string> result;
    for (const auto* node : ecf::get_all_nodes(defs)) {
        result.insert(node->absNodePath());
    }
    return result;
}

///
/// @brief Returns the paths of a node and of every node below it.
///
/// @param[in] node The node
/// @return The absolute paths.
///
inline std::set<std::string> node_paths(Node& node) {
    std::set<std::string> result;
    for (const auto* n : ecf::get_all_nodes(node)) {
        result.insert(n->absNodePath());
    }
    return result;
}

///
/// @brief Names a channel through which a client receives nodes, and fetches what it receives.
///
struct Channel
{
    std::string name; ///< The channel, as the request that uses it
    std::function<std::set<std::string>(const std::function<std::unique_ptr<ClientInvoker>()>&)>
        receive; ///< Returns the nodes received by a fresh client of the identity
};

///
/// @brief Returns the channels that carry full definitions: --get, --get_state and --migrate without a path, a full
///        synchronisation without a handle, and a full synchronisation through a handle holding the given suites.
///
/// @param[in] suites The suites registered in the handle
/// @return The channels.
/// @throws std::runtime_error from a channel, when its request fails.
///
inline std::vector<Channel> full_channels(const std::vector<std::string>& suites) {
    const auto require = [](const ClientInvoker& client, int status, const std::string& what) {
        if (status != 0) {
            throw std::runtime_error(what + " failed: " + client.errorMsg());
        }
    };
    const auto defs_of = [require](const std::string& request) {
        return [request, require](const std::function<std::unique_ptr<ClientInvoker>()>& make) {
            auto client = make();
            require(*client, client->invoke(request), request);
            return node_paths(*client->defs());
        };
    };
    return {
        {"--get", defs_of(CtsApi::get())},
        {"--get_state", defs_of(CtsApi::get_state())},
        {"--migrate", defs_of(CtsApi::migrate())},
        {"full synchronisation",
         [require](const std::function<std::unique_ptr<ClientInvoker>()>& make) {
             auto client = make();
             require(*client, client->sync_local(), "--sync_full");
             return node_paths(*client->defs());
         }},
        {"handle",
         [require, suites](const std::function<std::unique_ptr<ClientInvoker>()>& make) {
             auto client = make();
             require(*client, client->ch_register(false, suites), "--ch_register");
             require(*client, client->sync_local(), "--sync_full with a handle");
             auto result = node_paths(*client->defs());
             require(*client, client->ch_drop(client->client_handle()), "--ch_drop");
             return result;
         }},
    };
}

///
/// @brief Returns the channels that list or count suites: --suites, and --stats (as many paths as suites counted).
///
/// @return The channels; each returns the paths of the suites it names (or, for --stats, "/#<n>" for a count n).
/// @throws std::runtime_error from a channel, when its request fails.
///
inline std::vector<Channel> listing_channels() {
    const auto require = [](const ClientInvoker& client, int status, const std::string& what) {
        if (status != 0) {
            throw std::runtime_error(what + " failed: " + client.errorMsg());
        }
    };
    return {
        {"--suites",
         [require](const std::function<std::unique_ptr<ClientInvoker>()>& make) {
             auto client = make();
             require(*client, client->suites(), "--suites");
             std::set<std::string> result;
             for (const auto& name : client->server_reply().get_string_vec()) {
                 result.insert("/" + name);
             }
             return result;
         }},
        {"--stats",
         [require](const std::function<std::unique_ptr<ClientInvoker>()>& make) {
             auto client = make();
             require(*client, client->stats(), "--stats");
             const auto& text = client->get_string();
             const auto at    = text.find("Number of Suites");
             if (at == std::string::npos) {
                 throw std::runtime_error("--stats: no number of suites in " + text);
             }
             const auto line   = text.substr(at, text.find('\n', at) - at);
             const auto digits = line.find_first_of("0123456789");
             return std::set<std::string>{"/#" +
                                          line.substr(digits, line.find_first_not_of("0123456789", digits) - digits)};
         }},
    };
}

///
/// @brief Names a request addressing a node, which is denied when the requester cannot read the node.
///
struct NodeRequest
{
    std::string name;                                            ///< The request
    std::function<int(ClientInvoker&, const std::string&)> send; ///< Sends the request for a node; returns its status
    std::string expected; ///< Text found in the reply, when allowed, for a complete task with the script "echo hello"
};

///
/// @brief Returns the requests that read a node (R5.2): --query, --file and --edit_script.
///
/// @return The requests.
///
inline std::vector<NodeRequest> node_reading_requests() {
    return {
        {"--query", [](ClientInvoker& c, const std::string& path) { return c.query("state", path, ""); }, "complete"},
        {"--file", [](ClientInvoker& c, const std::string& path) { return c.file(path, "script"); }, "echo hello"},
        {"--edit_script",
         [](ClientInvoker& c, const std::string& path) { return c.edit_script_edit(path); },
         "echo hello"},
        {"--get", [](ClientInvoker& c, const std::string& path) { return c.invoke(CtsApi::get(path)); }, "t"},
    };
}

} // namespace ecf::test
