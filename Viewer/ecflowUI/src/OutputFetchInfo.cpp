//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputFetchInfo.hpp"

#include <map>

#include <QButtonGroup>
#include <QDebug>
#include <QDir>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "ServerHandler.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"

#include "ui_OutputFetchInfo.h"

OutputFetchInfo::OutputFetchInfo(QWidget* parent) : QWidget(parent),  ui_(new Ui::OutputFetchInfo)
{
    ui_->setupUi(this);

    ui_->te->setMinimumWidth(400);
    ui_->logTe->setMinimumWidth(400);
    ui_->stackedWidget->setCurrentIndex(0);

    bGroup_ = new QButtonGroup(this);
    bGroup_->addButton(ui_->infoTb);
    bGroup_->addButton(ui_->logTb);
    ui_->infoTb->setChecked(true);

    connect(bGroup_, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(buttonClicked(QAbstractButton*)));
}

void OutputFetchInfo::buttonClicked(QAbstractButton* b)
{
    ui_->stackedWidget->setCurrentIndex((b == ui_->infoTb)?0:1);
}

QString OutputFetchInfo::buildList(QStringList lst,bool ordered)
{
    QString t;

    if(lst.count() > 0)
    {
        t+=(ordered)?"<ol>":"<ul>";
        Q_FOREACH(QString s,lst)
        {
            t+="<li>" + s + "</li>";
        }
        t+=(ordered)?"</ol>":"</ul>";
    }

    return t;
}

void OutputFetchInfo::clearInfo()
{
    ui_->te->clear();
    ui_->logTe->clear();
}

void OutputFetchInfo::setInfo(VReply *reply,VInfo_ptr info)
{
    Q_ASSERT(reply);
    ui_->te->clear();

    QString html = makeHtml(reply, info);
    ui_->te->setHtml(html);

    QTextCursor cursor=ui_->te->textCursor();
    cursor.movePosition(QTextCursor::Start);
    ui_->te->setTextCursor(cursor);
}

void OutputFetchInfo::setError(QString err)
{
    ui_->logTe->clear();
    ui_->logTe->appendHtml(err);
    QTextCursor cursor=ui_->logTe->textCursor();
    cursor.movePosition(QTextCursor::Start);
    ui_->logTe->setTextCursor(cursor);
}

void OutputFetchInfo::setError(const std::vector<std::string>& errorVec)
{
    setError(formatErrors(errorVec));
}

QString OutputFetchInfo::formatErrors(const std::vector<std::string>& errorVec) const
{
    QString s;
    if(errorVec.size() > 0)
    {
        QColor col(70,71,72);
        if(errorVec.size() > 1)
        {
            for(size_t i=0; i < errorVec.size(); i++)
                s+=Viewer::formatBoldText("[" + QString::number(i+1) + "] ",col) +
                    QString::fromStdString(errorVec[i]) + ". &nbsp;&nbsp;";
        }
        else if(errorVec.size() == 1)
            s+=QString::fromStdString(errorVec[0]);
    }
    return s.replace("\n", "<br>");
}

void OutputFetchInfo::parseTry(QString s, QString& path, QString& msg)
{
    QRegularExpression rx("<PATH>(.+)<\\/PATH>");
    QRegularExpressionMatch match = rx.match(s);
    if (match.hasMatch()) {
         path = match.captured(1);
    }
    msg = "tried to ";
    msg += s.remove(rx);

    msg.replace(" OK","<font color=\'#269e00\'><b> SUCCEEDED</b></font>");
    msg.replace(" FAILED","<font color=\'#FF0000\'><b> FAILED</b></font>");
    msg.replace(" NO ACCESS","<font><b> NO ACCESS</b></font>");
    msg.replace(" NOT DEFINED","<font><b> NOT DEFINED</b></font>");
}

//==================================================
//
// OutputFileFetchInfo
//
//==================================================

