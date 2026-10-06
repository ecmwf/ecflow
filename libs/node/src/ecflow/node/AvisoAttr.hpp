// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include "ecflow/core/Log.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/service/aviso/AvisoBackend.hpp"

namespace cereal {
class access;
}

class Node;

namespace ecf {

///
/// \brief AvisoAttr represents an attribute, attached to a \ref Node.
///
/// The Aviso attribute effectively acts as a trigger for the Node, granting
/// the Node to be (re)queued as soon as a related notification is received.
///
/// \see https://github.com/ecmwf/aviso
///

class AvisoAttr {
public:
    using path_t     = std::string;
    using name_t     = std::string;
    using listener_t = std::string;
    using active_t   = std::string;
    using url_t      = std::string;
    using revision_t = std::uint64_t;
    using auth_t     = std::string;
    using reason_t   = std::string;

    using backend_t     = ecf::service::aviso::AvisoBackend;
    using backend_ptr_t = std::shared_ptr<backend_t>;

    static constexpr const char* default_url  = "%ECF_AVISO_URL%";
    static constexpr const char* default_auth = "%ECF_AVISO_AUTH%";

    static constexpr const char* reload_option_value = "reload";

    static bool is_valid_name(const std::string& name);

    /**
     * Creates a(n invalid) Aviso
     *
     * Note: this is required by Cereal serialization
     *       Cereal invokes the default ctor to create the object and only then proceeds to member-wise serialization.
     */
    AvisoAttr() = default;
    AvisoAttr(Node* parent,
              name_t name,
              const listener_t& handle,
              url_t url,
              revision_t revision,
              auth_t auth,
              const reason_t& reason);
    AvisoAttr(const AvisoAttr& rhs) = default;

    AvisoAttr& operator=(const AvisoAttr& rhs) = default;

    [[nodiscard]] AvisoAttr make_detached() const;

    [[nodiscard]] inline Node* parent() const { return parent_; }
    [[nodiscard]] inline const name_t& name() const { return name_; }
    [[nodiscard]] inline const listener_t& listener() const { return listener_; }
    [[nodiscard]] inline const url_t& url() const { return url_; }
    [[nodiscard]] inline revision_t revision() const { return revision_; }
    [[nodiscard]] inline const auth_t& auth() const { return auth_; }
    [[nodiscard]] inline const reason_t& reason() const { return reason_; }
    [[nodiscard]] inline const active_t& active() const { return active_; }
    [[nodiscard]] path_t path() const;

    ///
    /// @brief Returns whether the attribute currently holds an error (i.e. a non-empty reason).
    ///
    [[nodiscard]] bool has_error() const;

    void set_listener(std::string_view listener);
    void set_revision(revision_t revision);

    unsigned int state_change_no() const { return state_change_no_; }

    bool why(std::string& theReasonWhy) const;

    /**
     * Initialises the Aviso procedure, which effectively starts receiving notifications in the background.
     * Typically, called when traversing the tree -- does nothing if Aviso service is already set up.
     */
    void reset();

    /**
     * Restarts the Aviso procedure, which effectively stops before restarting the background notifications.
     * Typicallly, called explicitly via Alter command -- forces the reinitialisation of the Aviso service,
     * guaranteeing that parameters, given as ECF variables, are reevaluated.
     */
    void reload();

    [[nodiscard]] bool isFree() const;

    void start() const;
    void finish() const;

    template <class Archive>
    friend void serialize(Archive& ar, AvisoAttr& aviso, std::uint32_t version);

    /**
     * \brief Finishes all the Aviso attributes, effectively stopping the background notifications.
     *
     * @param avisos the avisos to finish
     */
    static void finish(const std::vector<AvisoAttr>& avisos);

    /**
     * \brief When the given state is a Task "terminal" state (i.e. complete, aborted, unknown), finishes all Aviso
     * attributes, effectively stopping the background notifications.
     *
     * @param avisos the avisos to finish
     * @param state the state to check against
     */
    static void finish(const std::vector<AvisoAttr>& avisos, NState::State state);

private:
    void start_backend(const std::string& aviso_path,
                       const std::string& aviso_listener,
                       const std::string& aviso_url,
                       const std::string& aviso_auth) const;
    void stop_backend() const;

    void set_error(const std::string& reason) const;
    void clear_error() const;

    /**
     * @brief The parent Node of this AvisoAttr.
     *
     *  -- This field is *not* serialized nor persisted; it is only used on the server side.
     */
    Node* parent_{nullptr}; // only ever used on the server side, to access parent Node variables

    /**
     * @brief The parent Node path of this AvisoAttr.
     */
    path_t parent_path_;

    /**
     * @brief The name of this AvisoAttr
     */
    name_t name_;

    /**
     * @brief The listener used to launch the Aviso listener
     *
     * This listener is the original configuration, and may contain variable placeholders.
     */
    listener_t listener_;

    /**
     * @brief The URL used to launch the Aviso listener
     *
     * This configuration parameter may contain variable placeholders.
     */
    url_t url_;

    /**
     * @brief The authentication token used to launch the Aviso listener
     *
     * This configuration parameter may contain variable placeholders.
     */
    auth_t auth_;

    // The following are mutable as they are modified by the const method isFree()

    /**
     * @brief A message buffer indicating, if any, the reason for the latest failure received from Aviso
     *
     * This field is empty if no error detected; otherwise contains a user facing message describing the error.
     **/
    mutable reason_t reason_{};

    /**
     * @brief The latest revision received from Aviso
     *
     * This is a 'marker' of the latest revision processed, and is used to avoid receiving repeated notifications.
     **/
    mutable revision_t revision_;

    /**
     * @brief The state change number, used to detect changes in the Aviso attribute
     *
     *  -- This field is *not* serialized nor persisted; it is only used on the server side.
     */
    mutable unsigned int state_change_no_{0};

    /**
     * @brief A 'cache' buffer, storing the fully configured (i.e. all variables substituted) Aviso listener
     *
     * This is the listener actually used to configure the controller.
     */
    mutable active_t active_;

    /**
     * @brief The backend, which is responsible for receiving the Aviso notifications in the background
     *
     * The backend is only instantiated between start() and finish() calls, and only when a backend is available
     * (i.e. on the server side, when built with Aviso support).
     * This allows AvisoAttr to have a copy-ctor and assignment operator.
     *
     *  -- This field is *not* serialized nor persisted; it is only used on the server side.
     **/
    mutable backend_ptr_t backend_;
};

bool operator==(const AvisoAttr& lhs, const AvisoAttr& rhs);

std::string to_python_string(const AvisoAttr& aviso);

template <class Archive>
void serialize(Archive& ar, AvisoAttr& aviso, [[maybe_unused]] std::uint32_t version) {
    // The Aviso v1 fields 'schema' and 'polling' are no longer used, but their (empty) placeholders are kept in the
    // archive, so that the archive remains compatible with ecFlow 5.19.x clients and servers
    std::string unused_schema;
    std::string unused_polling;

    ar & aviso.parent_path_;
    ar & aviso.name_;
    ar & aviso.listener_;
    ar & aviso.url_;
    ar & unused_schema;
    ar & unused_polling;
    ar & aviso.auth_;
    ar & aviso.reason_;
    ar & aviso.revision_;
    ar & aviso.active_;
}

} // namespace ecf
