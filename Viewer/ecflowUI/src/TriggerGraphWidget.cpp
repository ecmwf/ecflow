//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerGraphWidget.hpp"

#include "ui_TriggerGraphWidget.h"

#include <algorithm>

#include "TriggerGraphView.hpp"
#include "TriggerItemWidget.hpp"
#include "TriggerGraphModel.hpp"
#include "TriggerGraphDelegate.hpp"
#include "TriggerGraphLayout.hpp"
#include "UiLog.hpp"
#include "VSettings.hpp"




TriggerGraphWidget::TriggerGraphWidget(QWidget* parent) :
    QWidget(parent),
    ui_(new Ui::TriggerGraphWidget)
{
    ui_->setupUi(this);
    //scene_ = new TriggerGraphScene(this);
    //ui_->view->setScene(scene_);
    // the bg can only be set correctly when the scene is set on the view
    //ui_->view->adjustBackground();

    //model_ = new TriggerGraphModel(TriggerGraphModel::TriggerMode,this);
    //ui_->view->setModel(model_);

    //delegate_ = new TriggerGraphDelegate(this);

    //layout_ = new TriggerGraphLayout(ui_->view);

    //nodeCollector_=new TriggerTableCollector(false);

    //relay commands
    connect(ui_->view,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
            this,SIGNAL(infoPanelCommand(VInfo_ptr,QString)));

    connect(ui_->view,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
            this,SIGNAL(dashboardCommand(VInfo_ptr,QString)));
}

TriggerGraphWidget::~TriggerGraphWidget()
{
    clear();
    //delete layout_;
}

void TriggerGraphWidget::clear()
{
    info_.reset();
    ui_->view->clear();
}

void TriggerGraphWidget::setInfo(VInfo_ptr info, bool dependency)
{
    info_=info;
    dependency_ = dependency;
    scan();
    //nodeModel_->beginUpdate();
    //nodeCollector_->clear();
    //if(info_)
    //{
    //    nodeCollector_->add(info_->item(),nullptr,TriggerCollector::Normal);
    //}
    //nodeModel_->setTriggerCollector(nodeCollector_);
    //nodeModel_->endUpdate();

    //model_->setNode(info);

    //ui_->view->setInfo(info);
}

void TriggerGraphWidget::adjust(VInfo_ptr info, bool dependency, TriggerTableCollector* tc1, TriggerTableCollector* tc2)
{
    if (!info) {
        clear();
    } else if(info_ != info) {
        setInfo(info, dependency);
        //scan();
//        beginTriggerUpdate();
//        setTriggerCollector(tc1,tc2);
//        endTriggerUpdate();
    }
}

void TriggerGraphWidget::setTriggerCollector(TriggerTableCollector *tc1,TriggerTableCollector *tc2)
{
//    model_->setTriggerCollectors(tc1, tc2);
//    triggerTc_ = tc1;
//    triggeredTc_ = tc2;
}


void TriggerGraphWidget::beginTriggerUpdate()
{
    //model_->beginUpdate();
}

void TriggerGraphWidget::endTriggerUpdate()
{
    //model_->endUpdate();

    //scan();

//    TriggerGraphNodeItem* item =new TriggerGraphNodeItem(model_->index(0,0), delegate_, 0, nullptr);
//    scene_->addItem(item);

//    for(int i=1; i < model_->rowCount(); i++) {
//        TriggerGraphNodeItem* itemN =new TriggerGraphNodeItem(model_->index(i,0), delegate_, -1, nullptr);
//        scene_->addItem(itemN);

//        TriggerGraphEdgeItem* itemE =new TriggerGraphEdgeItem(itemN, item, nullptr);
//        scene_->addItem(itemE);
//    }
}

void TriggerGraphWidget::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& aspect)
{
    ui_->view->nodeChanged(node, aspect);
}

void TriggerGraphWidget::scan()
{
    if (!info_ || !info_->node())
        return;

    VNode *node = info_->node();
    Q_ASSERT(node);
    ui_->view->scan(node, dependency_);

//    UiLog().dbg() << model_->rowCount() << layout_->nodes_.size();

//    if(VNode *p = node->parent()) {
//        layout_->addRelation(p, node, nullptr, TriggerCollector::Hierarchy, nullptr);
//        scan(p);
//    }

}

void TriggerGraphWidget::setTriggeredScanner(TriggeredScanner* scanner)
{
    ui_->view->setTriggeredScanner(scanner);
}

//void TriggerGraphWidget::scan(VNode* node)
//{
//    Q_ASSERT(node);

//    //UiLog().dbg() << model_->rowCount() << " " << layout_->nodes_.size();
//    //Q_ASSERT(model_->rowCount() == layout_->nodes_.size());
//    size_t oriNum = model_->rowCount(); //layout_->nodes_.size();

//    // add the node to the layout
//    layout_->addNode(node);

//    // performs the trigger collection - it will populate the layout
//    TriggerRelationCollector tc(node, layout_, false);
//    node->triggers(&tc);

