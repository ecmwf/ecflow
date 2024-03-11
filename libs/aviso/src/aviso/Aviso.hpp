/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef aviso_Aviso_HPP
#define aviso_Aviso_HPP

#include <atomic>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

namespace aviso {

class Address {
public:
    explicit Address(std::string_view scheme_host_port) : scheme_host_port_(scheme_host_port) {}

    bool operator==(const Address& other) const { return scheme_host_port_ == other.scheme_host_port_; }
    bool operator!=(const Address& other) const { return !(*this == other); }

    std::string_view address() const { return scheme_host_port_; }

    friend std::hash<Address>;

private:
    std::string scheme_host_port_;
};

} // namespace aviso

namespace std {
template <>
struct hash<aviso::Address>
{
    size_t operator()(const aviso::Address& addr) const noexcept {
        // The hashing mechanism is std::hash<std::string>(), change it according to your requirement
        return std::hash<std::string>()(addr.scheme_host_port_);
    }
};
} // namespace std

namespace aviso {

struct Raw;
struct Base64;

class Content {
public:
    using content_t      = std::string;
    using content_view_t = std::string_view;

    const content_t& raw() const { return raw_; }
    content_t base64() const { return encode_base64(raw_); }

    template <typename tag>
    friend Content make_from(Content::content_view_t content);

private:
    static std::string decode_base64(const std::string& val);
    static std::string decode_base64(std::string_view val);

    static std::string encode_base64(const std::string& val);
    static std::string encode_base64(std::string_view val);

private:
    explicit Content(content_view_t raw) : raw_{raw} {}

    content_t raw_;
};

template <typename tag>
inline Content make_from(Content::content_view_t content);

template <>
inline Content make_from<Raw>(Content::content_view_t content) {
    return Content(content);
}

template <>
inline Content make_from<Base64>(Content::content_view_t content) {
    auto raw = Content::decode_base64(content);
    return make_from<Raw>(raw);
}

template <typename tag>
inline Content make_from(const Content::content_t& content);

template <>
inline Content make_from<Raw>(const Content::content_t& content) {
    return make_from<Raw>(std::string_view{content});
}

template <>
inline Content make_from<Base64>(const Content::content_t& content) {
    return make_from<Base64>(std::string_view{content});
}

class Range {
public:
    using key_t      = std::string;
    using key_view_t = std::string_view;

    explicit Range(key_view_t key)
        : range_bgn(make_from<Raw>(key)),
          range_end(make_from<Raw>(increment_last_byte(range_bgn.raw()))) {}

    key_t base64_begin() const { return range_bgn.base64(); }
    key_t base64_end() const { return range_end.base64(); }

private:
    static key_t increment_last_byte(key_t val);

private:
    Content range_bgn;
    Content range_end;
};

class Listener {
public:
    Listener() = default;
    Listener(std::string_view name, std::string_view base, std::string_view stem)
        : name_{name},
          base_{base},
          stem_{stem} {}

    std::string_view name() const { return name_; }
    std::string_view base() const { return base_; }
    std::string_view stem() const { return stem_; }

    std::string full() const { return base_ + '/' + stem_; }

private:
    std::string name_{};
    std::string base_{};
    std::string stem_{};
};

class Notification {
public:
    Notification() = default;
    Notification(std::string_view key, std::string_view value) : key_{key}, value_{value} {}

    std::string_view key() const { return key_; }
    std::string_view value() const { return value_; }

    void add_parameter(const std::string& parameter, const std::string& value) {
        parameters_.emplace_back(parameter, value);
    }
    std::vector<std::pair<std::string, std::string>> parameters() const { return parameters_; }

private:
    std::string key_{};
    std::string value_{};
    std::vector<std::pair<std::string, std::string>> parameters_{};
};

std::ostream& operator<<(std::ostream& os, const Notification& notification);

class ConfiguredListener : private Listener {
public:
    ConfiguredListener(Address address,
                       std::string_view path,
                       std::string_view name,
                       std::string_view base,
                       std::string_view stem)
        : Listener(name, base, stem),
          path_{path},
          address_(std::move(address)),
          resolved_base_(base) {}

    void with_parameter(const std::string& parameter, const std::string& value);
    void with_parameter(const std::string& parameter, int64_t value);
    void with_parameter(const std::string& parameter, const std::vector<std::string>& value);

    std::string path() const { return path_; }
    const Address& address() const { return address_; }

    using Listener::base;
    using Listener::name;
    using Listener::stem;

    using Listener::full;

