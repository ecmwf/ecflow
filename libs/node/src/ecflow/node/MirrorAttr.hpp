/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_MirrorAttr_HPP
#define ecflow_node_MirrorAttr_HPP

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/service/mirror/MirrorService.hpp"

namespace cereal {
class access;
}

class Node;

namespace ecf {

///
/// \brief MirrorAttr represents an attribute, attached to a \ref Node.
///
/// The Mirror attribute effectively mirrors the status of a remote Node, allowing other Nodes to be triggered.
///

class MirrorAttr {
public:
    using name_t        = std::string;
    using remote_path_t = std::string;
    using remote_host_t = std::string;
    using remote_port_t = std::string;
    using polling_t     = std::string;
    using flag_t        = bool;
    using auth_t        = std::string;
    using reason_t      = std::string;

    using controller_t     = ecf::service::mirror::MirrorController;
    using controller_ptr_t = std::shared_ptr<controller_t>;

    // Default values for the options, refer to expected ecFlow variables
    static constexpr const char* default_remote_host = "%ECF_MIRROR_REMOTE_HOST%";
    static constexpr const char* default_remote_port = "%ECF_MIRROR_REMOTE_PORT%";
    static constexpr const char* default_polling     = "%ECF_MIRROR_REMOTE_POLLING%";
    static constexpr const char* default_remote_auth = "%ECF_MIRROR_REMOTE_AUTH%";

    // Fallback option values, used when the variables providing default values are not defined
    static constexpr const char* fallback_remote_port = "3141";
    static constexpr const char* fallback_polling     = "120";
    static constexpr const char* fallback_remote_auth = "";

    static constexpr const char* reload_option_value = "reload";

    static bool is_valid_name(const std::string& name);

    /**
     * Creates a(n invalid) Mirror
     *
     * Note: this is required by Cereal serialization
     *       Cereal invokes the default ctor to create the object and only then proceeds to member-wise serialization.
     */
    MirrorAttr() = default;
    MirrorAttr(Node* parent,
               name_t name,
               remote_path_t remote_path,
               remote_host_t remote_host,
               remote_port_t remote_port,
               polling_t polling,
               flag_t ssl,
               auth_t auth,
               reason_t reason);

    MirrorAttr(const MirrorAttr& rhs) = default;
    ~MirrorAttr();

    MirrorAttr& operator=(const MirrorAttr& rhs) = default;

    [[nodiscard]] MirrorAttr make_detached() const;

    [[nodiscard]] inline const std::string& name() const { return name_; }
    [[nodiscard]] std::string absolute_name() const;

    [[nodiscard]] inline const std::string& remote_path() const { return remote_path_; }
    [[nodiscard]] inline const std::string& remote_host() const { return remote_host_; }
    [[nodiscard]] inline const std::string& remote_port() const { return remote_port_; }
    [[nodiscard]] inline polling_t polling() const { return polling_; }
    [[nodiscard]] inline flag_t ssl() const { return ssl_; }
    [[nodiscard]] inline const std::string& auth() const { return auth_; }
    [[nodiscard]] inline const std::string& reason() const { return reason_; }

    void set_parent(Node* parent) { parent_ = parent; }

    unsigned int state_change_no() const { return state_change_no_; }

    bool why(std::string& theReasonWhy) const;

    /**
     * Initialises the Mirror procedure, which effectively starts the background polling mechanism.
     * Typically, called when traversing the tree -- does nothing if Mirror service is already set up.
     */
    void reset();
    /**
     * Restarts the Mirror procedure, which effectively stops before restarting the background polling mechanism.
     * Typicallly, called explicitly via Alter command -- forces the reinitialisation of the Mirror service,
     * guaranteeing that parameters, given as ECF variables, are reevaluated.
     */
    void reload();
    void finish();

    /**
     * Check if state changes were detected by the background polling mechanism, and if so, reflect it on the Node.
     */
    void mirror();

    template <class Archive>
    friend void serialize(Archive& ar, MirrorAttr& aviso, std::uint32_t version);

private:
    std::optional<std::string> resolve_cfg(const std::string& value, std::string_view default_value) const;
    std::string
    resolve_cfg(const std::string& value, std::string_view default_value, std::string_view fallback_value) const;

    void start_controller();
    void stop_controller();

    Node* parent_{nullptr}; // only ever used on the server side, to update parent Node state
    name_t name_;
    remote_path_t remote_path_;
    remote_host_t remote_host_;
    remote_port_t remote_port_;
    polling_t polling_;
    flag_t ssl_;
    auth_t auth_;
    reason_t reason_;

    unsigned int state_change_no_{0}; // *not* persisted, only used on server side

    // The controller is only instanciated when the Mirror is reset()
    // This allows the MirrorAttr have a copy-ctor and assignment operator
    controller_ptr_t controller_;
};

bool operator==(const MirrorAttr& lhs, const MirrorAttr& rhs);

template <class Archive>
void serialize(Archive& ar, MirrorAttr& aviso, [[maybe_unused]] std::uint32_t version) {
    ar & aviso.name_;
    ar & aviso.remote_path_;
    ar & aviso.remote_host_;
    ar & aviso.remote_port_;
    ar & aviso.polling_;
    ar & aviso.ssl_;
    ar & aviso.auth_;
    ar & aviso.reason_;
}

} // namespace ecf

#endif /* ecflow_node_MirrorAttr_HPP */
