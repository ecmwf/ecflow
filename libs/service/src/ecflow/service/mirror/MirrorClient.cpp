/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/service/mirror/MirrorClient.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/service/Log.hpp"

namespace ecf::service::mirror {

namespace {
    std::string get_suite_name(const std::string& node_path) {
        std::string trimmed = node_path.substr(1);
        trimmed = trimmed.substr(0, trimmed.find('/'));
        return trimmed;
    }
}

struct MirrorClient::Impl
{
    ClientInvoker invoker_;
    bool initialized_ = false;

    ~Impl() {
        // Release Suite filter handle
        invoker_.ch1_drop();
    }
};

MirrorClient::MirrorClient() : impl_(std::make_unique<Impl>()) {
}

MirrorClient::~MirrorClient() = default;

MirrorData MirrorClient::get_node_status(const std::string& remote_host,
                                         const std::string& remote_port,
                                         const std::string& node_path,
                                         bool ssl,
                                         const std::string& remote_username,
                                         const std::string& remote_password) const {
    SLOG(D, "MirrorClient: Accessing " << remote_host << ":" << remote_port << ", path=" << node_path);
    SLOG(D, "MirrorClient: Authentication Credentials:  " << remote_username << ":" << remote_password);

    try {
        if (!impl_->initialized_) {
            // Setup Access/Authentication
            impl_->invoker_.set_host_port(remote_host, remote_port);
            if (ssl) {
                impl_->invoker_.enable_ssl();
            }
            if (!remote_username.empty()) {
                impl_->invoker_.set_user_name(remote_username);
            }
            if (!remote_password.empty()) {
                // Extremely important: the password actually needs to be encrypted before being set in the invoker!
                impl_->invoker_.set_password(PasswordEncryption::encrypt(remote_password, remote_username));
            }
            // Setup Suite filter handle
            auto selected_suite = get_suite_name(node_path);
            impl_->invoker_.ch1_register(false, std::vector{selected_suite});
        }

        SLOG(D, "MirrorClient: retrieving the latest defs");
        impl_->invoker_.sync_local();

        auto defs = impl_->invoker_.defs();
        if (!defs) {
            SLOG(E, "MirrorClient: unable to sync with remote defs");
            throw std::runtime_error("MirrorClient: Failed to sync with remote defs");
        }

        auto node = defs->findAbsNode(node_path);

        if (!node) {
            throw std::runtime_error(
                Message("MirrorClient: Unable to find requested node (", node_path, ") in remote remote defs").str());
        }

        MirrorData data{};

        // ** Node State
        data.state = node->state();

        // ** Node Variables
        data.regular_variables   = node->variables();
        data.inherited_variables = node->get_all_inherited_variables();
        data.generated_variables = node->get_all_generated_variables();

        // Filter out the Definitions structural variables (SUITE, to avoid conflicts with "local" side definitions
        data.generated_variables.erase(
            std::remove_if(std::begin(data.generated_variables),
                           std::end(data.generated_variables),
                           [](const auto& variable) {
                               return boost::algorithm::starts_with(variable.name(), "TASK") ||
                                      boost::algorithm::starts_with(variable.name(), "FAMILY") ||
                                      boost::algorithm::starts_with(variable.name(), "SUITE");
                           }),
            std::end(data.generated_variables));

        // ** Node Labels
        data.labels = node->labels();
        // ** Node Meters
        data.meters = node->meters();
        // ** Node Events
        data.events = node->events();

        SLOG(D, "MirrorClient: found node (" << node_path << "), with state " << data.state);
        return data;
    }
    catch (std::exception& e) {
        throw std::runtime_error(Message("MirrorClient: failure to sync remote defs, due to: ", e.what()));
    }
}

} // namespace ecf::service::mirror
