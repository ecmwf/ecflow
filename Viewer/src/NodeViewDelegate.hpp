//============================================================================
// Copyright 2014 ECMWF.
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

	virtual void renderStatus(QPainter *painter,const QModelIndex& index,
	                          const QStyleOptionViewItemV4& option) const;

	typedef void (NodeViewDelegate::*AttributeRendererProc)(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;

	virtual void renderMeter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderLabel(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderEvent(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderVar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderGenvar(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderLimit(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderLimiter(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderTrigger(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderTime(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderDate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderRepeat(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;
	virtual void renderLate(QPainter *painter,QStringList data,const QStyleOptionViewItemV4& option) const;

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
	int iconSize_;
	int iconGap_;

	bool useStateGrad_;
	mutable QLinearGradient grad_;
	static int lighter_;
    QBrush eventFillBrush_;
    QBrush eventBgBrush_;
    QBrush meterFillBrush_;
    QBrush meterThresholdBrush_;
    QBrush limitFillBrush_;
    QPixmap limitFillPix_;
    QPixmap limitEmptyPix_;
};

#endif



