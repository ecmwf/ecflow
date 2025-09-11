/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "InfoProvider.hpp"

#include <fstream>
#include <stdexcept>

#include <QDateTime>

#include "ServerHandler.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "ecflow/node/EcfFile.hpp"
#include "ecflow/node/Submittable.hpp"

InfoPresenter::~InfoPresenter() {
    // The following takes care of deleting the provider used by this presenter.
    // The provider is typically created in the ctor of derived class (e.g. WhyItemWidget)
    delete infoProvider_;
}

InfoProvider::InfoProvider(InfoPresenter* owner, VTask::Type taskType)
    : owner_(owner),
      reply_(new VReply(this)),
      taskType_(taskType) {
    if (owner_) {
        owner_->registerInfoProvider(this);
    }
}

InfoProvider::~InfoProvider() {
    clearInternal();
    delete reply_;
}

void InfoProvider::clearInternal() {
    if (task_) {
        task_->status(VTask::CANCELLED);
    }
    if (reply_) {
        reply_->reset();
    }
    info_.reset();
}

void InfoProvider::clear() {
    clearInternal();
    //    if (task_) {
    //		task_->status(VTask::CANCELLED);
    //    }
    //    if (reply_) {
    //        reply_->reset();
    //    }
    //	info_.reset();
}

void InfoProvider::setActive(bool b) {
    active_ = b;
    if (!active_) {
        clear();
    }
}

void InfoProvider::setAutoUpdate(bool b) {
    autoUpdate_ = b;
}

void InfoProvider::info(VInfo_ptr info) {
    // We keep it alive
    info_ = info;

    if (task_) {
        task_->status(VTask::CANCELLED);
        task_.reset();
    }

    if (owner_ && info_) {
        info_->accept(this);
    }
}

// Server
void InfoProvider::visit(VInfoServer* info) {
    reply_->reset();

    if (!info->server()) {
        owner_->infoFailed(reply_);
    }
    else {
        // Define a task for getting the info from the server.
        task_ = VTask::create(taskType_, this);

        // Run the task in the server. When it completes taskFinished() is called. The text returned
        // in the reply will be prepended to the string we generated above.
        info->server()->run(task_);
    }
}

// Node
void InfoProvider::visit(VInfoNode* info) {
    reply_->reset();

    if (!info->node() || !info->node()->node()) {
        owner_->infoFailed(reply_);
    }

    // Check if we have a server
    if (!info->server()) {
        owner_->infoFailed(reply_);
    }

    VNode* n = info->node();

    std::string fileName;
    if (!fileVarName_.empty()) {
        // Get the fileName
        fileName = n->genVariable(fileVarName_) + fileSuffix_;
    }

    // We try to read the file directly from the disk
    if (info->server()->readFromDisk()) {
        // There is a variable defined for the filename
        if (!fileName.empty()) {
            if (reply_->textFromFile(fileName)) {
                reply_->fileReadMode(VReply::LocalReadMode);
                reply_->fileName(fileName);
                owner_->infoReady(reply_);
                return;
            }
            /*else if(handleFileMissing(fileName,reply_))
            {
                    return;
            }*/
        }
    }

    // We try to get the file contents from the server
    //(this will go through the threaded communication)

    // Define a task for getting the info from the server.
    task_ = VTask::create(taskType_, n, this);
    task_->reply()->fileName(fileName);
    task_->reply()->fileReadMode(VReply::ServerReadMode);

    // Run the task in the server. When it finish taskFinished() is called. The text returned
    // in the reply will be prepended to the string we generated above.
    info->server()->run(task_);
}

void InfoProvider::handleFileNotDefined(VReply* reply) {
    reply->setInfoText(fileNotDefinedText_);
    owner_->infoReady(reply_);
}

bool InfoProvider::handleFileMissing(const std::string& /*fileName*/, VReply* /*reply*/) {
    return false;

    // reply->setWarningText(fileMissingText_);
    // owner_->infoReady(reply_);
}

void InfoProvider::taskChanged(VTask_ptr task) {
    if (task_ != task) {
        return;
    }

    // temporary hack!
    task_->reply()->setSender(this);

    switch (task->status()) {
        case VTask::FINISHED: {
            task->reply()->addLog("TRY>fetch file from ecflow server: OK");

            // The file should have a copy of the reply log
            VFile_ptr f = task_->reply()->tmpFile();
            if (f) {
                f->setFetchDate(QDateTime::currentDateTime());
                f->setFetchMode(VFile::ServerFetchMode);
                f->setLog(task_->reply()->log());
            }
            task->reply()->status(VReply::TaskDone);
            owner_->infoReady(task->reply());
            task_.reset();
        } break;
        case VTask::ABORTED:
        case VTask::REJECTED:
            task->reply()->addLog("TRY>fetch file from ecflow server: FAILED");
            task->reply()->status(VReply::TaskFailed);
            owner_->infoFailed(task->reply());
            task_.reset();
            break;
        case VTask::CANCELLED:
            if (!task->reply()->errorText().empty()) {
                task->reply()->addLog("TRY>fetch file from ecflow server: FAILED");
                task->reply()->status(VReply::TaskCancelled);
                owner_->infoFailed(task->reply());
            }
            // We do not need the task anymore.
            task_.reset();
            break;
        default:
            break;
    }
}