////    std::vector<VItem*> items;
////    for(size_t i=oriNum; i < layout_->nodes_.size(); i++) {
////        items.push_back(layout_->nodes_[i]->item_);
////    }

//    model_->setItems(layout_->nodes());

//    for(int i=0; i < model_->rowCount(); i++) {
//        layout_->nodes()[i]->addGrItem(model_->index(i,0), delegate_);

////        if (i < oriNum) {
////            nodes_[i]->setIndex(model_->index(i,0));
////        } else {
////            TriggerGraphNodeItem* itemN =
////                    new TriggerGraphNodeItem(model_->index(i,0), delegate_, -1, nullptr);
////            nodes_.push_back(itemN);
////            layout_->nodes_[i]->width_ = itemN->boundingRect().width();
////            layout_->nodes_[i]->height_ = itemN->boundingRect().height();
////        }
//    }

//    layout_->build();

//    layout_->postBuild();

////    for(size_t i=0; i < nodes_.size(); i++) {
////        nodes_[i]->setPos(QPoint(layout_->nodes_[i]->tmpX_, layout_->nodes_[i]->tmpY_));
////        if (i >= oriNum)
////            scene_->addItem(nodes_[i]);
////    }

////    for(size_t i=0; i < edges_.size(); i++) {
////        scene_->removeItem(edges_[i]);
////        delete edges_[i];
////    }
////    edges_.clear();

////    for(auto e: layout_->edges_) {
////        TriggerGraphEdgeItem* itemE =new TriggerGraphEdgeItem(nodes_[e->from_], nodes_[e->to_], nullptr);
////        if (e->mode_ == TriggerCollector::Normal) {
////            itemE->setPen(ui_->view->triggerConnectPen_);
////        } else if(e->mode_ == TriggerCollector::Hierarchy) {
////            itemE->setPen(ui_->view->parentConnectPen_);
////        } else {
////            itemE->setPen(ui_->view->depConnectPen_);
////        }
////        edges_.push_back(itemE);
////        scene_->addItem(itemE);
////    }

//    ui_->view->adjustSceneRect();
//}


//void TriggerGraphWidget::addRelation(VItem* from, VItem* to,
//                VItem* through, TriggerCollector::Mode mode, VItem *trigger)
//{
//    TriggerGraphNodeItem* from_gr = addNode(from);
//    TriggerGraphNodeItem* to_gr = addNode(to);

//    from_gr->addRelation(to_gr);

//  node_relation* n = (node_relation*)from_g->relation_data(to_g);
//  while(n)
//    {
//      if(n->trigger_ == trigger &&
//     n->through_ == through &&
//     n->mode_    == mode)
//    break;

//      n = n->next_;
//    }

//  if(n == 0) {

//    n = new node_relation(trigger,through,mode);
//    relations_.add(n);

//    void* x = from_g->relation_data(to_g,n);
//        if(x) n->next_ = (node_relation*)x;
//  }

//  switch(mode)
//    {
//    case trigger_lister::normal:
//      break;

//    case trigger_lister::child:
//      /* from_g->relation_gc(to_g,gui::colorGC(STATUS_SUBMITTED)); */
//      from_g->relation_gc(to_g,gui::blueGC());
//      break;

//    case trigger_lister::parent:
//      //from_g->relation_gc(to_g,gui::colorGC(STATUS_COMPLETE));
//      from_g->relation_gc(to_g,gui::blueGC());
//      break;

//    case trigger_lister::hierarchy:
//      from_g->relation_gc(to_g,gui::colorGC(STATUS_ABORTED));
//      break;
//    }
//}


//void TriggerGraphWidget::scan()
//{
//    TriggerGraphLayout layout;

//    TriggerGraphNodeItem* item =new TriggerGraphNodeItem(model_->index(0,0), delegate_, 0, nullptr);
//    scene_->addItem(item);
//    TriggerGraphLayoutNode* n = new TriggerGraphLayoutNode(item);
//    layout.items_.push_back(n);

//    QList<TriggerGraphNodeItem*> nodes;

//    for(int i=1; i < model_->rowCount(); i++) {
//        TriggerGraphNodeItem* itemN =new TriggerGraphNodeItem(model_->index(i,0), delegate_, -1, nullptr);
//        scene_->addItem(itemN);
//        nodes << itemN;

//        TriggerGraphLayoutNode* m = new TriggerGraphLayoutNode(itemN);
//        n->parents_.push_back(m);
//        m->children_.push_back(n);
//        layout.items_.push_back(m);

//        //TriggerGraphEdgeItem* itemE =new TriggerGraphEdgeItem(itemN, item, nullptr);
//        //scene_->addItem(itemE);
//    }

//    layout.build();

//    for (auto n: layout.items_) {
//        n->item_->setPos(QPointF(n->tmpX_, n->tmpY_));
//    }

//    for(int i=0; i < nodes.count(); i++) {
//        TriggerGraphEdgeItem* itemE =new TriggerGraphEdgeItem(nodes[i], item, nullptr);
//        scene_->addItem(itemE);
//    }

//    ui_->view->adjustSceneRect();
//}
