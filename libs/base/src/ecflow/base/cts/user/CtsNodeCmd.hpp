/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_CtsNodeCmd_HPP
#define ecflow_base_cts_user_CtsNodeCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// Collection of commands, that all take a abs node path as their only arg
// Reduce number of global symbols caused by boost serialisation
// Previously they were all separate commands
//
// Client---(CtsNodeCmd(GET))---->Server-----(DefsCmd | SNodeCmd )--->client:
// When doHandleRequest is called in the server it will return DefsCmd
// The DefsCmd is used to transport the node tree hierarchy to/from the server
//
// CHECK_JOB_GEN_ONLY command will traverse hierarchically from the given node path
// and force generation of jobs. (i.e independently of dependencies).
// This is used in *testing* only, so that we can compare/test/verify
// job generation with the old version.
// if absNodepath is empty we will generate jobs for all tasks
class CtsNodeCmd final : public UserCmd {
public:
    enum Api { NO_CMD, JOB_GEN, CHECK_JOB_GEN_ONLY, GET, WHY, GET_STATE, MIGRATE };
    CtsNodeCmd(Api a, const std::string& absNodePath) : api_(a), absNodePath_(absNodePath) {}
    explicit CtsNodeCmd(Api a) : api_(a) { assert(a != NO_CMD); }
    CtsNodeCmd() = default;

    Api api() const { return api_; }
    [[deprecated]] const std::string& absNodePath() const { return absNodePath_; }
    const std::string& pathToNode() const { return absNodePath_; }

    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    PrintStyle::Type_t show_style() const override;

    int timeout() const override;
    bool isWrite() const override;
    bool handleRequestIsTestable() const override { return !terminate_cmd(); }
    bool why_cmd(std::string& nodePath) const override;
    bool get_cmd() const override { return api_ == GET; }

    const char* theArg() const override;
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    // bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;

private:
    Api api_{NO_CMD};
    std::string absNodePath_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(api_), CEREAL_NVP(absNodePath_));
    }
};

std::ostream& operator<<(std::ostream& os, const CtsNodeCmd&);

CEREAL_FORCE_DYNAMIC_INIT(CtsNodeCmd)

#endif /* ecflow_base_cts_user_UserCmd_HPP */
