//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TreeNodeViewDelegate_HPP_
#define TreeNodeViewDelegate_HPP_

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyledItemDelegate>

#include "NodeViewDelegate.hpp"
#include "VProperty.hpp"

#include <string>

class AnimationHandler;
class PropertyMapper;
class NodeShape;
class NodeText;
class ServerUpdateData;

struct NodeShape
{
    QColor col_;
    QPolygon shape_;
};

struct NodeText
{
    QColor fgCol_;
    QColor bgCol_;
    QRect br_;
    QString text_;
};

struct ServerUpdateData
{
    QRect br_;
    int prevTime_;
    int nextTime_;
    QString prevText_;
    QString nextText_;
    float prog_;
};


class TreeNodeViewDelegate : public NodeViewDelegate
{
Q_OBJECT

public:
	explicit TreeNodeViewDelegate(QWidget *parent=0);
	~TreeNodeViewDelegate();

    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex& index ) const;   
    void setIndentation(int o) {indentation_=o;}

Q_SIGNALS:
    void sizeHintChangedGlobal();

protected:
    void updateSettings();

    int renderServer(QPainter *painter,const QModelIndex& index,
                        const QStyleOptionViewItem& option,QString text) const;

    int renderNode(QPainter *painter,const QModelIndex& index,
                    const QStyleOptionViewItem& option,QString text) const;

    void renderServerCell(QPainter *painter,const NodeShape& stateShape,
                                            const NodeText& text,bool selected) const;

    void renderNodeCell(QPainter *painter,const NodeShape& stateShape,const NodeShape &realShape,
                       const NodeText& nodeText,const NodeText& typeText,bool selected) const;

    void renderNodeShape(QPainter* painter,const NodeShape& shape) const;
    void renderTimer(QPainter *painter,QRect target, int remaining, int total) const;
    void renderServerUpdate(QPainter* painter,const ServerUpdateData&) const;

    QString formatTime(int timeInSec) const;
    QColor interpolate(QColor c1,QColor c2,float r) const;

    enum NodeStyle {ClassicNodeStyle,BoxAndTextNodeStyle};
                    
    AnimationHandler* animation_;

    int nodeRectRad_;
	bool drawChildCount_;
    NodeStyle nodeStyle_;
    int indentation_;

    /*int nodeHeight_;
    int nodeTopMargin_;
    int nodeBottomMargin_;
    int nodeLeftMargin_;
    int nodeRightMargin_;
    int nodeTopPadding_;
    int nodeBottomPadding_;
    int nodeLeftPadding_;
    int nodeRightPadding_;*/

    bool drawNodeType_;
    QColor typeBgCol_;

    QFont serverNumFont_;
	QFont suiteNumFont_;
	QFont serverInfoFont_;
	QFont abortedReasonFont_;
    QFont typeFont_;
    QColor bgCol_;

    QSize nodeSizeHintCache_;
    QSize attrSizeHintCache_;
};

#endif



