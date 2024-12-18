/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_CtsCmd_HPP
#define ecflow_base_cts_user_CtsCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// This command is used to encapsulate all commands that are
// simple signals to the server. This helps to cut down on the
// number of global symbols used by boost serialisation.
// =========================================================================
// *** IMPORTANT ***: If any of these commands in the future need arguments,
// *** then ensure to place a DUMMY enum in its place.
// *** This will allow a *newer* development client to still send message to a older server.
// *** i.e like terminating the server
// *** IMPORTANT: For any new commands, must be added to the end, for each major release
// *** - STATS_RESET was introduced in release 4.0.5
// =========================================================================
class CtsCmd final : public UserCmd {
public:
    enum Api {
        NO_CMD,
        RESTORE_DEFS_FROM_CHECKPT,
        RESTART_SERVER,
        SHUTDOWN_SERVER,
        HALT_SERVER,
        TERMINATE_SERVER,
        RELOAD_WHITE_LIST_FILE,
        FORCE_DEP_EVAL,
        PING,
        GET_ZOMBIES,
        STATS,
        SUITES,
        DEBUG_SERVER_ON,
        DEBUG_SERVER_OFF,
        SERVER_LOAD,
        STATS_RESET,
        RELOAD_PASSWD_FILE,
        STATS_SERVER,
        RELOAD_CUSTOM_PASSWD_FILE
    };

    explicit CtsCmd(Api a) : api_(a) {}
    CtsCmd() = default;

    Api api() const { return api_; }

    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    bool isWrite() const override;
    bool cmd_updates_defs() const override;
    bool terminate_cmd() const override { return api_ == TERMINATE_SERVER; }
    bool ping_cmd() const override { return api_ == PING; }
    int timeout() const override;

    bool handleRequestIsTestable() const override;

    const char* theArg() const override;
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

    Api api_{NO_CMD};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(api_));
    }
};

std::ostream& operator<<(std::ostream& os, const CtsCmd&);

CEREAL_FORCE_DYNAMIC_INIT(CtsCmd)

#endif /* ecflow_base_cts_user_CtsCmd_HPP */
