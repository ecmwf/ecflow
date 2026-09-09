/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VNodeStateDiag.hpp"

#include "DirectoryHandler.hpp"
#include "ServerHandler.hpp"
#include "ShellCommand.hpp"
#include "UiLog.hpp"
#include "VFile.hpp"
#include "VNode.hpp"

VNodeStateDiag::VNodeStateDiag(VInfo_ptr info) {
    if (!info) {
        return;
    }

    if (info->isNode() && info->node()) {
        VNode* node      = info->node();
        ServerHandler* s = info->server();

        if (node && s) {
            VFile_ptr tmpFile  = VFile::createTmpFile(false);
            VFile_ptr tmpFile2 = VFile::createTmpFile(false);

            s->writeDefs(info, tmpFile->path());
            std::string cmd = "sh ecflow_ui_node_state_diag.sh \'" + tmpFile->path() + "\' " + s->host() + " " +
                              s->port() + " \'" + info->nodePath() + "\' \'" + tmpFile2->path() + "\'";

            ShellCommand::run(cmd, "");
        }
    }
}
