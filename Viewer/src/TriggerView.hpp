//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TRIGGERVIEW_HPP_
#define TRIGGERVIEW_HPP_

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "VInfo.hpp"

class NodeItem : public QGraphicsItem
{
public:
	NodeItem();

	QRectF boundingRect() const;
	void paint(QPainter*, const QStyleOptionGraphicsItem *,QWidget*) {};
};


class TriggerScene : public QGraphicsScene
{
public:
	TriggerScene(QWidget *parent=0);

	void reset(VInfo_ptr info);
};

class TriggerView : public QGraphicsView
{
public:
	TriggerView(QWidget *parent=0);
};

#endif
