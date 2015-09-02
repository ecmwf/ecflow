//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TABLENODEVIEWDELEGATE_HPP
#define TABLENODEVIEWDELEGATE_HPP

#include <QBrush>
#include <QMap>
#include <QPen>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

#include "NodeViewDelegate.hpp"
#include "VProperty.hpp"

#include <string>

class ModelColumn;

class TableNodeViewDelegate : public NodeViewDelegate   //, public QStyledItemDelegate, public VPropertyObserver
{
public:
    explicit TableNodeViewDelegate(QWidget *parent=0);
    ~TableNodeViewDelegate();

    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

    //QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;

    //void notifyChange(VProperty*);

protected:
    void updateSettings();

    void renderIcons(QPainter *painter,const QModelIndex& index,
                    const QStyleOptionViewItemV4& option) const;

    void renderStatus(QPainter *painter,const QModelIndex& index,
                            const QStyleOptionViewItemV4& option) const;

    //typedef void (TreeNodeViewDelegate::*AttributeRendererProc)(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;

    void renderMeter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderLabel(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderEvent(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderVar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderGenvar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderLimit(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderLimiter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderTrigger(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderTime(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderDate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderRepeat(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
    void renderLate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;

    /*QPen hoverPen_;
    QBrush hoverBrush_;
    QPen selectPen_;
    QBrush selectBrush_;
    QPen nodePen_;
    QPen nodeSelectPen_;
    QPixmap errPix_;

    QBrush lostConnectBgBrush_;
    QBrush lostConnectBandBrush_;*/

    ModelColumn* columns_;

    //QMap<QString,AttributeRendererProc> attrRenderers_;
    //AnimationHandler* animation_;

    //PropertyMapper* prop_;
    //int nodeRectRad_;
    //QFont font_;
    bool drawChildCount_;

    QFont serverNumFont_;
    QFont suiteNumFont_;
    QFont serverInfoFont_;

};

#endif // TABLENODEVIEWDELEGATE_HPP

