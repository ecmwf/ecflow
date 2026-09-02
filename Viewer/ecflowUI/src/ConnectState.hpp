/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_ConnectState_HPP
#define ecflow_viewer_ConnectState_HPP

#include <ctime>
#include <string>

#include "ecflow/base/ConnectionDiagnosis.hpp"

class ConnectState {
public:
    ConnectState();

    ///
    /// @brief The state of the connection to a server, as far as the viewer can tell.
    ///
    /// ProtocolMismatch covers every pair of protocols the two ends can disagree on, and is
    /// deliberately distinct from the states that mean the server cannot be reached at all.
    ///
    enum State {
        Undef,
        Normal,
        Disconnected,
        Lost,
        VersionIncompatible,
        SslCertificateError,
        FailedClient,
        ProtocolMismatch
    };

    void state(State state);
    State state() const { return state_; }
    const std::string& describe() const;
    void errorMessage(const std::string&);
    std::time_t lastConnectTime() const { return lastConnect_; }
    std::time_t lastLostTime() const { return lastFailed_; }
    std::time_t lastDisconnectTime() const { return lastDisconnect_; }
    const std::string& errorMessage() const { return errMsg_; }
    const std::string& shortErrorMessage() const { return shortErrMsg_; }

    ///
    /// @brief Provides the structured diagnosis behind the current state.
    ///
    /// Carrying the diagnosis, rather than only its rendered message, is what allows each part of
    /// the interface to phrase the failure in its own terms without parsing prose.
    ///
    /// @return The diagnosis; records no failure whenever the state is not a failure
    ///
    const ecf::ConnectionDiagnosis& diagnosis() const { return diagnosis_; }

    ///
    /// @brief Records the structured diagnosis behind the current state.
    ///
    /// @param[in] diagnosis The diagnosis observed by the client
    ///
    void diagnosis(const ecf::ConnectionDiagnosis& diagnosis) { diagnosis_ = diagnosis; }

protected:
    static void init();
    void logConnect();
    void logFailed();
    void logDisconnect();

    State state_{Undef};
    std::time_t lastConnect_{0};
    std::time_t lastFailed_{0};
    std::time_t lastDisconnect_{0};
    std::string errMsg_;
    std::string shortErrMsg_;
    ecf::ConnectionDiagnosis diagnosis_;
};

#endif /* ecflow_viewer_ConnectState_HPP */
