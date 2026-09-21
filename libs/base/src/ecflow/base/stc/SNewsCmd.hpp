// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

///
/// @brief The reply to a --news request: tells the client whether it must synchronise.
///
/// The decision is taken by ecf::evaluate_news(); this class carries its result back to the client, and keeps the
/// justification, the annotation, for the server to append to the log record of the request. The annotation is not
/// sent to the client.
///
/// Paired with CSyncCmd(NEWS):
/// Client---CSyncCmd(NEWS)---->Server-----(SNewsCmd)--->client
///

#include <string>

#include "ecflow/base/stc/ServerToClientCmd.hpp"
class AbstractServer;
class ClientSuiteMgr;

namespace ecf {

///
/// @brief Holds the answer to a --news request: whether the client must synchronise, and why.
///
struct NewsOutcome
{
    ServerReply::News_t news{ServerReply::NO_NEWS}; ///< What the client must do
    std::string annotation;                         ///< Justification, for the log record of the request
};

///
/// @brief Determines whether the server holds changes the client has not yet seen.
///
/// The decision compares the change numbers the client last synchronised with those of the server:
///
/// - with client handle 0, the server-wide state and modify change numbers are used; otherwise the maximum over the
///   suites registered to the handle;
/// - a client whose numbers are ahead of the server (the server restarted, or a change number wrapped around), or
///   whose handle the server no longer knows, must perform a full synchronisation (DO_FULL_SYNC);
/// - a handle whose suites were added or removed, or a modify change number behind the server, means large-scale
///   changes; a state change number behind the server means small-scale changes; either way the client must
///   synchronise (NEWS);
/// - otherwise the client is up to date (NO_NEWS).
///
/// The annotation spells out which case applied, with the numbers compared, in the form
/// ` [server(<state>,<modify>) : <reason> :<news>]` (or ` [server handle(...) server(...) : ...]` for a handle), or
/// ` [:NO_NEWS]`.
///
/// The function only reads the server-wide change numbers (Ecf::state_change_no(), Ecf::modify_change_no()) and the
/// given client suite manager, and writes nothing to the log. The comparisons must stay consistent with those made
/// by SSyncCmd, which decides what a synchronisation carries.
///
/// @param[in] client_handle           The client handle, 0 when the client is not registered to a subset of suites.
/// @param[in] client_state_change_no  The state change number the client last synchronised with.
/// @param[in] client_modify_change_no The modify change number the client last synchronised with.
/// @param[in] client_suite_mgr        The server's client suite manager; consulted only when @p client_handle is not 0.
/// @return The decision, with its annotation.
///
NewsOutcome evaluate_news(unsigned int client_handle,
                          unsigned int client_state_change_no,
                          unsigned int client_modify_change_no,
                          const ClientSuiteMgr& client_suite_mgr);

} // namespace ecf

class SNewsCmd final : public ServerToClientCmd {
public:
    // The constructor is *called* in the server.
    SNewsCmd(unsigned int client_handle, // a reference to a set of suites used by client
             unsigned int client_state_change_no,
             unsigned int client_modify_change_no,
             AbstractServer* as);
    SNewsCmd()
        : ServerToClientCmd() {}

    ///
    /// @brief Evaluates the news for the given client, in the server, and stores the decision and its annotation.
    ///
    /// Delegates to ecf::evaluate_news(); see there for the decision rules. Nothing is written to the log.
    ///
    /// @param[in] client_handle           The client handle, 0 when the client is not registered to a subset of suites.
    /// @param[in] client_state_change_no  The state change number the client last synchronised with.
    /// @param[in] client_modify_change_no The modify change number the client last synchronised with.
    /// @param[in] as                      The server, providing the definitions and the client suite manager.
    ///
    void init(unsigned int client_handle,
              unsigned int client_state_change_no,
              unsigned int client_modify_change_no,
              AbstractServer* as);

    ServerReply::News_t news() const { return news_; } // used by equals only
    bool get_news() const { return (news_ != ServerReply::NO_NEWS); }

    ///
    /// @brief Returns the justification of the last decision, for the log record of the request.
    ///
    /// @return Text of the form ` [server(<state>,<modify>) : <reason> :<news>]`, or ` [:NO_NEWS]`; empty before
    ///         init() is called. Only meaningful in the server, as it is not serialised.
    ///
    const std::string& annotation() const { return annotation_; }

    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override;

private:
    ServerReply::News_t news_{ServerReply::NO_NEWS};
    std::string annotation_; // server side only, not serialised

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(news_));
    }
};

std::ostream& operator<<(std::ostream& os, const SNewsCmd&);
