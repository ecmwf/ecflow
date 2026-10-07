// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "LocalAvisoServer.hpp"

#include <algorithm>
#include <ctime>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "ecflow/core/HttpLibrary.hpp"

namespace ecf::test {

namespace {

using json = nlohmann::ordered_json;

std::string now_iso8601() {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
    gmtime_r(&now, &utc);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buffer;
}

std::string frame(const std::string& event, const json& data) {
    return "event: " + event + "\ndata: " + data.dump() + "\n\n";
}

std::string as_string(const json& value) {
    return value.is_string() ? value.get<std::string>() : value.dump();
}

json cloud_event(const LocalAvisoServer::Notification& notification, const std::string& source) {
    return json{{"data",
                 {{"identifier", json::parse(notification.identifier_json)},
                  {"payload", json::parse(notification.payload_json)}}},
                {"datacontenttype", "application/json"},
                {"id", notification.event_type + "@" + std::to_string(notification.sequence)},
                {"source", source},
                {"specversion", "1.0"},
                {"time", now_iso8601()},
                {"type", "int.ecmwf.aviso." + notification.event_type}};
}

} // namespace

LocalAvisoServer::LocalAvisoServer(int port)
    : port_{port},
      server_{std::make_unique<httplib::Server>()} {

    server_->Post("/api/v1/watch", [this](const httplib::Request& request, httplib::Response& response) {
        auto authorization = request.get_header_value("Authorization");

        Watch watch;
        std::optional<std::uint64_t> from_id;
        std::optional<std::chrono::system_clock::time_point> from_date;
        int status = 200;
        if (auto error = check_request(authorization, request.body, watch, from_id, from_date, status); error) {
            response.status = status;
            response.set_content(json{{"error", *error}}.dump(), "application/json");
            return;
        }

        // The position from which notifications are streamed is anchored at request time, so that a notification
        // published as soon as the request is received is not missed
        std::uint64_t last = 0;
        {
            std::scoped_lock lock(mutex_);
            requests_.push_back(Request{authorization, request.body});
            if (from_id) {
                last = *from_id > 0 ? *from_id - 1 : 0;
            }
            else if (from_date) {
                // Resume after the last notification published before the given date
                for (const auto& n : notifications_) {
                    if (n.published < *from_date) {
                        last = std::max(last, n.sequence);
                    }
                }
            }
            else {
                for (const auto& n : notifications_) {
                    last = std::max(last, n.sequence);
                }
            }
        }
        changed_.notify_all();

        struct Stream
        {
            Watch watch;
            bool resume;
            std::uint64_t last;
            bool opened = false;
            std::chrono::steady_clock::time_point started;
        };
        auto stream     = std::make_shared<Stream>(Stream{watch, from_id.has_value() || from_date.has_value(), last});
        stream->started = std::chrono::steady_clock::now();

        response.set_chunked_content_provider(
            "text/event-stream", [this, stream](size_t /*offset*/, httplib::DataSink& sink) {
                std::string topic = stream->watch.event_type;
                std::string out;
                std::size_t notifications_out = 0;

                std::unique_lock lock(mutex_);

                auto pending = [this, &stream]() {
                    std::vector<Notification> found;
                    for (const auto& n : notifications_) {
                        if (n.sequence > stream->last && matches(n, stream->watch)) {
                            found.push_back(n);
                        }
                    }
                    return found;
                };

                if (!stream->opened) {
                    stream->opened = true;
                    if (stream->resume) {
                        out += frame("replay-control",
                                     json{{"from_sequence", stream->last + 1},
                                          {"timestamp", now_iso8601()},
                                          {"topic", topic},
                                          {"type", "replay_started"}});
                        for (const auto& n : pending()) {
                            out += frame("replay", cloud_event(n, url()));
                            stream->last = std::max(stream->last, n.sequence);
                            ++notifications_out;
                        }
                        out +=
                            frame("replay-control",
                                  json{{"timestamp", now_iso8601()}, {"topic", topic}, {"type", "replay_completed"}});
                    }
                    else {
                        out += frame("live-notification",
                                     json{{"connection_will_close_in_seconds", 120},
                                          {"timestamp", now_iso8601()},
                                          {"topic", topic},
                                          {"type", "connection_established"}});
                    }
                }
                else {
                    auto found = pending();
                    if (found.empty()) {
                        changed_.wait_for(lock, std::chrono::milliseconds{200});
                        found = pending();
                    }
                    for (const auto& n : found) {
                        out += frame("live-notification", cloud_event(n, url()));
                        stream->last = std::max(stream->last, n.sequence);
                        ++notifications_out;
                    }
                    if (found.empty()) {
                        out += frame("heartbeat", json{{"timestamp", now_iso8601()}, {"topic", topic}});
                    }
                }

                bool expired =
                    stream_lifetime_ && std::chrono::steady_clock::now() - stream->started >= *stream_lifetime_;
                bool fails = stream_fails_;
                lock.unlock();

                if (expired && fails) {
                    out +=
                        frame("error",
                              json{{"error", "stream_processing_failed"}, {"message", "stream failed (test server)"}});
                }
                else if (expired) {
                    out += frame("connection-closing",
                                 json{{"reason", "max_duration_reached"}, {"timestamp", now_iso8601()}});
                }

                if (!out.empty() && !sink.write(out.data(), out.size())) {
                    return false;
                }
                if (expired || stopping_) {
                    sink.done();
                }

                // Record what was written, for the tests waiting on it
                if (notifications_out > 0 || expired) {
                    {
                        std::scoped_lock written(mutex_);
                        streamed_ += notifications_out;
                        ended_streams_ += expired ? 1 : 0;
                    }
                    changed_.notify_all();
                }
                return true;
            });
    });

    thread_ = std::thread([this]() { server_->listen("127.0.0.1", port_); });

    for (int i = 0; i < 100 && !server_->is_running(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
    }
    if (!server_->is_running()) {
        stopping_ = true;
        server_->stop();
        thread_.join();
        throw std::runtime_error("LocalAvisoServer: unable to listen on port " + std::to_string(port_));
    }
}

LocalAvisoServer::~LocalAvisoServer() {
    stopping_ = true;
    changed_.notify_all();
    server_->stop();
    if (thread_.joinable()) {
        thread_.join();
    }
}

std::string LocalAvisoServer::url() const {
    return "http://127.0.0.1:" + std::to_string(port_);
}

std::uint64_t LocalAvisoServer::publish(const std::string& event_type,
                                        const std::string& identifier_json,
                                        const std::string& payload_json) {
    // Identifier values are stored as strings, as aviso-server does
    json identifier = json::object();
    json given      = json::parse(identifier_json);
    for (const auto& [name, value] : given.items()) {
        identifier[name] = as_string(value);
    }

    std::uint64_t sequence = 1;
    {
        std::scoped_lock lock(mutex_);
        for (const auto& n : notifications_) {
            if (n.event_type == event_type) {
                sequence = std::max(sequence, n.sequence + 1);
            }
        }
        notifications_.push_back(Notification{event_type,
                                              sequence,
                                              identifier.dump(),
                                              json::parse(payload_json).dump(),
                                              std::chrono::system_clock::now()});
    }
    changed_.notify_all();
    return sequence;
}

void LocalAvisoServer::require_basic_auth(const std::string& username, const std::string& password) {
    std::scoped_lock lock(mutex_);
    expected_authorization_ = "Basic " + httplib::detail::base64_encode(username + ":" + password);
}

void LocalAvisoServer::require_bearer_auth(const std::string& token) {
    std::scoped_lock lock(mutex_);
    expected_authorization_ = "Bearer " + token;
}

void LocalAvisoServer::reject_field(const std::string& field) {
    std::scoped_lock lock(mutex_);
    rejected_fields_.push_back(field);
}

void LocalAvisoServer::close_streams_after(std::chrono::milliseconds lifetime) {
    std::scoped_lock lock(mutex_);
    stream_lifetime_ = lifetime;
    stream_fails_    = false;
}

void LocalAvisoServer::fail_streams_after(std::chrono::milliseconds lifetime) {
    std::scoped_lock lock(mutex_);
    stream_lifetime_ = lifetime;
    stream_fails_    = true;
}

std::vector<LocalAvisoServer::Request> LocalAvisoServer::requests() const {
    std::scoped_lock lock(mutex_);
    return requests_;
}

bool LocalAvisoServer::wait_for_requests(std::size_t count, std::chrono::milliseconds timeout) const {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout, [this, count]() { return requests_.size() >= count; });
}

bool LocalAvisoServer::wait_for_streamed(std::size_t count, std::chrono::milliseconds timeout) const {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout, [this, count]() { return streamed_ >= count; });
}

