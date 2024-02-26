/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_RunNodeCmd_HPP
#define ecflow_base_cts_user_RunNodeCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// The absNodepath must be provided
class RunNodeCmd final : public UserCmd {
public:
    RunNodeCmd(const std::string& absNodepath, bool force, bool test = false)
        : paths_(std::vector<std::string>(1, absNodepath)),
          force_(force),
          test_(test) {}

    RunNodeCmd(const std::vector<std::string>& paths, bool force, bool test = false)
        : paths_(paths),
          force_(force),
          test_(test) {}

    RunNodeCmd() = default;

    const std::vector<std::string>& paths() const { return paths_; }
    bool force() const { return force_; }

    bool isWrite() const override { return true; }
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    void print(std::string& os, const std::string& path) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    void cleanup() override { std::vector<std::string>().swap(paths_); } /// run in the server, after doHandleRequest

private:
    std::vector<std::string> paths_;
    bool force_{false};
    bool test_{false}; // only for test, hence we don't serialise this

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(paths_), CEREAL_NVP(force_));
    }
};

std::ostream& operator<<(std::ostream& os, const RunNodeCmd&);

CEREAL_FORCE_DYNAMIC_INIT(RunNodeCmd)

#endif /* ecflow_base_cts_user_RunNodeCmd_HPP */
