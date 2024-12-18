/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_ReplaceNodeCmd_HPP
#define ecflow_base_cts_user_ReplaceNodeCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

class ReplaceNodeCmd final : public UserCmd {
public:
    ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, defs_ptr client_defs, bool force);
    ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, const std::string& path_to_defs, bool force);
    ReplaceNodeCmd() = default;

    const std::string& the_client_defs() const { return clientDefs_; }
    const std::string& pathToNode() const { return pathToNode_; }
    const std::string& path_to_defs() const { return path_to_defs_; }
    bool createNodesAsNeeded() const { return createNodesAsNeeded_; }
    bool force() const { return force_; }

    bool isWrite() const override { return true; }
    int timeout() const override { return 300; }
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

    // void set_client_env(const std::vector<std::pair<std::string,std::string> >& env ) { client_env_ = env;} // only
    // used in test

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    // bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    void cleanup() override { std::string().swap(clientDefs_); } /// run in the server, after command send to client

    bool createNodesAsNeeded_{false};
    bool force_{false};
    std::string pathToNode_;
    std::string path_to_defs_; // Can be empty if defs loaded in memory via python api
    std::string clientDefs_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(createNodesAsNeeded_),
           CEREAL_NVP(force_),
           CEREAL_NVP(pathToNode_),
           CEREAL_NVP(path_to_defs_),
           CEREAL_NVP(clientDefs_));
    }
};

std::ostream& operator<<(std::ostream& os, const ReplaceNodeCmd&);

CEREAL_FORCE_DYNAMIC_INIT(ReplaceNodeCmd)

#endif /* ecflow_base_cts_user_ReplaceNodeCmd_HPP */