bool LocalAvisoServer::wait_for_ended_streams(std::size_t count, std::chrono::milliseconds timeout) const {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout, [this, count]() { return ended_streams_ >= count; });
}

bool LocalAvisoServer::matches(const Notification& notification, const Watch& watch) const {
    if (notification.event_type != watch.event_type) {
        return false;
    }
    if (watch.filter_json.empty()) {
        return true;
    }

    auto identifier = json::parse(notification.identifier_json);
    auto filter     = json::parse(watch.filter_json);
    for (const auto& [name, constraint] : filter.items()) {
        auto found = identifier.find(name);
        if (found == identifier.end()) {
            return false;
        }
        auto value = found->get<std::string>();
        if (constraint.is_object() && constraint.contains("in")) {
            const auto& candidates = constraint["in"];
            if (std::none_of(candidates.begin(), candidates.end(), [&value](const json& candidate) {
                    return as_string(candidate) == value;
                })) {
                return false;
            }
        }
        else if (as_string(constraint) != value) {
            return false;
        }
    }
    return true;
}

std::optional<std::string>
LocalAvisoServer::check_request(const std::string& authorization,
                                const std::string& body,
                                Watch& watch,
                                std::optional<std::uint64_t>& from_id,
                                std::optional<std::chrono::system_clock::time_point>& from_date,
                                int& status) const {
    std::scoped_lock lock(mutex_);

    if (!expected_authorization_.empty() && authorization != expected_authorization_) {
        status = 401;
        return "Unauthorized";
    }

    json content;
    try {
        content = json::parse(body);
    }
    catch (const json::parse_error&) {
        status = 400;
        return "Invalid JSON body";
    }

    if (!content.contains("event_type") || !content["event_type"].is_string()) {
        status = 400;
        return "Missing event_type";
    }
    watch.event_type = content["event_type"].get<std::string>();

    if (auto identifier = content.find("identifier"); identifier != content.end() && !identifier->is_null()) {
        for (const auto& [name, value] : identifier->items()) {
            if (std::find(rejected_fields_.begin(), rejected_fields_.end(), name) != rejected_fields_.end()) {
                status = 400;
                return "Unknown field '" + name + "' for event type '" + watch.event_type + "'";
            }
            if (value.is_array()) {
                status = 400;
                return "Field '" + name + "' must be a scalar value or constraint object, got array";
            }
        }
        watch.filter_json = identifier->dump();
    }

    if (auto id = content.find("from_id"); id != content.end() && !id->is_null()) {
        try {
            from_id = id->is_string() ? std::stoull(id->get<std::string>()) : id->get<std::uint64_t>();
        }
        catch (const std::exception&) {
            status = 400;
            return "Invalid from_id";
        }
    }

    if (auto date = content.find("from_date"); date != content.end() && !date->is_null()) {
        std::tm utc{};
        auto text = date->is_string() ? date->get<std::string>() : std::string{};
        if (strptime(text.c_str(), "%Y-%m-%dT%H:%M:%SZ", &utc) == nullptr) {
            status = 400;
            return "from_date must be a valid datetime/timestamp";
        }
        from_date = std::chrono::system_clock::from_time_t(timegm(&utc));
    }

    return std::nullopt;
}

} // namespace ecf::test
