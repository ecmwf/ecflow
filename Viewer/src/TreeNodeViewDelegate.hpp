//============================================================================
// Copyright 2014 ECMWF.
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
#include <QPen>
#include <QStyledItemDelegate>

class TreeNodeViewDelegate : public QStyledItemDelegate
{
public:
	TreeNodeViewDelegate(QWidget *parent=0);
	void paint(QPainter *painter,const QStyleOptionViewItem &option,
		           const QModelIndex& index) const;
	QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;

protected:
	void renderNode(QPainter *painter,const QModelIndex& index,
			          QString text,QRect textRect,QRect optRect) const;

	void renderMeter(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderLabel(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderEvent(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderVar(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderGenVar(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderLimit(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderLimiter(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderTrigger(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderTime(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderDate(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderRepeat(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;
	void renderLate(QPainter *painter,QStringList data,QRect textRect,QRect optRect) const;

	QPen hoverPen_;
	QBrush hoverBrush_;
	QPen selectPen_;
	QBrush selectBrush_;
};


#endif