QString OutputFileFetchInfo::makeHtml(VReply *reply,VInfo_ptr info)
{
    Q_ASSERT(reply);

    static QMap<int,QString> nums;
    if(nums.isEmpty())
    {
        nums[1]="1st";
        nums[2]="2nd";
        nums[3]="3rd";
        nums[4]="4th";
    }

    QStringList options;
    QStringList remarks;
    QStringList msg;
    QStringList tries;
    QStringList other;
    QString alg;

    QString html;

    int cnt=1;
    for(const auto & it : reply->log())
    {
        QString s=QString::fromStdString(it);
        if(s.startsWith("REMARK>"))
        {
            remarks << s.remove(0,7);
            continue;
        }
        else if(s.startsWith("OPTION>"))
        {
            options << s.remove(0,7);
            continue;
        }
        else if(s.startsWith("MSG>"))
        {
            msg << s.remove(0,4);
            continue;
        }
        else if(s.startsWith("TRY>"))
        {
            s.remove(0,4);
            QString path, msg;
            parseTry(s, path, msg);
            tries << msg;
            cnt++;
            continue;
        }
        else
            other << s;
    }

    if(info && info->server())
    {
        ServerHandler* server=info->server();
        bool rfd=server->readFromDisk();
        QString t;

        t="The following steps are tried in order to fetch the output files:<ul>";
        t+="<li>fetch from the logserver (if defined)</li>";

        bool proxy = VConfig::instance()->proxychainsUsed();
        if(rfd)
        {
            if (proxy) {
                t+="<li>read from disk on remote SOCKS host via ssh/scp</li>";
            } else {
                t+="<li>read from disk</li>";
            }
        }
        else
        {
            if (proxy) {
                t+="<li>read from disk on remote SOCKS host via ssh/scp (if <b>not</b> the <b>current</b> job output)</li>";
            } else {
                t+="<li>read from disk (if <b>not</b> the <b>current</b> job output)</li>";
            }
        }

        t+="<li>fetch from the ecflow server (if the <b>current</b> job output) </li> ";

        t+="</ul> (To enable/disable the read from disk option go Tools -> Preferences -> Server settings -> Fetching files)";

        alg=t;

        if(reply->tmpFile() && reply->fileReadMode() == VReply::LocalReadMode &&
            !server->isLocalHost())
        {
            remarks << "The output file was read <b>from disk</b> but the server (" +
                       QString::fromStdString(server->host()) +
                       ") is not running on the local machine. If the path is machine-specific (e.g. /tmp) \
                       and there exists a file with the same path on the local machine, then\
                       this will have been read instead.";
        }
    }

    if(!msg.isEmpty())
    {
       html+="<p><u>Messages</u></p>";
       html+=buildList(msg);
    }

    if(!options.isEmpty())
    {
        html+="<p><u>Options</u></p>";
        html+=buildList(options);
    }

    if(!tries.isEmpty())
    {
        html+="<p><u>How was this file fetched?</u></p>";
        html+=buildList(tries,true);
    }

    if(!alg.isEmpty())
    {
       html+="<p><u>Algorithm:</u></p>"+alg;
    }

    if(!remarks.isEmpty())
    {
        html+="<p><u>Remarks</u></p>";
        html+=buildList(remarks);
    }

    if(!other.isEmpty())
    {
        html+=buildList(other);

    }

    return html;
}

//==================================================
//
// OutputDirFetchInfo
//
//==================================================

QString OutputDirFetchInfo::makeHtml(VReply *reply,VInfo_ptr /*info*/)
{
    Q_ASSERT(reply);

    static QMap<int,QString> nums;
    if(nums.isEmpty())
    {
        nums[1]="1st";
        nums[2]="2nd";
        nums[3]="3rd";
        nums[4]="4th";
    }

    std::map<QString, QStringList> tries;
    QStringList options;
    QStringList remarks;
    QStringList msg;
    QStringList other;
    QString alg;
    QString html;

    int cnt=1;
    for(const auto & it : reply->log())
    {
        QString s=QString::fromStdString(it);
        if(s.startsWith("REMARK>"))
        {
            remarks << s.remove(0,7);
            continue;
        }
        else if(s.startsWith("OPTION>"))
        {
            options << s.remove(0,7);
            continue;
        }
        else if(s.startsWith("MSG>"))
        {
            msg << s.remove(0,4);
            continue;
        }
        else if(s.startsWith("TRY>"))
        {
            s.remove(0,4);
            QString path, msg;
            parseTry(s, path, msg);
            path = makeSearchPath(path);
            tries[path] << msg;
            cnt++;
            continue;
        }
        else
            other << s;
    }

    if(!msg.isEmpty())
    {
       html+="<p><u>Messages</u></p>";
       html+=buildList(msg);
    }

    if(!options.isEmpty())
    {
        html+="<p><u>Options</u></p>";
        html+=buildList(options);
    }

    if(!tries.empty())
    {
        html+="<p><u>How was this directory listing fetched?</u></p>";
        for (auto it: tries) {
            html+="pattern: <i>" + it.first + "</i>";
            html+=buildList(it.second,true);
        }
    }

    if(!alg.isEmpty())
    {
       html+="<p><u>Algorithm:</u></p>"+alg;
    }

    if(!remarks.isEmpty())
    {
        html+="<p><u>Remarks</u></p>";
        html+=buildList(remarks);
    }

    if(!other.isEmpty())
    {
        html+=buildList(other);

    }
    return html;
}

QString OutputDirFetchInfo::makeSearchPath(QString path) const
{
    QFileInfo f(path);
    auto d = f.dir();
    auto name = f.baseName();
    return d.filePath(name + ".*");
}


