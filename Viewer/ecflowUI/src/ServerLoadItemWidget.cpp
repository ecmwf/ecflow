//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerLoadItemWidget.hpp"

#include <QVBoxLayout>

#include "Node.hpp"

#include "InfoProvider.hpp"

#ifdef ECFLOW_LOGVIEW
#include "LogLoadWidget.hpp"
#endif

#include "MessageLabel.hpp"
#include "ServerHandler.hpp"
#include "SuiteFilter.hpp"
#include "UiLog.hpp"
#include "VNode.hpp"
#include "VNState.hpp"
#include "VSettings.hpp"

ServerLoadItemWidget::ServerLoadItemWidget(QWidget *parent)
{
    auto* vb=new QVBoxLayout(this);
    vb->setContentsMargins(0,0,0,0);
#ifdef ECFLOW_LOGVIEW
    w_=new LogLoadWidget(this);
    vb->addWidget(w_);
#else
    w_=new MessageLabel(this);
    w_->showWarning("The <b>server load view</b> is only avialable when ecFlowUI is built with <b>QtCharts</b>.");
    vb->addWidget(w_);
    vb->addStretch(1);
#endif

    //This tab is always visible whatever node is selected!!!
    //We keep the data unchanged unless a new server is selected
    keepServerDataOnLoad_=true;

    //We will KEEP the contents when the item (aka tab) becomes unselected
    //unselectedFlags_.clear();
}

ServerLoadItemWidget::~ServerLoadItemWidget()
{
    clearContents();
}

QWidget* ServerLoadItemWidget::realWidget()
{
    return this;
}

void ServerLoadItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    if(info && info->server() && info->server()->isDisabled())
    {
        setEnabled(false);
        return;
    }
    else
    {
        setEnabled(true);
    }

    bool same=hasSameContents(info);

    //set the info. We do not need to observe the node!!!
    info_=info;

    if(!same)
        load();
}

void ServerLoadItemWidget::load()
{
#ifdef ECFLOW_LOGVIEW
    if(info_ && info_->server())
    {
        ServerHandler *sh=info_->server();
        Q_ASSERT(sh);

        if(sh->activity() == ServerHandler::LoadActivity)
        {
            delayedLoad_=true;
        }
        else
        {
            if(VServer* vs=sh->vRoot())
            {
                QString logFile=QString::fromStdString(vs->findVariable("ECF_LOG",false));
                if(!logFile.isEmpty())
                {
                    std::vector<std::string> suites;
                    if(SuiteFilter* sf=sh->suiteFilter())
                    {
                        if(sf->isEnabled())
                            suites=sh->suiteFilter()->filter();
                    }

                    //last 100 MB are read
                    w_->initLoad(QString::fromStdString(sh->name()),
                         QString::fromStdString(sh->host()),
                         QString::fromStdString(sh->port()),
                         logFile, suites, sh->uidForServerLogTransfer(),
                         sh->maxSizeForTimelineData(),
                         info_->nodePath(), detached_);//last 100 MB are read*/

                    delayedLoad_=false;

                }
            }
        }
//        QString logFile;
//        if(VServer* vs=sh->vRoot())
//        {
//            logFile=QString::fromStdString(vs->findVariable("ECF_LOG",false));

//            w_->load(QString::fromStdString(sh->name()),
//                         QString::fromStdString(sh->host()),
//                         QString::fromStdString(sh->port()),
//                         logFile,-50000); //last 50000 rows are read
//        }
    }
#endif
}

void ServerLoadItemWidget::clearContents()
{
#ifdef ECFLOW_LOGVIEW
    w_->clear();
    delayedLoad_=false;
#endif
    InfoPanelItem::clear();
}

bool ServerLoadItemWidget::hasSameContents(VInfo_ptr info)
{
    if(info && info_ && info->server())
    {
        return info->server() == info_->server();
    }
    return false;
}

//We are independent of the server's state
void ServerLoadItemWidget::serverSyncFinished()
{
#ifdef ECFLOW_LOGVIEW
    if(delayedLoad_)
        load();
#endif
}

void ServerLoadItemWidget::connectStateChanged()
{
    if(frozen_)
        return;
#ifdef ECFLOW_LOGVIEW
    if(delayedLoad_)
        load();
#endif
}


//We are independent of the server's state
void ServerLoadItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
#ifdef ECFLOW_LOGVIEW
    if(flags.isSet(SuspendedChanged))
    {
        //Suspend
        if(suspended_)
        {
            //reloadTb_->setEnabled(false);
        }
        //Resume
        else
        {
            if(info_ && info_->node())
            {
                //reloadTb_->setEnabled(true);
                if(delayedLoad_)
                    load();
            }
            else
            {
                clearContents();
            }
        }
    }

    if(flags.isSet(DetachedChanged))
    {
        w_->setDetached(detached_);
//        if(!detached_ && info_)
//            w_->selectPathInView(info_->nodePath());
    }
#endif
}

void ServerLoadItemWidget::writeSettings(VComboSettings* vs)
{
    vs->beginGroup("serverload");
    w_->writeSettings(vs);
    vs->endGroup();
}

void ServerLoadItemWidget::readSettings(VComboSettings* vs)
{
    vs->beginGroup("serverload");
    w_->readSettings(vs);
    vs->endGroup();
}


static InfoPanelItemMaker<ServerLoadItemWidget> maker1("server_load");
