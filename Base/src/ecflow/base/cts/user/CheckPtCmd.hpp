/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_CheckPtCmd_HPP
#define ecflow_base_cts_user_CheckPtCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"
#include "ecflow/core/CheckPt.hpp"

class CheckPtCmd final : public UserCmd {
public:
    CheckPtCmd(ecf::CheckPt::Mode m, int interval, int checkpt_save_time_alarm)
        : mode_(m),
          check_pt_interval_(interval),
          check_pt_save_time_alarm_(checkpt_save_time_alarm) {}
    CheckPtCmd() = default;

    ecf::CheckPt::Mode mode() const { return mode_; }
    int check_pt_interval() const { return check_pt_interval_; }
    int check_pt_save_time_alarm() const { return check_pt_save_time_alarm_; }

    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;
    bool isWrite() const override;
    bool is_mutable() const override;
    const char* theArg() const override;
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

private:
    ecf::CheckPt::Mode mode_{ecf::CheckPt::UNDEFINED};
    int check_pt_interval_{0};
    int check_pt_save_time_alarm_{0};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(mode_),
           CEREAL_NVP(check_pt_interval_),
           CEREAL_NVP(check_pt_save_time_alarm_));
    }
};

std::ostream& operator<<(std::ostream& os, const CheckPtCmd&);

CEREAL_FORCE_DYNAMIC_INIT(CheckPtCmd)

#endif /* ecflow_base_cts_user_CheckPtCmd_HPP */
