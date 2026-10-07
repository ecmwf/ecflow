// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/service/aviso/v2/AvisoV2Backend.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <ctime>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#include <aviso_ffi/aviso.hpp>

#include "ecflow/core/Overload.hpp"
#include "ecflow/service/Log.hpp"
#include "ecflow/service/Registry.hpp"
#include "ecflow/service/aviso/v2/AvisoV2.hpp"

namespace ecf::service::aviso::v2 {

namespace {

const char* to_string(AvisoErrorKind kind) {
    switch (kind) {
        case AvisoErrorKind_Transport:
            return "transport";
        case AvisoErrorKind_Http:
            return "http";
        case AvisoErrorKind_Auth:
            return "auth";
        case AvisoErrorKind_Decode:
            return "decode";
        case AvisoErrorKind_MalformedEvent:
            return "malformed event";
        case AvisoErrorKind_HistoryGap:
            return "history gap";
        case AvisoErrorKind_StreamProtocol:
            return "stream protocol";
        case AvisoErrorKind_Config:
            return "config";
        case AvisoErrorKind_StateStore:
            return "state store";
        case AvisoErrorKind_Trigger:
            return "trigger";
        case AvisoErrorKind_InvalidInput:
            return "invalid input";
        case AvisoErrorKind_InvalidUsage:
            return "invalid usage";
        case AvisoErrorKind_Internal:
            return "internal";
        case AvisoErrorKind_Panic:
            return "panic";
        default:
            return "unknown";
    }
}

std::string describe(const ::aviso::ErrorInfo& error) {
    return describe_error(to_string(error.kind), error.http_status, error.message, error.request_id);
}

///
/// @brief Runs delayed tasks on a single background thread, shared by all backends.
///
/// The tasks re-create watches, which involves waiting for the library to release the previous watch; this must
/// never happen on a library thread, nor delay the server's main thread.
///
class RetryTimer {
public:
    using clock_type = std::chrono::steady_clock;
    using task_type  = std::function<void()>;

    static RetryTimer& instance() {
        static RetryTimer timer;
        return timer;
    }

    void schedule(clock_type::duration delay, task_type task) {
        {
            std::scoped_lock lock(mutex_);
            tasks_.emplace(clock_type::now() + delay, std::move(task));
            if (!thread_.joinable()) {
                thread_ = std::thread([this]() { run(); });
            }
        }
        cv_.notify_one();
    }

    RetryTimer(const RetryTimer&)            = delete;
    RetryTimer& operator=(const RetryTimer&) = delete;

