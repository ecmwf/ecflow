// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/service/aviso/BaseAvisoBackend.hpp"

namespace cereal {
class access;
}

class Node;

namespace ecf {

///
/// @brief AvisoEvent describes the Aviso notification that released a node.
///
/// The event is exposed to the job as generated variables, and is persisted (in checkpoints, as option --event) so
/// that the variables survive a server restart.
///
struct AvisoEvent
{
    using sequence_t = std::uint64_t;

    /// The event type of the notification (e.g. mars).
    std::string type;
    /// The sequence number of the notification; 0 when no notification has released the node.
    sequence_t sequence{0};
    /// The identifier of the notification, as JSON.
    std::string identifier;
    /// The payload of the notification, as JSON.
    std::string payload;

    ///
    /// @brief Creates the event describing the given notification.
    ///
    /// @param[in] notification The notification that released the node.
    /// @return The event.
    ///
    static AvisoEvent from(const ecf::service::aviso::AvisoNotification& notification);

    ///
    /// @brief Creates the event from the value of option --event.
    ///
    /// @param[in] option The value, with or without the surrounding single quotes.
    /// @return The event.
    /// @throws std::runtime_error if the value is not a valid event.
    ///
    static AvisoEvent from_option(const std::string& option);

    ///
    /// @brief Returns whether no notification is described (i.e. the sequence is 0).
    ///
    /// @return True when no notification is described.
    ///
    [[nodiscard]] bool empty() const { return sequence == 0; }

    ///
    /// @brief Returns the event as the single-quoted value of option --event.
    ///
    /// The value is a JSON object (type, sequence, identifier, payload), in which single quotes are escaped (as
    /// `\u0027`).
    ///
    /// @return The single-quoted value.
    ///
    [[nodiscard]] std::string to_option() const;
};

bool operator==(const AvisoEvent& lhs, const AvisoEvent& rhs);

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

    using backend_t     = ecf::service::aviso::BaseAvisoBackend;
    using backend_ptr_t = std::shared_ptr<backend_t>;

    static constexpr const char* default_url  = "%ECF_AVISO_URL%";
    static constexpr const char* default_auth = "%ECF_AVISO_AUTH%";

    static constexpr const char* reload_option_value = "reload";

    static bool is_valid_name(const std::string& name);

    ///
    /// @brief Creates an invalid Aviso attribute.
    ///
    /// @note Required by Cereal serialization, which invokes the default constructor to create the object and only
    ///       then proceeds to member-wise serialization.
    ///
    AvisoAttr() = default;
    AvisoAttr(Node* parent,
              name_t name,
              const listener_t& listener,
              url_t url,
              revision_t revision,
              auth_t auth,
              const reason_t& reason,
              bool collapse = false);
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

    ///
    /// @brief Returns whether a release consumes all the notifications received, or exactly one.
    ///
    /// @return True when a release consumes all the notifications received, false when it consumes exactly one.
    ///
    [[nodiscard]] inline bool collapse() const { return collapse_; }

    static constexpr const char* genvar_event_type            = "ECF_AVISO_EVENT_TYPE";
    static constexpr const char* genvar_event_sequence        = "ECF_AVISO_EVENT_SEQUENCE";
    static constexpr const char* genvar_event_data_identifier = "ECF_AVISO_EVENT_DATA_IDENTIFIER";
    static constexpr const char* genvar_event_data_payload    = "ECF_AVISO_EVENT_DATA_PAYLOAD";

    ///
    /// @brief Returns the notification that released the node.
    ///
    /// @return The notification, empty until a notification releases the node.
    ///
    [[nodiscard]] inline const AvisoEvent& event() const { return event_; }

    ///
    /// @brief Sets the notification that released the node.
    ///
    /// @param[in] event The event describing the notification.
    ///
    void set_event(const AvisoEvent& event);

    ///
    /// @brief Appends the variables generated from the notification that released the node.
    ///
    /// The variables describe the notification held to release the node, as soon as it is held (so that the job,
    /// generated while the node is still queued, sees it), or otherwise the last one committed. The variables are
    /// always defined; they are empty (and the sequence is 0) until a notification releases the node.
    ///
    /// @param[out] vars The variables to append to.
    ///
    void gen_variables(std::vector<Variable>& vars) const;

    ///
    /// @brief Returns the generated variable with the given name.
    ///
    /// @param[in] name The name of the generated variable (e.g. ECF_AVISO_EVENT_TYPE).
    /// @return The variable, or an empty variable when there is none with the given name.
    ///
    [[nodiscard]] const Variable& find_gen_variable(const std::string& name) const;
    [[nodiscard]] inline const active_t& active() const { return active_; }
    [[nodiscard]] path_t path() const;

    ///
    /// @brief Returns whether the attribute currently holds an error.
    ///
    /// @return True when a failure reason is recorded.
    ///
    [[nodiscard]] bool has_error() const;

    unsigned int state_change_no() const { return state_change_no_; }

    bool why(std::string& theReasonWhy) const;

    ///
    /// @brief Starts receiving notifications in the background, when the node is queued (see start()).
    ///
    /// Called when the node is reset (e.g. on begin or requeue).
    ///
    void reset();

    ///
    /// @brief Stops and starts again receiving notifications, so that the configuration is evaluated again.
    ///
    /// Called by the Alter command (value `reload`), typically after changing the variables that configure the
    /// attribute; does nothing unless the attribute is running, or its node is queued.
    ///
    void reload();

    [[nodiscard]] bool isFree() const;

    void start() const;
    void finish() const;

