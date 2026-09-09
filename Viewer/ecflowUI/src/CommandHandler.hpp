/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_CommandHandler_HPP
#define ecflow_viewer_CommandHandler_HPP

#include "VInfo.hpp"

// Class to interpret generic commands and send them
//  1. to ServerHandler if they are "ecflow_client" commands
//  2. to ShellCommand if they are shell commands

class CommandHandler {
public:
    static void run(std::vector<VInfo_ptr>, const std::string&);
    static void run(VInfo_ptr, const std::vector<std::string>&);
    static void run(VInfo_ptr, const std::string&);
    static void openLinkInBrowser(VInfo_ptr);
    static void executeAborted(const std::vector<VNode*>& info_vec);
    static void rerunAborted(const std::vector<VNode*>& info_vec);
    static bool kill(const std::vector<VNode*>& info_vec, bool confirm);
    static const std::string& executeCmd() { return executeCmd_; }
    static const std::string& rerunCmd() { return rerunCmd_; }

protected:
    static std::string commandToString(const std::vector<std::string>& cmd);
    static void substituteVariables(std::string& cmd, const std::vector<VInfo_ptr>& info);

    static std::string executeCmd_;
    static std::string rerunCmd_;
    static std::string killCmd_;
};

#endif /* ecflow_viewer_CommandHandler_HPP */
