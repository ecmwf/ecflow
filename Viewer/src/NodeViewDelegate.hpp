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

#include "FontMetrics.hpp"
#include "VProperty.hpp"

#include <string>

class PropertyMapper;

//Base class for the node/attr renderer properties
struct BaseNodeDelegateBox
{
    BaseNodeDelegateBox() : fontHeight(10), height(10), fullHeight(10), topMargin(0), bottomMargin(0),
       leftMargin(0), rightMargin(0), topPadding(0), bottomPadding(0),
       leftPadding(0), rightPadding(0), sizeHintCache(QSize(10,10)), spacing(2) {}

    virtual void adjust(const QFont& f)=0;
    virtual QRect adjustTextRect(const QRect& rIn) const { QRect r=rIn; return r;}
    virtual QRect adjustSelectionRect(const QRect& optRect) const {
        QRect r=optRect;
        r.adjust(leftMargin,1,-1,-1);
        return r;
    }

    int fontHeight;
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
    int spacing;
};

//Node renderer properties
struct NodeDelegateBox : public BaseNodeDelegateBox
{
    NodeDelegateBox() : iconSize(16), iconPreGap(2),
       iconGap(2) {}

    int iconSize;
    int iconPreGap;
    int iconGap;

    void adjust(const QFont& f) {
         QFontMetrics fm(f);
         fontHeight=fm.height();
         height=fontHeight+topPadding+bottomPadding;
         fullHeight=height+topMargin+bottomMargin;
         sizeHintCache=QSize(100,fullHeight);
         spacing=fm.width('A')*3/4;

         int h=static_cast<int>(static_cast<float>(fm.height())*0.7);
         iconSize=h;
         if(iconSize % 2 == 1)
             iconSize+=1;

         iconGap=1;
         if(iconSize > 16)
             iconGap=2;

         iconPreGap=fm.width('A')/2;
     }
};

//Attr renderer properties
struct AttrDelegateBox : public BaseNodeDelegateBox
{
    AttrDelegateBox() {}

    void adjust(const QFont& f) {
         QFontMetrics fm(f);
         fontHeight=fm.height();
         height=fontHeight+topPadding+bottomPadding;
         fullHeight=height+topMargin+bottomMargin;
         sizeHintCache=QSize(100,fullHeight);
         spacing=fm.width('A')*3/4;
     }
};

class NodeViewDelegate : public QStyledItemDelegate, public VPropertyObserver
{
public:
	explicit NodeViewDelegate(QWidget *parent=0);
	~NodeViewDelegate();

	void notifyChange(VProperty*);

protected:
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
    NodeDelegateBox* nodeBox_;
    AttrDelegateBox* attrBox_;

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