    ///
    /// @brief Consumes the notification held to release the node, advancing the revision to it.
    ///
    /// Called when the node leaves the queued state; does nothing when no notification is held.
    ///
    void commit() const;

    template <class Archive>
    friend void serialize(Archive& ar, AvisoAttr& aviso, std::uint32_t version);

    ///
    /// @brief Finishes all the given Aviso attributes, stopping the background notifications.
    ///
    /// @param[in] avisos The attributes to finish.
    ///
    static void finish(const std::vector<AvisoAttr>& avisos);

    ///
    /// @brief Informs the Aviso attributes of a state change of their node.
    ///
    /// When the node becomes queued (e.g. forced by the Force command), the attributes are started (see start()).
    /// When the node leaves the queued state, the notification held by each attribute is consumed (see commit()).
    /// When the new state is a Task "terminal" state (i.e. complete, aborted, unknown), the attributes are finished,
    /// stopping the background notifications.
    ///
    /// @param[in] avisos The attributes of the node.
    /// @param[in] state  The new state of the node.
    ///
    static void state_changed(const std::vector<AvisoAttr>& avisos, NState::State state);

private:
    void start_backend(const std::string& aviso_path,
                       const std::string& aviso_listener,
                       const std::string& aviso_url,
                       const std::string& aviso_auth) const;
    void stop_backend() const;

    void set_error(const std::string& reason) const;
    void clear_error() const;

    ///
    /// @brief The parent Node of this AvisoAttr.
    ///
    ///  -- This field is *not* serialized nor persisted; it is only used on the server side.
    ///
    Node* parent_{nullptr}; // only ever used on the server side, to access parent Node variables

    ///
    /// @brief The parent Node path of this AvisoAttr.
    ///
    path_t parent_path_;

    ///
    /// @brief The name of this AvisoAttr
    ///
    name_t name_;

    ///
    /// @brief The listener used to launch the Aviso listener
    ///
    /// This listener is the original configuration, and may contain variable placeholders.
    ///
    listener_t listener_;

    ///
    /// @brief The URL used to launch the Aviso listener
    ///
    /// This configuration parameter may contain variable placeholders.
    ///
    url_t url_;

    ///
    /// @brief The path to the credentials file used to contact the Aviso server.
    ///
    /// This configuration parameter may contain variable placeholders.
    ///
    auth_t auth_;

    ///
    /// @brief Whether a release consumes all the notifications received, or exactly one (the default).
    ///
    bool collapse_{false};

    ///
    /// @brief The notification that released the node.
    ///
    /// Set when the node is released (see commit()); persisted, so that the generated variables survive a restart.
    ///
    mutable AvisoEvent event_;

    ///
    /// @brief The generated variables, refreshed from the held or committed notification when accessed.
    ///
    /// These fields are not serialised nor persisted.
    ///
    mutable Variable genvar_event_type_{genvar_event_type, ""};
    mutable Variable genvar_event_sequence_{genvar_event_sequence, "0"};
    mutable Variable genvar_event_data_identifier_{genvar_event_data_identifier, ""};
    mutable Variable genvar_event_data_payload_{genvar_event_data_payload, ""};

    void update_gen_variables() const;

    // The following are mutable as they are modified by const methods (e.g. isFree(), start(), commit())

    ///
    /// @brief A message buffer indicating, if any, the reason for the latest failure received from Aviso
    ///
    /// This field is empty if no error detected; otherwise contains a user facing message describing the error.
    ///
    mutable reason_t reason_{};

    ///
    /// @brief The sequence of the last notification committed (i.e. that released the node).
    ///
    /// The next watch resumes after it, and notifications up to it are ignored as already consumed.
    ///
    mutable revision_t revision_{0};

    ///
    /// @brief The state change number, used to detect changes in the Aviso attribute
    ///
    ///  -- This field is *not* serialized nor persisted; it is only used on the server side.
    ///
    mutable unsigned int state_change_no_{0};

    ///
    /// @brief A 'cache' buffer, storing the fully configured (i.e. all variables substituted) Aviso listener
    ///
    /// This is the listener actually used to configure the controller.
    ///
    mutable active_t active_;

    ///
    /// @brief The backend, which is responsible for receiving the Aviso notifications in the background.
    ///
    /// The backend is only instantiated between start() and finish() calls, and only when a backend is available
    /// (i.e. on the server side, when built with Aviso support). A copy shares the backend (see make_detached()).
    ///
    /// This field is not serialised nor persisted; it is only used on the server side.
    ///
    mutable backend_ptr_t backend_;

    ///
    /// @brief The notifications received, and not yet consumed (sorted by sequence when consumed).
    ///
    /// This field is not serialised nor persisted; the Aviso server redelivers them after the revision.
    ///
    mutable std::vector<ecf::service::aviso::AvisoNotification> queued_;

    ///
    /// @brief The notification held to release the node, until the node leaves the queued state (see commit()).
    ///
    /// Holding it makes isFree() repeatable: the node is released by this notification however many times isFree()
    /// is called (e.g. by a query of why the node is held, or while another dependency holds the node).
    ///
    /// This field is not serialised nor persisted.
    ///
    mutable std::optional<ecf::service::aviso::AvisoNotification> pending_;
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
    if (version >= 1) {
        ar & aviso.collapse_;
        ar & aviso.event_.type;
        ar & aviso.event_.sequence;
        ar & aviso.event_.identifier;
        ar & aviso.event_.payload;
    }
}

} // namespace ecf

CEREAL_CLASS_VERSION(ecf::AvisoAttr, 1)