    std::string_view resolved_base() const { return resolved_base_; }

    std::string prefix() const { return resolved_base_ + '/'; }

    std::optional<Notification> accepts(const std::string& key, const std::string& value) const;

private:
    std::string path_;
    Address address_;
    std::string resolved_base_;

    using parameters_t = std::variant<std::string, std::int64_t, std::vector<std::string>>;
    std::unordered_map<std::string, parameters_t> parameters_ = {};
};

class ListenerSchema {
public:
    inline void add_listener(const Listener& listener) {
        auto name        = std::string(listener.name());
        listeners_[name] = listener;
    };
    inline std::optional<Listener> get_listener(const std::string& name) const {
        if (auto found = listeners_.find(name); found != listeners_.end()) {
            return found->second;
        }
        return std::nullopt;
    }

private:
    std::unordered_map<std::string, Listener> listeners_{};
};

ListenerSchema load_listener_schema(const std::string& schema_location);

ConfiguredListener
create_configured_listener(const std::string& cfg, const ListenerSchema& schema);

class EtcdClient {
public:
    EtcdClient() = default;
    EtcdClient(const Address& address);

    EtcdClient(const EtcdClient&) = delete;
    EtcdClient(EtcdClient&&)      = delete;

    ~EtcdClient();

    std::vector<std::pair<std::string, std::string>> poll(std::string_view key_prefix, int64_t revision);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    inline static const std::string endpoint_path = "/v3/kv/range";
};

class Timer {
public:
    explicit Timer() : running_{false}, worker_{} {}
    Timer(const Timer&)            = delete;
    Timer(Timer&&)                 = delete;
    Timer& operator=(const Timer&) = delete;
    Timer& operator=(Timer&&)      = delete;
    ~Timer() { stop(); }

    void stop() {
        if (running_) {
            running_ = false;
            worker_.join();
        }
    }

    template <typename CALLBACK>
    void start(CALLBACK& callback, std::chrono::seconds expiry) {
        running_ = true;
        worker_  = std::thread([expiry, &callback, this]() {
            while (running_) {
                callback();
                std::this_thread::sleep_for(expiry);
            }
        });
    }

private:
    std::atomic<bool> running_;
    std::thread worker_;
};

template <typename CALLBACK>
class Listen {
public:
private:
};

template <typename CALLBACK>
class ListenService {
public:
    using listener_t     = aviso::ConfiguredListener;
    using address_t      = Address;
    using key_prefix_t   = std::string;
    using listener_set_t = std::vector<listener_t>;
    using storage_t      = std::unordered_map<address_t, std::unordered_map<key_prefix_t, listener_set_t>>;
    using callback_t     = CALLBACK;
    using revision_t     = int64_t;

    ListenService(CALLBACK callback) : timer_{}, listeners_{}, callback_{callback}, latest_revision_{0} {}
    ListenService()                     = delete;
    ListenService(const ListenService&) = delete;
    ~ListenService() { stop(); }

    ListenService& operator=(const ListenService&) = delete;

    void start(std::chrono::seconds expiry) { timer_.start(*this, expiry); }

    void stop() { timer_.stop(); }

    void register_listener(const listener_t& listener) {
        auto address    = listener.address();
        auto key_prefix = listener.prefix();
        listeners_[address][key_prefix].push_back(listener);
    }

    void operator()() {

        int64_t highest_revision = 0;

        // For each host(+port)
        for (auto&& [address, prefix_listerners] : listeners_) {
            EtcdClient client{address};
            // For each key prefix
            for (auto&& [key_prefix, registered_listeners] : prefix_listerners) {
                // Poll notifications on the key prefix
                auto updated_keys = client.poll(key_prefix, latest_revision_ + 1);
                // Pass updated keys to the listeners
                for (auto&& [key, value] : updated_keys) {
                    if (key == "latest_revision") {
                        highest_revision = std::max(static_cast<int64_t>(std::stoll(value)), highest_revision);
                        continue;
                    }
                    // For each listener
                    for (auto&& listener : registered_listeners) {
                        // Accept the notification, if the listener is interested in the key/value
                        if (auto notification = listener.accepts(key, value); notification) {
                            callback_(listener, *notification);
                        }
                    }
                }
            }
        }

        // Ensure lastest revisision is up-to-date
        latest_revision_ = std::max(latest_revision_, highest_revision);
    }

private:
    aviso::Timer timer_;
    storage_t listeners_;
    callback_t callback_;
    revision_t latest_revision_;
};

} // namespace aviso

#endif /* aviso_Aviso_HPP */
