//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputFetchInfo.hpp"

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QString>
#include <QFile>

#include <QDebug>

#include "ServerHandler.hpp"

OutputFetchInfo::OutputFetchInfo(QWidget* parent) : QWidget(parent)
{
    QVBoxLayout *vb=new QVBoxLayout(this);
    label_=new QLabel(this);
    label_->setText("<b>Additional information</b>");

    te_=new QTextEdit(this);
    te_->setReadOnly(true);
    te_->setMinimumWidth(350);

    vb->addWidget(label_);
    vb->addWidget(te_,1);
}

void OutputFetchInfo::setInfo(VReply *reply,VInfo_ptr info)
{
    Q_ASSERT(reply);

    te_->clear();

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
    QStringList tries;
    QStringList other;

    QString html;

    if(info && info->server())
    {
        ServerHandler* server=info->server();
        bool rfd=server->readFromDisk();
        QString t;
        if(rfd)
        {
            t="With the current settings the viewer tries to read the ouput files <b>from disk</b>, if it fails it tries the <b>logserver</b> (if defined) and finally the <b>server</b>.";
        }
        else
        {
            t="With the current settings the viewer tries to read the ouput files from the <b>log server</b> (if defined), then from the <b>server</b>.";
        }
        t+=" (To change this setting go Edit -> Preferences -> Server options -> Files)";

        html+=t;
    }

    int cnt=1;
    for(std::vector<std::string>::const_iterator it=reply->log().begin(); it != reply->log().end(); ++it)
    {
        QString s=QString::fromStdString(*it);
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
        else if(s.startsWith("TRY>"))
        {
            s.remove(0,4);
            s.prepend("tried to ");
            s.replace(" OK","<font color=\'#269e00\'><b> SUCCEEDED</b></font>");
            s.replace(" FAILED","<font color=\'#FF0000\'><b> FAILED</b></font>");
            s.replace(" NO ACCESS","<font><b> NO ACCESS</b></font>");
            s.replace(" NOT DEFINED","<font><b> NOT DEFINED</b></font>");
            tries << s;
            cnt++;
            continue;
        }
        else
            other << s;
    }


    if(info && info->server())
    {
        ServerHandler* server=info->server();
        if(reply->fileReadMode() == VReply::LocalReadMode &&
            !server->isLocalHost())
        {
            remarks << "The ouput file was read <b>from disk</b> but the server's host (" + QString::fromStdString(server->host()) +
                   ") is not the local host. Therefore it is possible that output file is not"
                   " located on the disk, but there is a file with the same name on disk. So we might have read the wrong file";
        }
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

    if(!remarks.isEmpty())
    {  
        html+="<p><u>Remarks</u></p>";
        html+=buildList(remarks);
    }

    if(!other.isEmpty())
    {       
        html+=buildList(other);

    }

    te_->setHtml(html);

    QTextCursor cursor=te_->textCursor();
    cursor.movePosition(QTextCursor::Start);
    te_->setTextCursor(cursor);
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
    te_->clear();
}
