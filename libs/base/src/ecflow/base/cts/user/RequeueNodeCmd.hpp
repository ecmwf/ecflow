/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_RequeueNodeCmd_HPP
#define ecflow_base_cts_user_RequeueNodeCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

class RequeueNodeCmd final : public UserCmd {
public:
    enum Option { NO_OPTION, ABORT, FORCE };

    explicit RequeueNodeCmd(const std::vector<std::string>& paths, Option op = NO_OPTION)
        : paths_(paths),
          option_(op) {}

    explicit RequeueNodeCmd(const std::string& absNodepath, Option op = NO_OPTION)
        : paths_(std::vector<std::string>(1, absNodepath)),
          option_(op) {}

    RequeueNodeCmd() = default;

    const std::vector<std::string>& paths() const { return paths_; }
    Option option() const { return option_; }

    bool isWrite() const override { return true; }
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    void print(std::string& os, const std::string& path) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    // bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    void cleanup() override { std::vector<std::string>().swap(paths_); } /// run in the server, after doHandleRequest

private:
    mutable std::vector<std::string> paths_; // mutable to allow swap to clear & reclaim memory, as soon as possible
    Option option_{NO_OPTION};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(paths_), CEREAL_NVP(option_));
    }
};

std::ostream& operator<<(std::ostream& os, const RequeueNodeCmd&);

CEREAL_FORCE_DYNAMIC_INIT(RequeueNodeCmd)

#endif /* ecflow_base_cts_user_RequeueNodeCmd_HPP */
