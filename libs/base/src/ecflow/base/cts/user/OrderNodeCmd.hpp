/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_OrderNodeCmd_HPP
#define ecflow_base_cts_user_OrderNodeCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"
#include "ecflow/core/NOrder.hpp"

class OrderNodeCmd final : public UserCmd {
public:
    OrderNodeCmd(const std::string& absNodepath, NOrder::Order op) : absNodepath_(absNodepath), option_(op) {}
    OrderNodeCmd() = default;

    [[deprecated]] const std::string& absNodePath() const { return absNodepath_; }
    const std::string& pathToNode() const { return absNodepath_; }
    NOrder::Order option() const { return option_; }

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
    // bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;

private:
    std::string absNodepath_;
    NOrder::Order option_{NOrder::TOP};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(absNodepath_), CEREAL_NVP(option_));
    }
};

std::ostream& operator<<(std::ostream& os, const OrderNodeCmd&);

CEREAL_FORCE_DYNAMIC_INIT(OrderNodeCmd)

#endif /* ecflow_base_cts_user_OrderNodeCmd_HPP */
