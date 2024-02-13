/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_LoadDefsCmd_HPP
#define ecflow_base_cts_user_LoadDefsCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// Will *load* the suites, into the server.
// Additionally the server will try to resolve extern's. The extern are references
// to Node, events, meters, limits, variables defined on another suite.
class LoadDefsCmd final : public UserCmd {
public:
    explicit LoadDefsCmd(const defs_ptr& defs, bool force = false);
    explicit LoadDefsCmd(const std::string& defs_filename,
                         bool force      = false,
                         bool check_only = false /* not persisted */,
                         bool print      = false /* not persisted */,
                         bool stats      = false /* not persisted */,
                         const std::vector<std::pair<std::string, std::string>>& client_env =
                             std::vector<std::pair<std::string, std::string>>());
    LoadDefsCmd() = default;

    // Uses by equals only
    const std::string& defs_as_string() const { return defs_; }

    bool isWrite() const override { return true; }
    int timeout() const override { return time_out_for_load_sync_and_get(); }
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;
    static Cmd_ptr create(const std::string& defs_filename,
                          bool force,
                          bool check_only,
                          bool print,
                          bool stats,
                          AbstractClientEnv* clientEnv);

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the command as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

    bool force_{false};
    std::string defs_;
    std::string defs_filename_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(force_), CEREAL_NVP(defs_), CEREAL_NVP(defs_filename_));
    }
};

std::ostream& operator<<(std::ostream& os, const LoadDefsCmd&);

CEREAL_FORCE_DYNAMIC_INIT(LoadDefsCmd)

#endif /* ecflow_base_cts_user_LoadDefsCmd_HPP */
