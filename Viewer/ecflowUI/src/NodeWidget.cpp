/*
 * Copyright 2009- ECMWF and INPE.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "NodeWidget.hpp"

#include <QAction>
#include <QMenu>

#include "AbstractNodeModel.hpp"
#include "InfoPanelHandler.hpp"
#include "NodeViewBase.hpp"
#include "VFilter.hpp"
#include "VModelData.hpp"

NodeWidget::NodeWidget(const std::string& type, ServerFilter* serverFilter, QWidget* parent)
    : DashboardWidget(type, parent),
      serverFilter_(serverFilter),
      model_(nullptr),
      view_(nullptr),
      icons_(nullptr),
      atts_(nullptr),
      filterDef_(nullptr),
      states_(nullptr),
      broadcastSelection_(true) {
    // Define the icon filter for the model. It controls what icons
    // are displayed next to the nodes. This is exposed via a menu.
    icons_ = new IconFilter;

    // Define the attribute filter for the model. It controls what attributes
    // are displayed for a given node. This is exposed via a menu.
    atts_ = new AttributeFilter;

    createActions();
}

NodeWidget::~NodeWidget() {
    // We only need to delete the non-qobject members
    delete icons_;
    delete atts_;

    if (filterDef_) {
        delete filterDef_;
    }
}

QWidget* NodeWidget::widget() {
    return view_->realWidget();
}

VInfo_ptr NodeWidget::currentSelection() {
    return view_->currentSelection();
}

/*void NodeWidget::currentSelection(VInfo_ptr info)
{
        view_->currentSelection(info);
}*/

void NodeWidget::setCurrentSelection(VInfo_ptr info) {
    if (!detached() && info) {
        // We need to avoid inifinite recursion. This could happen like this:
        // 1. Select node in panel A
        // 2. We broadcast the selection to panel B
        // 3. Panel B selects the node
        // 4  Panel B then brodcasts the selection back to panel A
        //  ...
        // So here we need to check if the given item is already selected!!
        VInfo_ptr csInfo = currentSelection();
        if (csInfo && info && *(csInfo.get()) == *(info.get())) {
            return;
        }

        // This will broadcast the selection!!!
        view_->setCurrentSelection(info);
    }
}

void NodeWidget::slotSelectionChangedInBc(VInfo_ptr info) {
    view_->setCurrentSelection(info);
}

void NodeWidget::reload() {
    active(true);
    // model_->reload();
}

void NodeWidget::active(bool b) {
    model_->active(b);
    view_->realWidget()->setEnabled(b);
}

bool NodeWidget::active() const {
    return model_->active();
}

void NodeWidget::createActions() {
    /*QAction* infoAc=new QAction(" ",this);
    QPixmap pix(":/viewer/dock_info.svg");
    infoAc->setIcon(QIcon(pix));
    infoAc->setToolTip(tr("Start up information panel as dialog"));
    dockActions_ << infoAc;
    dockActionMap_["info"]=infoAc;

    QMenu *menu=new QMenu(this);
    infoAc->setMenu(menu);

    for(std::vector<InfoPanelDef*>::const_iterator it=InfoPanelHandler::instance()->panels().begin();
    it != InfoPanelHandler::instance()->panels().end(); it++)
{
    if((*it)->show().find("toolbar") != std::string::npos)
    {
            QAction *ac=new QAction(QString::fromStdString((*it)->label()) + "...",this);
        //QPixmap pix(":/viewer/" + QString::fromStdString((*it)->dockIcon()));
            QPixmap pix(":/viewer/" + QString::fromStdString((*it)->icon()));
            ac->setIcon(QIcon(pix));
        ac->setData(QString::fromStdString((*it)->name()));
            ac->setEnabled(false);

            connect(ac,SIGNAL(triggered()),
               this,SLOT(slotInfoPanelAction()));

            infoPanelActions_ << ac;
    }
}*/
}

void NodeWidget::slotInfoPanelAction() {
    /*if(QAction* ac=static_cast<QAction*>(sender()))
    {
        Q_EMIT popInfoPanel(view_->currentSelection(),ac->data().toString());
    }*/
}

void NodeWidget::updateActionState(VInfo_ptr /*info*/) {
    /* std::vector<InfoPanelDef*> ids;
     InfoPanelHandler::instance()->visible(info,ids);

     QAction *infoAc=dockActionMap_["info"];
     assert(infoAc);

     QMenu* menu=infoAc->menu();
     assert(menu);

     menu->clear();

     Q_FOREACH(QAction* ac,infoPanelActions_)
     {
         ac->setEnabled(false);

         std::string name=ac->data().toString().toStdString();

         for(std::vector<InfoPanelDef*>::const_iterator it=ids.begin(); it != ids.end(); it++)
         {
                 if((*it)->name() == name)
                 {
                         ac->setEnabled(true);
                         menu->addAction(ac);
                         break;
                 }
         }
     }*/
}