    ~RetryTimer() {
        {
            std::scoped_lock lock(mutex_);
            stopping_ = true;
        }
        cv_.notify_one();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    RetryTimer() = default;

    void run() {
        std::unique_lock lock(mutex_);
        while (!stopping_) {
            if (tasks_.empty()) {
                cv_.wait(lock);
                continue;
            }
            auto next = tasks_.begin();
            if (clock_type::now() < next->first) {
                cv_.wait_until(lock, next->first);
                continue;
            }
            auto task = std::move(next->second);
            tasks_.erase(next);
            lock.unlock();
            try {
                task();
            }
            catch (const std::exception& e) {
                SLOG(E, "AvisoV2: retry failed: " << e.what());
            }
            lock.lock();
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::multimap<clock_type::time_point, task_type> tasks_;
    std::thread thread_;
    bool stopping_ = false;
};

std::string to_rfc3339(std::chrono::system_clock::time_point when) {
    auto seconds = std::chrono::system_clock::to_time_t(when);
    std::tm utc{};
    gmtime_r(&seconds, &utc);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buffer;
}

void wake_server() {
    if (auto* server = TheOneServer::server(); server) {
        // Forces the server to traverse the definitions, so that the attribute is evaluated
        server->increment_job_generation_count();
    }
}

} // namespace

///
/// @brief The state of one backend, shared with the library threads and the retry timer.
///
/// Two mutexes are used: `responses_mutex_` protects the collected responses, and is the only one taken by the
/// library threads; `watch_mutex_` serialises the creation and destruction of the watch, which waits for the library
/// threads to finish.
///
struct AvisoV2Backend::Impl : public std::enable_shared_from_this<AvisoV2Backend::Impl>
{
    Impl(const AvisoSubscribe& request, std::chrono::milliseconds retry_delay)
        : request_{request},
          retry_delay_{retry_delay} {}

    std::vector<AvisoResponse> drain() {
        std::scoped_lock lock(responses_mutex_);
        auto drained = std::move(responses_);
        responses_.clear();
        return drained;
    }

    void open() {
        std::scoped_lock lock(watch_mutex_);
        open_locked();
    }

    void close() {
        std::scoped_lock lock(watch_mutex_);
        closed_ = true;
        // Destroying the watch stops it and waits for its callbacks to finish
        watch_.reset();
        handler_.reset();
        client_.reset();
    }

private:
    struct Handler : public ::aviso::NotificationHandler
    {
        explicit Handler(Impl& impl)
            : impl_{impl} {}

        bool on_notification(const ::aviso::Notification& notification) override {
            AvisoNotification received{notification.event_type(),
                                       notification.sequence(),
                                       notification.identifier_json(),
                                       notification.payload_json()};
            SLOG(D, "AvisoV2: notification for " << impl_.request_.path() << ": " << received);
            impl_.received(received.sequence());
            impl_.push(std::move(received));
            wake_server();
            return true;
        }

        void on_end(const std::optional<::aviso::ErrorInfo>& error) override {
            if (impl_.closed_) {
                return;
            }
            if (error) {
                auto reason = describe(*error);
                SLOG(E, "AvisoV2: watch for " << impl_.request_.path() << " ended with " << reason);
                impl_.push(AvisoError{reason});
                wake_server();
            }
            else {
                SLOG(I, "AvisoV2: watch for " << impl_.request_.path() << " ended by the server");
            }
            impl_.schedule_retry();
        }

        Impl& impl_;
    };

    void push(AvisoResponse response) {
        std::scoped_lock lock(responses_mutex_);
        responses_.push_back(std::move(response));
    }

    void received(std::uint64_t sequence) {
        auto current = last_received_.load();
        while (sequence > current && !last_received_.compare_exchange_weak(current, sequence)) {}
    }

    void schedule_retry() {
        std::weak_ptr<Impl> self = weak_from_this();
        RetryTimer::instance().schedule(retry_delay_, [self]() {
            if (auto impl = self.lock(); impl) {
                impl->reopen();
            }
        });
    }

    void reopen() {
        std::scoped_lock lock(watch_mutex_);
        if (closed_) {
            return;
        }
        SLOG(I, "AvisoV2: re-creating watch for " << request_.path());
        // The previous watch has ended; destroying it waits for the library to release it
        watch_.reset();
        handler_.reset();
        client_.reset();
        open_locked();
    }

    void open_locked() {
        if (closed_) {
            return;
        }

        // Announce the attempt to (re)create the watch first, so that any notification or error it produces comes
        // after (clearing any previous error)
        push(AvisoWatchStarted{});

        try {
            auto listener = parse_listener(request_.listener());
            auto auth     = load_auth(request_.auth());

            ::aviso::ClientBuilder builder(request_.url());
            std::visit(ecf::overload{[&builder](const BasicAuth& a) { builder.basic_auth(a.username, a.password); },
                                     [&builder](const BearerAuth& a) { builder.bearer_auth(a.token); }},
                       auth);
            client_ = std::make_unique<::aviso::Client>(builder.build());

            ::aviso::WatchRequest watch_request(listener.event);
            if (!listener.filter_json.empty()) {
                watch_request.filter_json(listener.filter_json);
            }
            // A re-created watch resumes after the last notification already received, so that none is lost while
            // the watch was down; without any, it resumes from when the first watch was opened (one second earlier,
            // allowing for a clock difference with the Aviso server)
            if (auto after = std::max(request_.revision(), last_received_.load()); after > 0) {
                watch_request.watch_from_sequence(after);
            }
            else if (first_opened_) {
                watch_request.watch_from_date(to_rfc3339(*first_opened_ - std::chrono::seconds{1}));
            }
            else {
                first_opened_ = std::chrono::system_clock::now();
            }

            handler_ = std::make_unique<Handler>(*this);
            watch_   = std::make_unique<::aviso::Watch>(client_->watch(watch_request, *handler_));

            SLOG(D, "AvisoV2: watching " << request_.url() << " for " << request_.path());
        }
        catch (const ::aviso::Error& e) {
            fail(describe(e.error()));
        }
        catch (const std::exception& e) {
            fail(describe_error("config", 0, e.what()));
        }
    }

    void fail(const std::string& reason) {
        SLOG(E, "AvisoV2: unable to watch for " << request_.path() << ": " << reason);
        watch_.reset();
        handler_.reset();
        client_.reset();
        push(AvisoError{reason});
        // The failure may happen on the retry timer thread, between two traversals of the server
        wake_server();
        schedule_retry();
    }

    const AvisoSubscribe request_;
    const std::chrono::milliseconds retry_delay_;
    std::atomic<bool> closed_{false};
    std::atomic<std::uint64_t> last_received_{0};
    std::optional<std::chrono::system_clock::time_point> first_opened_; // only used while holding watch_mutex_

    std::mutex responses_mutex_;
    std::vector<AvisoResponse> responses_;

    std::mutex watch_mutex_;
    std::unique_ptr<::aviso::Client> client_;
    std::unique_ptr<Handler> handler_;
    std::unique_ptr<::aviso::Watch> watch_;
};

AvisoV2Backend::AvisoV2Backend(std::chrono::milliseconds retry_delay)
    : retry_delay_{retry_delay} {
}

AvisoV2Backend::~AvisoV2Backend() {
    if (impl_) {
        impl_->close();
    }
}

void AvisoV2Backend::subscribe(const AvisoSubscribe& request) {
    SLOG(D, "AvisoV2: subscribing " << request);
    if (impl_) {
        impl_->close();
    }
    impl_ = std::make_shared<Impl>(request, retry_delay_);
    impl_->open();
}

std::vector<AvisoResponse> AvisoV2Backend::drain() {
    if (!impl_) {
        return {};
    }
    return impl_->drain();
}

void AvisoV2Backend::register_as_default() {
    register_backend([]() { return std::make_unique<AvisoV2Backend>(); });
}

} // namespace ecf::service::aviso::v2
