/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_BeginCmd_HPP
#define ecflow_base_cts_user_BeginCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"
#include "ecflow/node/Defs.hpp"

// class Begin:  if suiteName is empty we will begin all suites
class BeginCmd final : public UserCmd {
public:
    explicit BeginCmd(const std::string& suiteName, bool force = false);
    BeginCmd() = default;

    const std::string& suiteName() const { return suiteName_; }
    bool force() const { return force_; }

    int timeout() const override { return 80; }

    bool isWrite() const override { return true; }
    void print(std::string&) const override;
    void print_only(std::string&) const override;
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

private:
    std::string suiteName_;
    bool force_{false}; // reset begin status on suites & bypass checks, can create zombies, used in test only

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(suiteName_), CEREAL_NVP(force_));
    }
};

std::ostream& operator<<(std::ostream& os, const BeginCmd&);

CEREAL_FORCE_DYNAMIC_INIT(BeginCmd)

#endif /* ecflow_base_cts_user_BeginCmd_HPP */
