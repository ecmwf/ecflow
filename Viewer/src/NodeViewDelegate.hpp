//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef NODEVIEWDELEGATE_HPP_
#define NODEVIEWDELEGATE_HPP_

#include <QBrush>
#include <QLinearGradient>
#include <QMap>
#include <QPen>
#include <QStyledItemDelegate>

#include "VProperty.hpp"

#include <string>

class PropertyMapper;

struct NodeDelegateBox
{
    NodeDelegateBox() : height(10) {}
    void adjust(int fontHeight) {
        height=fontHeight+topPadding+bottomPadding;
        fullHeight=height+topMargin+bottomMargin;
        sizeHintCache=QSize(100,fullHeight);
    }

    int height;
    int fullHeight;
    int topMargin;
    int bottomMargin;
    int leftMargin;
    int rightMargin;
    int topPadding;
    int bottomPadding;
    int leftPadding;
    int rightPadding;
    QSize sizeHintCache;
};

class NodeViewDelegate : public QStyledItemDelegate, public VPropertyObserver
{
public:
	explicit NodeViewDelegate(QWidget *parent=0);
	~NodeViewDelegate();

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;

	void notifyChange(VProperty*);

protected:
	void adjustIconSize();
	virtual void updateSettings()=0;
    void addBaseSettings(std::vector<std::string>&);
    void updateBaseSettings();

    void renderSelectionRect(QPainter* painter,QRect r) const;

	virtual void renderStatus(QPainter *painter,const QModelIndex& index,
                              const QStyleOptionViewItem& option) const;

    typedef int (NodeViewDelegate::*AttributeRendererProc)(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;

    virtual int renderMeter(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderLabel(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderEvent(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderVar(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderGenvar(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderLimit(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderLimiter(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderTrigger(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderTime(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderDate(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderRepeat(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;
    virtual int renderLate(QPainter *painter,QStringList data,const QStyleOptionViewItem& option) const;

	QPen hoverPen_;
	QBrush hoverBrush_;
	QPen selectPen_;
	QBrush selectBrush_;
	QPen nodePen_;
	QPen nodeSelectPen_;
	QPixmap errPix_;

	QBrush lostConnectBgBrush_;
	QBrush lostConnectBandBrush_;

	QMap<QString,AttributeRendererProc> attrRenderers_;

	PropertyMapper* prop_;
	QFont font_;
    QFont attrFont_;
    int fontHeight_;
    int attrFontHeight_;
    int iconSize_;
	int iconGap_;

    NodeDelegateBox nodeBox_;
    NodeDelegateBox attrBox_;

	bool useStateGrad_;
	mutable QLinearGradient grad_;
	static int lighter_;
    bool drawAttrSelectionRect_;
    QBrush  eventFillBrush_;
    QBrush  eventBgBrush_;
    QBrush  meterFillBrush_;
    QBrush  meterThresholdBrush_;
    QBrush  limitFillBrush_;
    QBrush  limitExtraFillBrush_;
    QPixmap limitFillPix_;
    QPixmap limitEmptyPix_;
    QPixmap limitExtraFillPix_;
    QBrush  triggerBgBrush_;
    QPen    triggerBorderPen_;
    QPen    triggerFontPen_;
    QBrush  completeBgBrush_;
    QPen    completeBorderPen_;
    QPen    completeFontPen_;
};

#endif



