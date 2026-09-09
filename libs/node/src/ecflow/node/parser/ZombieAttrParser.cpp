/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/parser/ZombieAttrParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/ZombieAttr.hpp"
#include "ecflow/node/Node.hpp"

using namespace ecf;

bool ZombieAttrParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // expect:  tokenizer
    //    zombie <zombie_type>: action : child_cmds :  lifetime
    //    zombie_type = [ user | ecf | ecf_pid | ecf_pid_passwd | ecf_passwd| path ]  # can only have one
    //    action      = [ fob | fail | block | remove | adopt ]                       # can only have one
    //    child_cmd   = [ init, event, meter, label, wait, abort, complete ]          # can have multiple
    //    zombie ecf:fob::                                                            # fob all child commands
    //    zombie ecf:fail:event,meter:200                                             # fail child command  event,meter
    //    and block other children
    if (lineTokens.size() < 2) {
        throw std::runtime_error("ZombieAttrParser::doParse: Invalid zombie :" + line);
    }
    if (nodeStack().empty()) {
        throw std::runtime_error("Add zombie failed empty node stack");
    }

    // cout << "ZombieAttrParser::doParse: " << lineTokens[1] << "\n";

    nodeStack_top()->addZombie(ZombieAttr::create(lineTokens[1]));

    return true;
}