JobProvider::JobProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::JobTask) {
    fileVarName_ = "ECF_JOB";

    fileNotDefinedText_ = "Job is <b>not</b> defined";

    fileMissingText_ = "Job <b>not</b> found! <br> Check <b>ECF_HOME</b> directory  \
				 for read/write access. Check for file presence and read access below. \
	             The file may have been deleted or this may be a '<i>dummy</i>' task";
}

bool JobProvider::handleFileMissing(const std::string& fileName, VReply* reply) {
    if (fileName.find(".job0") != std::string::npos) {
        reply->setInfoText("No job to be expected when <b>TRYNO</b> is 0!");
        owner_->infoReady(reply_);
        return true;
    }
    return false;
}

JobStatusFileProvider::JobStatusFileProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::JobStatusFileTask) {
    fileVarName_        = "ECF_JOB";
    fileSuffix_         = ".stat";
    fileNotDefinedText_ = "Job status is <b>not</b> defined";
    fileMissingText_    = "Job status file <b>not</b> found! <br> Check <b>ECF_HOME</b> directory  \
                 for read/write access. Check for file presence and read access below. \
                 The file may have been deleted or this may be a '<i>dummy</i>' task";
}

bool JobStatusFileProvider::handleFileMissing(const std::string& fileName, VReply* reply) {
    if (fileName.find(".job0") != std::string::npos) {
        reply->setInfoText("No job to be expected when <b>TRYNO</b> is 0!");
        owner_->infoReady(reply_);
        return true;
    }
    return false;
}

JobStatusProvider::JobStatusProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::JobStatusTask) {
}

ManualProvider::ManualProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::ManualTask) {
    fileVarName_        = "ECF_MANUAL";
    fileNotDefinedText_ = "Manual is <b>not</b> available";
    fileMissingText_    = "Manual is <b>not</b> available";
}

MessageProvider::MessageProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::MessageTask) {
}

ScriptProvider::ScriptProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::ScriptTask) {
    fileVarName_        = "ECF_SCRIPT";
    fileNotDefinedText_ = "Script is <b>not</b> defined";
    fileMissingText_    = "Script <b>not/b> found! <br> Check <b>ECF_FILES</b> or <b>ECF_HOME</b> directories,  \
			 for read access. Check for file presence and read access below files directory \
             or this may be a '<i>dummy</i>' task";
}

void ScriptProvider::visit(VInfoNode* info) {
    reply_->reset();

    if (!info->node() || !info->node()->node()) {
        owner_->infoFailed(reply_);
    }

    // Check if we have a server
    if (!info->server()) {
        owner_->infoFailed(reply_);
    }

    VNode* n = info->node();

    std::string fileName;

    // We try to read the file directly from the disk
    // Try client first.
    // *THIS will minimize calls to the server. when .ecf is not in ECF_SCRIPT but still accessible from the client
    try {
        if (Submittable* sb = info->node()->node()->isSubmittable()) {
            EcfFile ecf_file = sb->locatedEcfFile(); // will throw std::runtime_error for errors
            std::string fileContents, fileName, fileMethod;
            ecf_file.script(fileContents);

            // Check extra info in the first line
            std::string pattern("# ecf_script_origin :");
            if (fileContents.size() > pattern.size() && fileContents.substr(0, pattern.size()) == pattern) {
                // strip off the first line
                std::string::size_type pos = fileContents.find('\n');
                if (pos != std::string::npos && fileContents.size() > pos) {
                    QString s        = QString::fromStdString(fileContents.substr(0, pos));
                    QStringList sLst = s.split(":");
                    if (sLst.count() == 3) {
                        fileMethod = sLst[1].simplified().toStdString();
                        fileName   = sLst[2].simplified().toStdString();
                    }

                    fileContents = fileContents.substr(pos + 1);
                }
            }

            else {
                fileName = ecf_file.script_path_or_cmd();
            }

            reply_->text(fileContents);
            reply_->fileReadMode(VReply::LocalReadMode);
            reply_->fileName(fileName);
            reply_->fileReadMethod(fileMethod);
            owner_->infoReady(reply_);
            return;
        }
    }

    catch (std::exception& e) {
        // Try the server
    }

    // We try to get the file contents from the server
    //(this will go through the threaded communication)

    if (!fileVarName_.empty()) {
        // Get the fileName
        fileName = n->genVariable(fileVarName_) + fileSuffix_;
    }

    // Define a task for getting the info from the server.
    task_ = VTask::create(taskType_, n, this);
    task_->reply()->fileName(fileName);
    task_->reply()->fileReadMode(VReply::ServerReadMode);

    // Run the task in the server. When it finish taskFinished() is called. The text returned
    // in the reply will be prepended to the string we generated above.
    info->server()->run(task_);
}

HistoryProvider::HistoryProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::HistoryTask) {
}

SuiteProvider::SuiteProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::SuiteListTask) {
}

ZombieProvider::ZombieProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::ZombieListTask) {
}

WhyProvider::WhyProvider(InfoPresenter* owner) : InfoProvider(owner, VTask::WhySyncTask) {
}
