//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerViewDelegate.hpp"

#include <QApplication>
#include <QDebug>
#include <QImageReader>
#include <QLinearGradient>
#include <QPainter>
#include <QtGlobal>

#include "AbstractNodeModel.hpp"
#include "Animation.hpp"
#include "FontMetrics.hpp"
#include "IconProvider.hpp"
#include "PropertyMapper.hpp"
#include "ServerHandler.hpp"
#include "UiLog.hpp"

static std::vector<std::string> propVec;

#if 0
static QColor typeFgColourClassic=QColor(Qt::white);
static QColor typeBgColourClassic=QColor(150,150,150);
static QColor childCountColour=QColor(90,91,92);
#endif

struct TriggerNodeDelegateBox : public NodeDelegateBox
{
    TriggerNodeDelegateBox() {
        topMargin     = 2;
        bottomMargin  = 2;
        leftMargin    = 2;
        rightMargin   = 2;
        topPadding    = 0;
        bottomPadding = 0;
        leftPadding   = 2;
        rightPadding  = 2;
    }
};

struct TriggerAttrDelegateBox : public AttrDelegateBox
{
    TriggerAttrDelegateBox() {
        topMargin     = 2;
        bottomMargin  = 2;
        leftMargin    = 2;
        rightMargin   = 2;
        topPadding    = 0;
        bottomPadding = 0;
        leftPadding   = 2;
        rightPadding  = 0;
    }
};

TriggerViewDelegate::TriggerViewDelegate(QWidget* parent)
    : TreeNodeViewDelegate(nullptr,
                           // new TriggerNodeDelegateBox,
                           // new TriggerAttrDelegateBox,
                           parent),
      borderPen_(QPen(QColor(220, 220, 220))) {
}

void TriggerViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QStyleOptionViewItem vopt(option);
#else
    QStyleOptionViewItemV4 vopt(option);
#endif

    initStyleOption(&vopt, index);

    // const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
    // const QWidget* widget = vopt.widget;

    // Save painter state
    painter->save();

    // Background
    auto bgcol = index.data(Qt::UserRole).value<QColor>();
    if (bgcol.isValid())
        painter->fillRect(vopt.rect, bgcol);

    if (index.column() == 0) {
        QVariant tVar = index.data(Qt::DisplayRole);
        painter->setFont(font_);

        // Node
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (tVar.typeId() == QMetaType::QString) {
#else
        if (tVar.type() == QVariant::String) {
#endif
            QString text = index.data(Qt::DisplayRole).toString();

            // If the textalignment is AlignCenter the node is displayed
            // as centered in the opt rect!
            QVariant vTa = index.data(Qt::TextAlignmentRole);
            if (!vTa.isNull()) {
                if (vTa.toInt() == Qt::AlignCenter) {
                    int w  = nodeWidth(index, vopt.rect.height(), text);
                    int dw = (vopt.rect.width() - w) / 2;
                    if (dw > 0)
                        vopt.rect.moveLeft(dw);
                }
            }

            renderNode(painter, index, vopt, text);
        }

        // Render attributes
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        else if (tVar.typeId() == QMetaType::QStringList) {
#else
        else if (tVar.type() == QVariant::StringList) {
#endif
            QStringList lst = tVar.toStringList();
            if (lst.count() > 0) {
                QMap<QString, AttributeRendererProc>::const_iterator it = attrRenderers_.find(lst.at(0));
                if (it != attrRenderers_.end()) {
                    QSize size;
                    AttributeRendererProc a = it.value();
                    (this->*a)(painter, lst, vopt, size);
                }
            }
        }
    }
    // Render the horizontal border for rows. We only render the top border line.
    // With this technique we miss the bottom border line of the last row!!
    if (renderSeparatorLine_) {
        QRect bgRect = option.rect;
        painter->setPen(borderPen_);
        painter->drawLine(bgRect.topLeft(), bgRect.topRight());
    }

    painter->restore();
}
