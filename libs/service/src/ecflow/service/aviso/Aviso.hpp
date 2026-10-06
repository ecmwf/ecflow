// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <variant>

namespace ecf::service::aviso {

///
/// @brief Names the incompatibility with Aviso v1, and the last ecFlow release that supports it.
///
/// The text is appended to every error caused by an Aviso v1 configuration, so that users of Aviso v1 learn which
/// ecFlow release to use instead.
///
inline constexpr std::string_view unsupported_v1 =
    "ecFlow 5.20.0 supports Aviso v2 only; ecFlow 5.19.x is the last release that supports Aviso v1";

///
/// @brief Requests notifications for one Aviso attribute.
///
/// All values are fully resolved, i.e. ecFlow variables have already been substituted.
///
class AvisoSubscribe {
public:
    ///
    /// @brief Creates a subscription request.
    ///
    /// @param path     Unique identifier of the Aviso attribute (the node path, followed by the attribute name).
    /// @param listener Listener configuration, as JSON (event and request).
    /// @param url      Address of the Aviso server.
    /// @param revision Last notification already processed; 0 means that only new notifications are requested.
    /// @param auth     Path to the credentials file.
    ///
    AvisoSubscribe(std::string_view path,
                   std::string_view listener,
                   std::string_view url,
                   std::uint64_t revision,
                   std::string_view auth)
        : path_{path},
          listener_{listener},
          url_{url},
          revision_{revision},
          auth_{auth} {}

    [[nodiscard]] const std::string& path() const { return path_; }
    [[nodiscard]] const std::string& listener() const { return listener_; }
    [[nodiscard]] const std::string& url() const { return url_; }
    [[nodiscard]] std::uint64_t revision() const { return revision_; }
    [[nodiscard]] const std::string& auth() const { return auth_; }

    friend std::ostream& operator<<(std::ostream& os, const AvisoSubscribe& request);

private:
    std::string path_;
    std::string listener_;
    std::string url_;
    std::uint64_t revision_;
    std::string auth_;
};

///
/// @brief Holds one notification received for an Aviso attribute.
///
class AvisoNotification {
public:
    ///
    /// @brief Creates a notification.
    ///
    /// @param event_type      Event type of the notification (e.g. mars).
    /// @param sequence        Sequence number of the notification, strictly increasing within its event type.
    /// @param identifier_json Identifier of the notification, as JSON.
    /// @param payload_json    Payload of the notification, as JSON.
    ///
    AvisoNotification(std::string_view event_type,
                      std::uint64_t sequence,
                      std::string_view identifier_json,
                      std::string_view payload_json)
        : event_type_{event_type},
          sequence_{sequence},
          identifier_json_{identifier_json},
          payload_json_{payload_json} {}

    [[nodiscard]] const std::string& event_type() const { return event_type_; }
    [[nodiscard]] std::uint64_t sequence() const { return sequence_; }
    [[nodiscard]] const std::string& identifier_json() const { return identifier_json_; }
    [[nodiscard]] const std::string& payload_json() const { return payload_json_; }

    friend std::ostream& operator<<(std::ostream& os, const AvisoNotification& notification);

private:
    std::string event_type_;
    std::uint64_t sequence_;
    std::string identifier_json_;
    std::string payload_json_;
};

///
/// @brief Holds an error that prevents an Aviso attribute from receiving notifications.
///
class AvisoError {
public:
    ///
    /// @brief Creates an error.
    ///
    /// @param reason User-facing description of the error.
    ///
    explicit AvisoError(std::string_view reason)
        : reason_{reason} {}

    [[nodiscard]] const std::string& reason() const { return reason_; }

    friend std::ostream& operator<<(std::ostream& os, const AvisoError& error);

private:
    std::string reason_;
};

///
/// @brief Signals that the watch of an Aviso attribute was (re)created, clearing any previous error.
///
class AvisoWatchStarted {
public:
    friend std::ostream& operator<<(std::ostream& os, const AvisoWatchStarted& started);
};

///
/// @brief Represents one outcome delivered to an Aviso attribute: a notification, an error, or a (re)started watch.
///
using AvisoResponse = std::variant<AvisoNotification, AvisoError, AvisoWatchStarted>;

std::ostream& operator<<(std::ostream& os, const AvisoResponse& response);

} // namespace ecf::service::aviso
