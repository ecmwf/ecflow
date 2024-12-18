/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_PlugCmd_HPP
#define ecflow_base_cts_user_PlugCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

class PlugCmd final : public UserCmd {
public:
    PlugCmd(const std::string& source, const std::string& dest) : source_(source), dest_(dest) {}
    PlugCmd() = default;

    // Uses by equals only
    const std::string& source() const { return source_; }
    const std::string& dest() const { return dest_; }

    int timeout() const override { return 120; }
    bool handleRequestIsTestable() const override { return false; }
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
    std::string source_;
    std::string dest_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(source_), CEREAL_NVP(dest_));
    }
};

std::ostream& operator<<(std::ostream& os, const PlugCmd&);

CEREAL_FORCE_DYNAMIC_INIT(PlugCmd)

#endif /* ecflow_base_cts_user_PlugCmd_HPP */
