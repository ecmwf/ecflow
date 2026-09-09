/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VReportMaker.hpp"

#include "OutputFileProvider.hpp"
#include "ServerHandler.hpp"
#include "ShellCommand.hpp"
#include "UiLog.hpp"
#include "VNode.hpp"
#include "VReply.hpp"

VReportMaker::VReportMaker(QObject* parent)
    : QObject(parent) {
    infoProvider_ = new OutputFileProvider(this);
}

void VReportMaker::run(VInfo_ptr info) {
    info_ = info;

    // Get job output
    infoProvider_->info(info);
}

void VReportMaker::infoReady(VReply* reply) {
    Q_ASSERT(reply);
    VFile_ptr f = reply->tmpFile();
    sendJiraReport(f);
    deleteLater();
}

void VReportMaker::infoFailed(VReply*) {
    VFile_ptr f;
    sendJiraReport(f);
    deleteLater();
}

void VReportMaker::sendJiraReport(VFile_ptr file) {
    if (info_->isNode() && info_->node()) {
        VNode* node      = info_->node();
        ServerHandler* s = info_->server();

        if (node && s) {
            std::string filePath = "_undef_";
            if (file) {
                if (file->storageMode() != VFile::DiskStorage) {
                    file->setStorageMode(VFile::DiskStorage);
                }

                filePath = file->path();
            }

            UiLog().dbg() << "REPORT outfile=" << filePath;

            std::string jiraProject = "_undef_";
            if (const char* ch = getenv("ECFLOWUI_JIRA_PROJECT")) {
                jiraProject = std::string(ch);
            }

            std::string cmd = "sh ecflow_ui_create_jira_issue.sh \'" + filePath + "\' " + s->host() + " " + s->port() +
                              " \'" + node->absNodePath() + "\' \'" + node->stateName().toStdString() + "\' " +
                              jiraProject;

            ShellCommand::run(cmd, "");
        }
    }
}

void VReportMaker::sendReport(VInfo_ptr info) {
    auto* maker = new VReportMaker();
    maker->run(info);
    // maker will delete itself when the job is done!
}
