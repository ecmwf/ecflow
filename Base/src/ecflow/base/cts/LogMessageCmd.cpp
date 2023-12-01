/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/Stats.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/base/cts/CtsApi.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

void LogMessageCmd::print(std::string& os) const {
    user_cmd(os, CtsApi::logMsg(msg_));
}
void LogMessageCmd::print_only(std::string& os) const {
    os += CtsApi::logMsg(msg_);
}

bool LogMessageCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<LogMessageCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (msg_ != the_rhs->msg())
        return false;
    return UserCmd::equals(rhs);
}

STC_Cmd_ptr LogMessageCmd::doHandleRequest(AbstractServer* as) const {
    // ***** No need to log message here, already done via print, in base ****
    as->update_stats().log_msg_cmd_++;
    return PreAllocatedReply::ok_cmd();
}

const char* LogMessageCmd::arg() {
    return CtsApi::logMsgArg();
}
const char* LogMessageCmd::desc() {
    return "Writes the input string to the log file.\n"
           "  arg1 = string\n"
           "Usage:\n"
           "  --msg=\"place me in the log file\"";
}

void LogMessageCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(LogMessageCmd::arg(), po::value<string>(), LogMessageCmd::desc());
}

void LogMessageCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ace) const {
    string msg = vm[arg()].as<string>();
    if (ace->debug()) {
        cout << "  LogMessageCmd::create arg = " << msg << "\n";
    }
    cmd = std::make_shared<LogMessageCmd>(msg);
}

std::ostream& operator<<(std::ostream& os, const LogMessageCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}
