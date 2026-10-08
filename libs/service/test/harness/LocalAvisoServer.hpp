// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace httplib {
class Server;
}

namespace ecf::test {

///
/// @brief Serves the part of the aviso-server protocol used by the Aviso client library, for tests.
///
/// The server answers `POST /api/v1/watch` with a stream of Server-Sent Events, following the wire format of
/// aviso-server 0.13.x:
///  - a watch without `from_id` is live: it opens with a `connection_established` control event, and receives the
///    notifications published after the request;
///  - a watch with `from_id` resumes: it replays the stored notifications from that sequence (inclusive) between
///    `replay_started` and `replay_completed`, and then goes live;
///  - a watch with `from_date` (RFC 3339, UTC) resumes likewise, from the notifications published at or after that
///    time.
///
/// The filter of a watch (`identifier`) is matched against the identifier of each notification, with identifier
/// values compared as strings, and `{"in": [...]}` accepted as a constraint. A filter naming a rejected field, or
/// holding an array, is refused with HTTP 400. When credentials are required, a request without the expected
/// `Authorization` header is refused with HTTP 401. Any other path is answered with HTTP 404, as an Aviso v1 server
/// would.
///
/// The server listens on 127.0.0.1, on a port reserved by the caller, and records every watch request.
///
class LocalAvisoServer {
public:
    ///
    /// @brief Holds a published notification.
    ///
    struct Notification
    {
        std::string event_type;
        std::uint64_t sequence;
        std::string identifier_json;
        std::string payload_json;
        std::chrono::system_clock::time_point published;
    };

    ///
    /// @brief Holds a watch request, as received.
    ///
    struct Request
    {
        std::string authorization;
        std::string body;
    };

    ///
    /// @brief Starts the server, and waits until it accepts connections.
    ///
    /// @param[in] port The port to listen on.
    /// @throws std::runtime_error if the server does not start.
    ///
    explicit LocalAvisoServer(int port);

    LocalAvisoServer(const LocalAvisoServer&)            = delete;
    LocalAvisoServer& operator=(const LocalAvisoServer&) = delete;

    ///
    /// @brief Stops the server, closing every open stream.
    ///
    ~LocalAvisoServer();

    ///
    /// @brief Returns the base URL of the server.
    ///
    /// @return The URL, of the form http://127.0.0.1:<port>.
    ///
    [[nodiscard]] std::string url() const;

    ///
    /// @brief Stores a notification, and streams it to every matching open watch.
    ///
    /// @param[in] event_type      The event type (e.g. test_event).
    /// @param[in] identifier_json The identifier, as a JSON object; values are stored as strings.
    /// @param[in] payload_json    The payload, as JSON.
    /// @return The sequence number given to the notification.
    ///
    std::uint64_t publish(const std::string& event_type,
                          const std::string& identifier_json,
                          const std::string& payload_json = "null");

    ///
    /// @brief Requires every watch request to carry the given Basic credentials.
    ///
    /// @param[in] username The user name expected.
    /// @param[in] password The password expected.
    ///
    void require_basic_auth(const std::string& username, const std::string& password);

    ///
    /// @brief Requires every watch request to carry the given bearer token.
    ///
    /// @param[in] token The token expected.
    ///
    void require_bearer_auth(const std::string& token);

    ///
    /// @brief Refuses, with HTTP 400, every filter naming the given field.
    ///
    /// @param[in] field The name of the identifier field to refuse.
    ///
    void reject_field(const std::string& field);

    ///
    /// @brief Closes every stream after the given duration, as aviso-server does after its connection lifetime.
    ///
    /// The stream ends with the routine reason `max_duration_reached`, after which the client library reconnects by
    /// itself, resuming after the last notification received.
    ///
    /// @param[in] lifetime The duration of each stream.
    ///
    void close_streams_after(std::chrono::milliseconds lifetime);

    ///
    /// @brief Ends every stream with an error event after the given duration.
    ///
    /// The client library does not recover from a stream error: the watch ends, and its owner is to re-create it.
    ///
    /// @param[in] lifetime The duration of each stream.
    ///
    void fail_streams_after(std::chrono::milliseconds lifetime);

    ///
    /// @brief Returns the watch requests received so far.
    ///
    /// @return The requests, in order of arrival.
    ///
    [[nodiscard]] std::vector<Request> requests() const;

    ///
    /// @brief Waits until at least the given number of watch requests has been received.
    ///
    /// @param[in] count   The number of requests to wait for, counted since the server started.
    /// @param[in] timeout The maximum time to wait.
    /// @return True when the requests were received before the timeout.
    ///
    bool wait_for_requests(std::size_t count, std::chrono::milliseconds timeout) const;

    ///
    /// @brief Waits until at least the given number of notifications has been written to the streams.
    ///
    /// @param[in] count   The number of notifications to wait for, counted over all streams since the server started.
    /// @param[in] timeout The maximum time to wait.
    /// @return True when the notifications were written before the timeout.
    ///
    bool wait_for_streamed(std::size_t count, std::chrono::milliseconds timeout) const;

    ///
    /// @brief Waits until at least the given number of streams has been ended by the server.
    ///
    /// A stream ends after the duration set by close_streams_after() or fail_streams_after().
    ///
    /// @param[in] count   The number of streams to wait for, counted since the server started.
    /// @param[in] timeout The maximum time to wait.
    /// @return True when the streams were ended before the timeout.
    ///
    bool wait_for_ended_streams(std::size_t count, std::chrono::milliseconds timeout) const;

private:
    struct Watch
    {
        std::string event_type;
        std::string filter_json;
    };

    [[nodiscard]] bool matches(const Notification& notification, const Watch& watch) const;
    [[nodiscard]] std::optional<std::string>
    check_request(const std::string& authorization,
                  const std::string& body,
                  Watch& watch,
                  std::optional<std::uint64_t>& from_id,
                  std::optional<std::chrono::system_clock::time_point>& from_date,
                  int& status) const;

    int port_;
    std::unique_ptr<httplib::Server> server_;
    std::thread thread_;

    mutable std::mutex mutex_;
    mutable std::condition_variable changed_;
    std::vector<Notification> notifications_;
    std::vector<Request> requests_;
    std::vector<std::string> rejected_fields_;
    std::string expected_authorization_;
    std::optional<std::chrono::milliseconds> stream_lifetime_;
    bool stream_fails_         = false; // whether a stream ends with an error (true) or a routine close (false)
    std::size_t streamed_      = 0;     // notifications written to the streams
    std::size_t ended_streams_ = 0;     // streams ended by the server
    std::atomic<bool> stopping_{false};
};

} // namespace ecf::test
