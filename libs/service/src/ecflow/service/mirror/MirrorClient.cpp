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

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/service/Log.hpp"

namespace ecf::service::mirror {
/* MirrorClient */

struct MirrorClient::Impl
{
    std::shared_ptr<Defs> defs_;
    ClientInvoker invoker_;
};

MirrorClient::MirrorClient() : impl_(std::make_unique<Impl>()) {
}

MirrorClient::~MirrorClient() = default;

int MirrorClient::get_node_status(const std::string& remote_host,
                                  const std::string& remote_port,
                                  const std::string& node_path,
                                  bool ssl,
                                  const std::string& remote_username,
                                  const std::string& remote_password) const {
    SLOG(D, "MirrorClient: Accessing " << remote_host << ":" << remote_port << ", path=" << node_path);
    SLOG(D, "MirrorClient: Authentication Credentials:  " << remote_username << ":" << remote_password);

    try {
        impl_ = std::make_unique<Impl>();
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

        SLOG(D, "MirrorClient: retrieving the latest defs");
        impl_->invoker_.sync(impl_->defs_);

        if (!impl_->defs_) {
            SLOG(E, "MirrorClient: unable to sync with remote defs");
            throw std::runtime_error("MirrorClient: Failed to sync with remote defs");
        }

        auto node = impl_->defs_->findAbsNode(node_path);

        if (!node) {
            throw std::runtime_error(
                Message("MirrorClient: Unable to find requested node (", node_path, ") in remote remote defs").str());
        }

        auto state = node->state();
        SLOG(D, "MirrorClient: found node (" << node_path << "), with state " << state);
        return state;
    }
    catch (std::exception& e) {
        throw std::runtime_error(Message("MirrorClient: failure to sync remote defs, due to: ", e.what()));
    }
}

} // namespace ecf::service::mirror
