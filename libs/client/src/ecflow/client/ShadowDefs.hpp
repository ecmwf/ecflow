// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "ecflow/client/ClientInvoker.hpp"

class Defs;

namespace ecf {

///
/// @brief Obtains the definition held by an ecFlow server, on behalf of ShadowDefs.
///
/// The interface separates ShadowDefs from the means of reaching the server, so that a test can replace the server.
/// Every operation reports a failure to reach the server, or a refusal by the server, by throwing.
///
class ShadowDefsSource {
public:
    virtual ~ShadowDefsSource() = default;

    ///
    /// @brief Fetches the whole definition from the server.
    ///
    /// @return The definition, as the server holds it now.
    /// @throws std::exception when the server cannot be reached, or refuses the request.
    ///
    virtual std::shared_ptr<Defs> fetch() = 0;

    ///
    /// @brief Asks the server whether its definition changed since the given copy was obtained.
    ///
    /// @param[in] defs The copy held so far; never null.
    /// @return True when the server has changes that the copy does not have.
    /// @throws std::exception when the server cannot be reached, or refuses the request.
    ///
    virtual bool has_news(const std::shared_ptr<Defs>& defs) = 0;

    ///
    /// @brief Brings a copy of the definition up to date with the server.
    ///
    /// @param[in] defs The copy held so far; may be null, in which case the definition obtained by the last fetch is
    ///                 returned.
    /// @return The updated definition, which may be the same object as the copy given.
    /// @throws std::exception when the server cannot be reached, or refuses the request.
    ///
    virtual std::shared_ptr<Defs> update(const std::shared_ptr<Defs>& defs) = 0;
};

///
/// @brief Reaches the ecFlow server through a ClientInvoker, configured as ClientInvoker is by default.
///
/// This is the source used outside tests. It uses news_local and sync_local to fetch, news to ask for changes, and
/// sync to update.
///
class ClientInvokerShadowDefsSource : public ShadowDefsSource {
public:
    std::shared_ptr<Defs> fetch() override;
    bool has_news(const std::shared_ptr<Defs>& defs) override;
    std::shared_ptr<Defs> update(const std::shared_ptr<Defs>& defs) override;

private:
    ClientInvoker client_;
};

///
/// @brief Keeps a copy of the definition held by an ecFlow server, polling the server for changes.
///
/// After start, a thread asks the server for changes at an interval, and updates the copy when there are some. The
/// interval is given by a policy that the owner provides, so that each service chooses its own: the REST service
/// lengthens it while it receives no requests, and other services may keep it fixed. A request for an immediate
/// update is served by the same thread, with an update that starts after the request was made.
///
/// A failure to reach the server while polling does not stop the thread: it is reported through the report function,
/// and the thread tries again after the retry delay, keeping the last copy.
///
/// Threads: get and refresh may be called from any thread while the polling thread runs, and stop may run while
/// refresh waits, ending that wait. start and stop must not overlap with each other, with poll_once, or with a refresh
/// made while no polling thread runs. poll_once must only be called when no polling thread runs, since it uses the
/// same means of reaching the server. The interval policy is called in the polling thread; an exception that it
/// throws is reported and retried like a failure to reach the server.
///
/// The copy returned by get is the object that later updates change in place; the copy is not handed out while an
/// update runs, but a holder of the copy may observe a later update.
///
/// @invariant Once start has returned, get never returns null.
///
class ShadowDefs {
public:
    /// @brief Returns the time to wait before the next question to the server.
    using IntervalPolicy = std::function<std::chrono::milliseconds()>;
    /// @brief Receives a description of a failure to reach the server while polling.
    using Report = std::function<void(const std::string&)>;
    /// @brief Receives the copy after each update; called in the thread that made the update.
    using Updated = std::function<void(const Defs&)>;

    ///
    /// @brief Creates a copy that is empty until start is called.
    ///
    /// @param[in] source The means of reaching the server; must not be null.
    /// @param[in] next_interval Called after the start and after each question, to give the time to wait before the
    ///                          next question.
    /// @param[in] report Called with a description of each failure to reach the server while polling; may be empty.
    /// @param[in] retry_delay The time to wait after such a failure before trying again.
    /// @param[in] updated Called with the copy after each update made by poll_once or refresh; may be empty.
    ///
    ShadowDefs(std::unique_ptr<ShadowDefsSource> source,
               IntervalPolicy next_interval,
               Report report                         = {},
               std::chrono::milliseconds retry_delay = std::chrono::seconds(5),
               Updated updated                       = {});

    ShadowDefs(const ShadowDefs&)            = delete;
    ShadowDefs& operator=(const ShadowDefs&) = delete;

    ///
    /// @brief Stops the polling thread, if it runs.
    ///
    ~ShadowDefs();

    ///
    /// @brief Fetches the whole definition, then starts polling for changes.
    ///
    /// The definition is fetched in the calling thread, so that a server that cannot be reached is reported to the
    /// caller.
    ///
    /// @throws std::exception when the server cannot be reached, or refuses the request; the polling thread is then
    ///         not started.
    ///
    void start();

    ///
    /// @brief Stops the polling thread and waits for it to end. Calling it again, or before start, does nothing.
    ///
    void stop();

    ///
    /// @brief Returns the current copy of the definition.
    ///
    /// @return The copy; null before start has succeeded.
    ///
    [[nodiscard]] std::shared_ptr<Defs> get() const;

    ///
    /// @brief Asks the polling thread for an immediate update, waits until it is done, then calls the continuation.
    ///
    /// The update that ends the wait is one that started after this call was made. When the update fails, the polling
    /// thread tries again after the retry delay, and this call keeps waiting; a stop ends the wait. When no polling
    /// thread runs, the update is made in the calling thread.
    ///
    /// @param[in] then Called in the calling thread once the copy is up to date; also called, without an update,
    ///                 when a stop ends the wait.
    /// @throws std::exception when no polling thread runs and the update fails.
    ///
    void refresh(const std::function<void()>& then = [] {});

    ///
    /// @brief Asks the server once for changes, and updates the copy when there are some.
    ///
    /// This is one iteration of the polling thread, without its waits; tests call it directly. It requires a copy,
    /// that is, a successful start.
    ///
    /// @throws std::exception when the server cannot be reached, or refuses the request; the copy is then unchanged.
    ///
    void poll_once();

    ///
    /// @brief Returns the number of refresh requests made while the polling thread ran.
    ///
    /// Intended for tests and diagnostics, to observe that a request was registered.
    ///
    /// @return The number of requests made since construction.
    ///
    [[nodiscard]] std::uint64_t requests_made();

private:
    void update_copy();
    void loop();
    std::uint64_t served_requests();

    std::unique_ptr<ShadowDefsSource> source_;
    IntervalPolicy next_interval_;
    Report report_;
    std::chrono::milliseconds retry_delay_;
    Updated updated_;

    mutable std::mutex defs_mutex_;
    std::shared_ptr<Defs> defs_;

    std::mutex wakeup_mutex_;
    std::condition_variable wakeup_;
    std::atomic<bool> stopping_{false};
    std::uint64_t requested_{0}; // guarded by wakeup_mutex_
    std::uint64_t served_{0};    // guarded by wakeup_mutex_

    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> thread_;
};

} // namespace ecf
