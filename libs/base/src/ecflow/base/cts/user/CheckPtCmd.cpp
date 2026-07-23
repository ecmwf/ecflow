/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/CheckPtCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/HelpCatalog.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Converter.hpp"

using namespace ecf;

void CheckPtCmd::print(std::string& os) const {
    user_cmd(os, CtsApi::checkPtDefs(mode_, check_pt_interval_, check_pt_save_time_alarm_));
}
void CheckPtCmd::print_only(std::string& os) const {
    os += CtsApi::checkPtDefs(mode_, check_pt_interval_, check_pt_save_time_alarm_);
}

bool CheckPtCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<CheckPtCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (mode_ != the_rhs->mode()) {
        return false;
    }
    if (check_pt_interval_ != the_rhs->check_pt_interval()) {
        return false;
    }
    if (check_pt_save_time_alarm_ != the_rhs->check_pt_save_time_alarm()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t CheckPtCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t CheckPtCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

bool CheckPtCmd::isWrite() const {
    if (mode_ != ecf::CheckPt::UNDEFINED) {
        return true;
    }
    if (check_pt_interval_ != 0) {
        return true;
    }
    if (check_pt_save_time_alarm_ != 0) {
        return true;
    }
    return false;
}

bool CheckPtCmd::is_mutable() const {
    // if save takes too long, the late flag is set. Even when command is read only
    // Also if writing to checkpoint fails we set: ecf::Flag::CHECKPT_ERROR
    // Likewise writing to the log file can also fail, hence we set: ecf::Flag::LOG_ERROR, when manually check pointing
    // Even when command is read only ?
    // This an exceptional situation!!
    return true;
}

const char* CheckPtCmd::theArg() const {
    return CtsApi::checkPtDefsArg();
}

STC_Cmd_ptr CheckPtCmd::doHandleRequest(AbstractServer* as) const {
    // Placed here rather than in server. Since we want to record explicit request to check pt
    // The update_stats() is used to record the number of requests per second, hence we do not
    // want to skew this, and hence we ignore implicit request's via signal handling,
    // or when server terminates( does implicit check pt also)
    as->update_stats().checkpt_++;
    if (!as->checkPtDefs(mode_, check_pt_interval_, check_pt_save_time_alarm_)) {
        throw std::runtime_error("Could not save check point file. file system full or permissions ?");
    }
    return PreAllocatedReply::ok_cmd();
}

void CheckPtCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(CtsApi::checkPtDefsArg(),
                       boost::program_options::value<std::string>()->implicit_value(std::string{}));
}

static int parse_check_pt_interval(const std::string& the_arg) {
    int check_pt_interval = 0;
    try {
        check_pt_interval = ecf::convert_to<int>(the_arg);
    }
    catch (...) {
        throw std::runtime_error(
            MESSAGE("check_pt: Illegal argument("
                    << the_arg << "), expected [ never | on_time | on_time:<integer> | always | <integer>]\n"
                    << HelpCatalog::description_for("check_pt").value_or(HelpCatalog::not_provided)));
    }
    if (check_pt_interval <= 0) {
        throw std::runtime_error(MESSAGE(
            "check_pt: interval(" << check_pt_interval << ") must be greater than zero :\n"
                                  << HelpCatalog::description_for("check_pt").value_or(HelpCatalog::not_provided)));
    }
    return check_pt_interval;
}

static int parse_check_pt_alarm_time(const std::string& the_arg, int colon_pos) {
    std::string alarm_time = the_arg.substr(colon_pos + 1);

    int check_pt_alarm_time = 0;
    try {
        check_pt_alarm_time = ecf::convert_to<int>(alarm_time);
    }
    catch (...) {
        throw std::runtime_error(MESSAGE(
            "check_pt: Illegal argument("
            << the_arg << "), expected [ never | on_time | on_time:<integer> | alarm::integer> | always | <integer>]\n"
            << HelpCatalog::description_for("check_pt").value_or(HelpCatalog::not_provided)));
    }
    if (check_pt_alarm_time <= 0) {
        throw std::runtime_error(MESSAGE(
            "check_pt: alarm time(" << check_pt_alarm_time << ") must be greater than zero :\n"
                                    << HelpCatalog::description_for("check_pt").value_or(HelpCatalog::not_provided)));
    }
    return check_pt_alarm_time;
}

void CheckPtCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ace) const {
    if (ace->debug()) {
        std::cout << "CheckPtCmd::create\n";
    }

    std::string the_arg = vm[theArg()].as<std::string>();

    if (ace->debug()) {
        std::cout << "  CheckPtCmd::create arg = " << the_arg << "\n";
    }

    ecf::CheckPt::Mode m         = ecf::CheckPt::UNDEFINED;
    int check_pt_interval        = 0;
    int check_pt_save_time_alarm = 0;

    if (!the_arg.empty()) {
        size_t colon_pos = the_arg.find(":");
        if (colon_pos != std::string::npos) {
            // could be mode:interval or alarm:integer
            if (the_arg.find("alarm") != std::string::npos) {
                check_pt_save_time_alarm = parse_check_pt_alarm_time(the_arg, colon_pos);
            }
            else {
                std::string mode     = the_arg.substr(0, colon_pos);
                std::string interval = the_arg.substr(colon_pos + 1);

                if (mode == "never") {
                    m = ecf::CheckPt::NEVER;
                }
                else if (mode == "on_time") {
                    m = ecf::CheckPt::ON_TIME;
                }
                else if (mode == "always") {
                    m = ecf::CheckPt::ALWAYS;
                }
                else {
                    throw std::runtime_error(MESSAGE(
                        "check_pt: Illegal argument("
                        << the_arg
                        << "), expected [ never | on_time | on_time:<integer> | alarm:<integer> | always | <integer>]\n"
                        << HelpCatalog::description_for("check_pt").value_or(HelpCatalog::not_provided)));
                }
                check_pt_interval = parse_check_pt_interval(interval);
            }
        }
        else {
            if (the_arg == "never") {
                m = ecf::CheckPt::NEVER;
            }
            else if (the_arg == "on_time") {
                m = ecf::CheckPt::ON_TIME;
            }
            else if (the_arg == "always") {
                m = ecf::CheckPt::ALWAYS;
            }
            else {
                check_pt_interval = parse_check_pt_interval(the_arg);
            }
        }
    }

    // testing client interface
    if (ace->under_test()) {
        return;
    }

    if (ace->debug()) {
        std::cout << "  CheckPtCmd::create mode = " << m << " check_pt_interval = " << check_pt_interval << "\n";
    }

    cmd = std::make_shared<CheckPtCmd>(m, check_pt_interval, check_pt_save_time_alarm);
}

std::ostream& operator<<(std::ostream& os, const CheckPtCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(CheckPtCmd)
CEREAL_REGISTER_DYNAMIC_INIT(CheckPtCmd)
