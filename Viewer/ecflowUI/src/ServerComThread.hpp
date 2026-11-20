// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <deque>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <QThread>

#include "VTask.hpp"
#include "ecflow/node/AbstractObserver.hpp"
#include "ecflow/node/Defs.hpp"

class ChangeMgrSingleton;
class ClientInvoker;
class ServerComQueue;
class ServerHandler;

// -------------------------------------------------------
// ServerComThread - a class to handler communication with
// an ecflow server.
// -------------------------------------------------------

class ServerComThread : public QThread, public AbstractObserver {
    Q_OBJECT

public:
    ServerComThread(ServerHandler* server, ClientInvoker* ci);
    ~ServerComThread() override;

    void task(VTask_ptr);
    void blockingLogout();

    // From AbstractObserver
    void update_start(const Node*, const std::vector<ecf::Aspect::Type>&) override {}
    void update_start(const Defs*, const std::vector<ecf::Aspect::Type>&) override {}
    void update(const Node*, const std::vector<ecf::Aspect::Type>&) override;
    void update(const Defs*, const std::vector<ecf::Aspect::Type>&) override;
    void update_delete(const Node*) override;
    void update_delete(const Defs*) override;

Q_SIGNALS:
    void nodeChanged(const Node*, std::vector<ecf::Aspect::Type>);
    void defsChanged(std::vector<ecf::Aspect::Type>);
    void rescanNeed();
    void failed(std::string message);
    void suiteListChanged(const std::vector<std::string>&, const std::vector<std::string>&);
    void logoutDone();

protected:
    void run() override;
    void reset();

    ///
    /// @brief Registers, with a new handle, the suites of the filter that the user can read, or a dummy suite when
    ///        there is none, after the registration of the whole filter was denied (R6.4).
    ///
    /// @throws std::runtime_error when a request fails.
    ///
    void registerReadableSuites();

    ///
    /// @brief Registers the suites with a new handle, after dropping the current handle and those left behind by
    ///        failed registrations (R6.4).
    ///
    /// @param[in] suites The suites registered
    /// @throws std::runtime_error, or ecf::PermissionDenied, when the registration fails; the handles that could not
    ///         be dropped are kept, to be dropped before the next registration.
    ///
    void registerSuites(const std::vector<std::string>& suites);

    ///
    /// @brief Drops the handles kept by registerSuites(), without synchronising.
    ///
    /// The handles that the server no longer lists among those of the user (--ch_suites) are forgotten; a handle
    /// whose drop fails, and every handle when the listing fails, is kept.
    ///
    void dropOrphanedHandles();

private:
    void attach();
    void attach(defs_ptr d);
    void attach(Node* node);
    void detach();
    void detach(defs_ptr d);
    void detach(Node* node);
    void aspectToStr(std::stringstream& s, const std::vector<ecf::Aspect::Type>& t) const;

    ServerHandler* server_{nullptr};
    std::string serverName_;
    ClientInvoker* ci_{nullptr};
    VTask::Type taskType_{VTask::NoTask};
    std::vector<std::string> command_;
    std::string commandAsStr_;
    std::map<std::string, std::string> params_;
    std::vector<std::string> contents_;
    NameValueVec vars_;
    Zombie zombie_;
    std::string nodePath_;
    bool rescanNeed_{false};
    bool hasSuiteFilter_{false};
    std::vector<std::string> filteredSuites_;
    std::vector<int> orphanedHandles_; // handles left on the server by failed registrations (R6.4)
    bool autoAddNewSuites_{false};
    int maxLineNum_{-1};
    bool initialResetDone_{false};
};
