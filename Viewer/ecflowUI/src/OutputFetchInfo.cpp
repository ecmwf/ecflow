//============================================================================
// Copyright 2009-2017 ECMWF.
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
    QStringList msg;
    QStringList tries;
    QStringList other;
    QString alg;

    QString html;

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
        else if(s.startsWith("MSG>"))
        {
            msg << s.remove(0,4);
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
        bool rfd=server->readFromDisk();
        QString t;

        t="The following are tried in order:<ul>";

        if(rfd)
        {
            t+="<li>Try to read the output files from the logserver \
               (if defined)</li><li>from disk</li><li>\
               through the ecflow server (if the <b>current</b> job output) </li>";
        }
        else
        {
            t+="<li>Try to read the output files from the logserver \
               (if defined)</li><li>from disk (if <b>not</b> the <b>current</b> job output)</li>\
               <li>from the ecflow server (if the <b>current</b> job output)</li> ";
        }
        t+="</ul> (To change this behaviour go Tools -> Preferences -> Server settings -> Fetching files)";

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
