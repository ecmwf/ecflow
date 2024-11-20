/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "NodeViewDelegate.hpp"

#include <QApplication>
#include <QDebug>
#include <QImageReader>
#include <QPainter>

#include "AbstractNodeModel.hpp"
#include "IconProvider.hpp"
#include "PropertyMapper.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "ViewerUtil.hpp"

int NodeViewDelegate::lighter_ = 150;

static std::vector<std::string> propVec;

LabelStyle::LabelStyle(const std::string& prefix, bool alwaysEnabled)
    : alwaysEnabled_(alwaysEnabled),
      enabled_(true),
      enabledBg_(true) {
    enabledProp_   = VConfig::instance()->find(prefix + "Enabled");
    enabledBgProp_ = VConfig::instance()->find(prefix + "BgEnabled");
    fontProp_      = VConfig::instance()->find(prefix + "FontColour");
    bgProp_        = VConfig::instance()->find(prefix + "BgColour");
    regexProp_     = VConfig::instance()->find(prefix + "Pattern");

    if (!alwaysEnabled_) {
        Q_ASSERT(enabledProp_);
        Q_ASSERT(regexProp_);
    }

    Q_ASSERT(enabledBgProp_);
    Q_ASSERT(fontProp_);
    Q_ASSERT(bgProp_);

    update();
}

void LabelStyle::update() {
    if (!alwaysEnabled_) {
        enabled_ = enabledProp_->value().toBool();
        regex_   = QRegularExpression(regexProp_->value().toString());
        regex_.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }

    enabledBg_ = enabledBgProp_->value().toBool();
    fontPen_   = QPen(fontProp_->value().value<QColor>());
    bgBrush_   = QBrush(bgProp_->value().value<QColor>());
}

NodeViewDelegate::NodeViewDelegate(QWidget* parent) : QStyledItemDelegate(parent) {
    hoverPen_      = QPen(QColor(201, 201, 201));
    hoverBrush_    = QBrush(QColor(250, 250, 250, 210));
    selectPen_     = QPen(QColor(125, 162, 206));
    selectBrush_   = QBrush(QColor(193, 220, 252, 110));
    nodePen_       = QPen(QColor(180, 180, 180));
    nodeSelectPen_ = QPen(QColor(0, 0, 0), 2);

    lostConnectBgBrush_   = QBrush(QColor(150, 150, 150, 150), Qt::Dense7Pattern);
    lostConnectBandBrush_ = QBrush(QColor(255, 166, 0, 150));

    noConnectBgBrush_   = QBrush(QColor(186, 16, 16, 150), Qt::Dense7Pattern);
    noConnectBandBrush_ = QBrush(QColor(186, 16, 16, 240));

    QImageReader imgR(":/viewer/warning.svg");
    if (imgR.canRead()) {
        QFont font;
        QFontMetrics fm(font);
        int size = fm.height() + 2;
        imgR.setScaledSize(QSize(size, size));
        QImage img = imgR.read();
        errPix_    = QPixmap(QPixmap::fromImage(img));
    }

    avisoPixId_  = IconProvider::add(":/viewer/aviso.svg", "aviso");
    mirrorPixId_ = IconProvider::add(":/viewer/remote.svg", "mirror");

    grad_.setCoordinateMode(QGradient::ObjectBoundingMode);
    grad_.setStart(0, 0);
    grad_.setFinalStop(0, 1);

    eventFillBrush_      = QBrush(QColor(0, 0, 255));
    eventFillBrush_      = QBrush(QColor(240, 240, 240));
    meterFillBrush_      = QBrush(QColor(0, 0, 255));
    meterThresholdBrush_ = QBrush(QColor(0, 0, 255));
    limitFillBrush_      = QBrush(QColor(0, 255, 0));
    limitExtraFillBrush_ = QBrush(QColor(0, 0, 255));
    repeatDayPen_        = QPen(QColor(0, 0, 0));
    repeatDatePen_       = QPen(QColor(0, 0, 0));
    repeatEnumPen_       = QPen(QColor(37, 118, 38));
    repeatIntPen_        = QPen(QColor(0, 0, 240));
    repeatStringPen_     = QPen(QColor(0, 0, 0));

    triggerBgBrush_    = QBrush(QColor(230, 230, 230));
    triggerBorderPen_  = QPen(QColor(150, 150, 150));
    triggerFontPen_    = QPen(QColor(0, 0, 0));
    completeBgBrush_   = QBrush(QColor(230, 230, 230));
    completeBorderPen_ = QPen(QColor(150, 150, 150));
    completeFontPen_   = QPen(QColor(0, 0, 255));

    labelStyle_[DefaultLabel] = new LabelStyle("view.label.default", true);
    labelStyle_[ErrorLabel]   = new LabelStyle("view.label.error");
    labelStyle_[WarningLabel] = new LabelStyle("view.label.warning");
    labelStyle_[InfoLabel]    = new LabelStyle("view.label.info");

    holdingTimeFontPen_ = QPen(QColor(255, 0, 0));
    holdingDateFontPen_ = QPen(QColor(255, 0, 0));

    holdingTimePixId_ = IconProvider::add(":/viewer/icon_clock.svg", "icon_clock");
    holdingDatePixId_ = IconProvider::add(":/viewer/icon_calendar.svg", "icon_calendar");

    attrRenderers_["meter"]       = &NodeViewDelegate::renderMeter;
    attrRenderers_["label"]       = &NodeViewDelegate::renderLabel;
    attrRenderers_["aviso"]       = &NodeViewDelegate::renderAviso;
    attrRenderers_["mirror"]      = &NodeViewDelegate::renderMirror;
    attrRenderers_["event"]       = &NodeViewDelegate::renderEvent;
    attrRenderers_["var"]         = &NodeViewDelegate::renderVar;
    attrRenderers_["genvar"]      = &NodeViewDelegate::renderGenvar;
    attrRenderers_["limit"]       = &NodeViewDelegate::renderLimit;
    attrRenderers_["limiter"]     = &NodeViewDelegate::renderLimiter;
    attrRenderers_["trigger"]     = &NodeViewDelegate::renderTrigger;
    attrRenderers_["time"]        = &NodeViewDelegate::renderTime;
    attrRenderers_["date"]        = &NodeViewDelegate::renderDate;
    attrRenderers_["repeat"]      = &NodeViewDelegate::renderRepeat;
    attrRenderers_["late"]        = &NodeViewDelegate::renderLate;
    attrRenderers_["autoarchive"] = &NodeViewDelegate::renderAutoArchive;
    attrRenderers_["autocancel"]  = &NodeViewDelegate::renderAutoCancel;
    attrRenderers_["autorestore"] = &NodeViewDelegate::renderAutoRestore;
    attrRenderers_["queue"]       = &NodeViewDelegate::renderQueue;
}

NodeViewDelegate::~NodeViewDelegate() {
    Q_ASSERT(nodeBox_);
    delete nodeBox_;

    Q_ASSERT(attrBox_);
    delete attrBox_;

    if (prop_)
        delete prop_;

    Q_FOREACH (LabelStyle* s, labelStyle_.values()) {
        if (s)
            delete s;
    }
}

void NodeViewDelegate::notifyChange(VProperty*) {
    updateSettings();
}

void NodeViewDelegate::addBaseSettings(std::vector<std::string>& propVec) {
    propVec.emplace_back("view.common.node_gradient");
    propVec.emplace_back("view.attribute.eventFillColour");
    propVec.emplace_back("view.attribute.meterFillColour");
    propVec.emplace_back("view.attribute.meterThresholdColour");
    propVec.emplace_back("view.attribute.limitShape");
    propVec.emplace_back("view.attribute.limitFillColour");
    propVec.emplace_back("view.attribute.limitExtraFillColour");
    propVec.emplace_back("view.attribute.repeatDateColour");
    propVec.emplace_back("view.attribute.repeatDayColour");
    propVec.emplace_back("view.attribute.repeatEnumColour");
    propVec.emplace_back("view.attribute.repeatIntColour");
    propVec.emplace_back("view.attribute.repeatStringColour");
    propVec.emplace_back("view.attribute.triggerBackground");
    propVec.emplace_back("view.attribute.triggerBorderColour");
    propVec.emplace_back("view.attribute.triggerFontColour");
    propVec.emplace_back("view.attribute.completeBackground");
    propVec.emplace_back("view.attribute.completeBorderColour");
    propVec.emplace_back("view.attribute.completeFontColour");
    propVec.emplace_back("view.attribute.holdingTimeFontColour");
    propVec.emplace_back("view.attribute.holdingDateFontColour");

    Q_FOREACH (LabelStyle* s, labelStyle_.values()) {
        if (s->enabledProp_) {
            propVec.emplace_back(s->enabledProp_->path());
        }
        propVec.emplace_back(s->enabledBgProp_->path());
        propVec.emplace_back(s->fontProp_->path());
        propVec.emplace_back(s->bgProp_->path());
        if (s->regexProp_) {
            propVec.emplace_back(s->regexProp_->path());
        }
    }
}

void NodeViewDelegate::updateBaseSettings() {
    if (VProperty* p = prop_->find("view.common.node_gradient")) {
        useStateGrad_ = p->value().toBool();
    }
    if (VProperty* p = prop_->find("view.attribute.eventFillColour")) {
        eventFillBrush_ = QBrush(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.meterFillColour")) {
        QLinearGradient gr;
        gr.setCoordinateMode(QGradient::ObjectBoundingMode);
        gr.setStart(0, 0);
        gr.setFinalStop(0, 1);
        auto c1 = p->value().value<QColor>();
        gr.setColorAt(0, c1);
        gr.setColorAt(1, c1.lighter(110));
        meterFillBrush_ = QBrush(gr);
    }
    if (VProperty* p = prop_->find("view.attribute.meterThresholdColour")) {
        QLinearGradient gr;
        gr.setCoordinateMode(QGradient::ObjectBoundingMode);
        gr.setStart(0, 0);
        gr.setFinalStop(0, 1);
        auto c1 = p->value().value<QColor>();
        gr.setColorAt(0, c1);
        gr.setColorAt(1, c1.lighter(110));
        meterThresholdBrush_ = QBrush(gr);
    }
    if (VProperty* p = prop_->find("view.attribute.limitShape")) {
        QString shape = p->value().toString();
        if (shape == "rectangle")
            limitShape_ = RectLimitShape;
        else if (shape == "circle")
            limitShape_ = CircleLimitShape;
        else
            limitShape_ = CircleLimitShape;
    }
    if (VProperty* p = prop_->find("view.attribute.limitFillColour")) {
        limitFillBrush_ = QBrush(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.limitExtraFillColour")) {
        limitExtraFillBrush_ = QBrush(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.repeatDayColour")) {
        repeatDayPen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.repeatDateColour")) {
        repeatDatePen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.repeatEnumColour")) {
        repeatEnumPen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.repeatIntColour")) {
        repeatIntPen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.repeatStringColour")) {
        repeatStringPen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.triggerBackground")) {
        triggerBgBrush_ = QBrush(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.triggerBorderColour")) {
        triggerBorderPen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.triggerFontColour")) {
        triggerFontPen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.completeBackground")) {
        completeBgBrush_ = QBrush(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.completeBorderColour")) {
        completeBorderPen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.completeFontColour")) {
        completeFontPen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.holdingTimeFontColour")) {
        holdingTimeFontPen_ = QPen(p->value().value<QColor>());
    }
    if (VProperty* p = prop_->find("view.attribute.holdingDateFontColour")) {
        holdingDateFontPen_ = QPen(p->value().value<QColor>());
    }

    // limit pixmaps
    if (limitShape_ == CircleLimitShape) {
        QFontMetrics fm(attrFont_);
        auto itemSize = static_cast<int>(static_cast<float>(fm.ascent()) * 0.8);

        QImage img(itemSize, itemSize, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter painter(&img);
        painter.setRenderHint(QPainter::Antialiasing, true);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(70, 70, 70));
        painter.drawEllipse(1, 1, itemSize - 1, itemSize - 1);
        painter.setBrush(limitFillBrush_);
        painter.drawEllipse(1, 1, itemSize - 2, itemSize - 2);
        limitFillPix_ = QPixmap::fromImage(img);

        painter.fillRect(QRect(QPoint(0, 0), img.size()), Qt::transparent);
        painter.setBrush(QColor(70, 70, 70));
        painter.drawEllipse(1, 1, itemSize - 1, itemSize - 1);
        painter.setBrush(limitExtraFillBrush_);
        painter.drawEllipse(1, 1, itemSize - 2, itemSize - 2);
        limitExtraFillPix_ = QPixmap::fromImage(img);

        painter.fillRect(QRect(QPoint(0, 0), img.size()), Qt::transparent);
        painter.setBrush(QColor(130, 130, 130));
        painter.drawEllipse(1, 1, itemSize - 1, itemSize - 1);
        painter.setBrush(QColor(240, 240, 240));
        painter.drawEllipse(1, 1, itemSize - 2, itemSize - 2);
        limitEmptyPix_ = QPixmap::fromImage(img);
    }

    // labels
    Q_FOREACH (LabelStyle* s, labelStyle_.values()) {
        s->update();
    }
}

void NodeViewDelegate::renderSelectionRect(QPainter* painter, QRect r) const {
    painter->setPen(nodeSelectPen_);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(r);
}

void NodeViewDelegate::renderStatus(QPainter* painter,
                                    const QModelIndex& index,
                                    const QStyleOptionViewItem& option) const {
    int offset = 4;

    QFontMetrics fm(font_);
    int deltaH = (option.rect.height() - (fm.height() + 4)) / 2;

    // The initial filled rect (we will adjust its  width)
    QRect fillRect = option.rect.adjusted(offset, deltaH, -offset, -deltaH - 1);
    if (option.state & QStyle::State_Selected)
        fillRect.adjust(0, 0, 0, -0);

    int currentRight = fillRect.right();

    // The text rectangle
    QString text   = index.data(Qt::DisplayRole).toString();
    int textWidth  = ViewerUtil::textWidth(fm, text);
    QRect textRect = fillRect.adjusted(offset, 0, 0, 0);
    textRect.setWidth(textWidth);

    if (textRect.right() > currentRight)
        currentRight = textRect.right();

    // Define clipping
    int rightPos           = currentRight + 1;
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        QRect cr = option.rect.adjusted(0, 0, -offset, 0);
        painter->setClipRect(cr);
    }

    // Fill rect
    auto bg        = index.data(Qt::BackgroundRole).value<QColor>();
    QColor bgLight = bg.lighter(lighter_);
    QBrush bgBrush;
    if (useStateGrad_) {
        grad_.setColorAt(0, bgLight);
        grad_.setColorAt(1, bg);
        bgBrush = QBrush(grad_);
    }
    else
        bgBrush = QBrush(bg);

    painter->fillRect(fillRect, bgBrush);

    // Draw text
    painter->setFont(font_);
    painter->setPen(Qt::black);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    if (setClipRect) {
        painter->restore();
    }
}

//========================================================
// data is encoded as a QStringList as follows:
// "meter" name  value min  max colChange
//========================================================

void NodeViewDelegate::renderMeter(QPainter* painter,
                                   QStringList data,
                                   const QStyleOptionViewItem& option,
                                   QSize& size,
                                   [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 6) {
        return;
    }

    // The data
    int val       = data.at(2).toInt();
    int min       = data.at(3).toInt();
    int max       = data.at(4).toInt();
    int threshold = data.at(5).toInt();

    bool selected = option.state & QStyle::State_Selected;

    QString name = data.at(1) + ":";

    if (data.count() == 7)
        name.prepend(data[6] + ":");

    QString valStr = data.at(2);
    QFontMetrics fm(attrFont_);

    // The contents rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin,
                                          attrBox_->topMargin + attrBox_->topPadding,
                                          0,
                                          -attrBox_->bottomMargin - attrBox_->bottomPadding);

    // The status rectangle
    auto stHeight    = static_cast<int>(static_cast<float>(fm.ascent()) * 0.8);
    int stHeightDiff = (contRect.height() - stHeight) / 2;

    QRect stRect = contRect.adjusted(attrBox_->leftPadding, stHeightDiff, 0, -stHeightDiff);
    stRect.setWidth(50);

    // The text rectangle
    QFont nameFont = attrFont_;
    nameFont.setBold(true);
    fm             = QFontMetrics(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect;
    nameRect.setLeft(stRect.x() + stRect.width() + attrBox_->spacing);
    nameRect.setWidth(nameWidth);

    // The value rectangle
    QFont valFont = attrFont_;
    fm            = QFontMetrics(valFont);
    int valWidth  = ViewerUtil::textWidth(fm, valStr);
    QRect valRect = nameRect;
    valRect.setX(nameRect.x() + nameRect.width() + attrBox_->spacing);
    valRect.setWidth(valWidth);

    // Define clipping
    int rightPos           = valRect.x() + valRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Fill st rect
    painter->fillRect(stRect, QColor(229, 229, 229));

    // Draw progress
    if (max > min) {
        QRect progRect = stRect;

        float valPercent = static_cast<float>(val - min) / static_cast<float>(max - min);
        if (threshold > min && threshold < max && val > threshold) {
            auto progWidth = static_cast<int>(static_cast<float>(stRect.width()) * valPercent);
            if (val < max) {
                progRect.setWidth(progWidth);
            }
            painter->fillRect(progRect, meterThresholdBrush_);

            float thresholdPercent = static_cast<float>(threshold - min) / static_cast<float>(max - min);
            progWidth              = static_cast<int>(static_cast<float>(stRect.width()) * thresholdPercent);
            progRect.setWidth(progWidth);
            painter->fillRect(progRect, meterFillBrush_);
        }
        else {
            auto progWidth = static_cast<int>(static_cast<float>(stRect.width()) * valPercent);
            if (val < max) {
                progRect.setWidth(progWidth);
            }
            painter->fillRect(progRect, meterFillBrush_);
        }
    }

    // Draw st rect border
    if (max > min) {
        painter->setPen(QColor(140, 140, 140));
    }
    else {
        painter->setPen(QPen(QColor(140, 140, 140), Qt::DotLine));
    }
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(stRect);

    // Draw name
    painter->setPen(Qt::black);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    // Draw value
    painter->setPen(Qt::black);
    painter->setFont(valFont);
    painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, valStr);

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

//========================================================
// data is encoded as a QStringList as follows:
// "label" name  value
//========================================================

void NodeViewDelegate::renderLabel(QPainter* painter,
                                   QStringList data,
                                   const QStyleOptionViewItem& option,
                                   QSize& size,
                                   [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;
    size           = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 2)
        return;

    QString name = data.at(1) + ":";
    QString val;
    if (data.count() > 2)
        val = data.at(2);

    bool selected = option.state & QStyle::State_Selected;

    // The border rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin,
                                          attrBox_->topMargin + attrBox_->topPadding,
                                          0,
                                          -attrBox_->bottomMargin - attrBox_->bottomPadding);

    int currentRight = contRect.x();
    int multiCnt     = val.count('\n');

    QRect nameRect;
    QRect valRect, valRestRect;

    QFont nameFont = attrFont_;
    nameFont.setBold(true);
    QFont valFont = attrFont_;
    QString valFirst, valRest;
    QString full;

    if (multiCnt == 0) {
        // The text rectangle
        QFontMetrics fm(nameFont);
        int nameWidth = ViewerUtil::textWidth(fm, name);
        nameRect      = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
        nameRect.setWidth(nameWidth);

        // The value rectangle
        fm           = QFontMetrics(valFont);
        int valWidth = ViewerUtil::textWidth(fm, val);
        valRect      = nameRect;
        valRect.setX(nameRect.x() + nameRect.width() + attrBox_->spacing);
        valRect.setWidth(valWidth);

        // Adjust the filled rect width
        currentRight = valRect.x() + valRect.width();
    }
    else {
        // The text rectangle
        QFontMetrics fm(nameFont);
        int nameWidth = ViewerUtil::textWidth(fm, name);
        nameRect      = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
        nameRect.setWidth(nameWidth);
        nameRect.setHeight(attrBox_->height - attrBox_->topPadding - attrBox_->bottomPadding);

        // The value rectangles
        fm = QFontMetrics(valFont);

        // First row comes after the name rect!
        QStringList valLst = val.split("\n");
        Q_ASSERT(valLst.count() > 0);
        valFirst = valLst[0];

        valRect = nameRect;
        valRect.setX(nameRect.x() + nameRect.width() + attrBox_->spacing);
        valRect.setWidth(ViewerUtil::textWidth(fm, valFirst));

        // The rest of the rows
        valLst.takeFirst();
        valRest       = valLst.join("\n");
        QSize valSize = fm.size(0, valRest);

        valRestRect = QRect(nameRect.x(), nameRect.y() + nameRect.height() + 2, valSize.width(), valSize.height());

        currentRight = qMax(valRect.x() + valRect.width(), valRestRect.x() + valRestRect.width());

        val = valFirst + " " + valRest;
    }

    // Define clipping
    int rightPos           = currentRight + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.left();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    QPen fontPen(Qt::black);
    LabelStyle* labelStyle = labelStyle_[DefaultLabel];
    QList<LabelType> types;
    types << ErrorLabel << WarningLabel << InfoLabel;
    full = name + " " + val;
    Q_FOREACH (LabelType t, types) {
        if (labelStyle_[t]->enabled_) {
            if (full.contains(labelStyle_[t]->regex_)) {
                labelStyle = labelStyle_[t];
                break;
            }
        }
    }

    if (labelStyle) {
        if (labelStyle->enabled_) {
            fontPen = labelStyle->fontPen_;
            // draw bg
            if (labelStyle->enabledBg_) {
                QRect sr = option.rect;
                sr.setWidth(rightPos - sr.x());
                painter->fillRect(attrBox_->adjustSelectionRect(sr), labelStyle->bgBrush_);
            }
        }
    }

    // Draw name
    painter->setPen(fontPen);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    // Draw value
    painter->setPen(fontPen);
    painter->setFont(valFont);

    if (multiCnt == 0)
        painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, val);
    else {
        painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, valFirst);
        painter->drawText(valRestRect, Qt::AlignLeft | Qt::AlignVCenter, valRest);
    }

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size = QSize(totalWidth, labelHeight(multiCnt + 1));
}

void NodeViewDelegate::renderAviso(QPainter* painter,
                                   QStringList data,
                                   const QStyleOptionViewItem& option,
                                   QSize& size,
                                   [[maybe_unused]] const QColor& fg) const {

    int totalWidth = 0;
    size           = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 2)
        return;

    QString name = data.at(1); // + ":";
    QString val;
    if (data.count() > 2)
        val = data.at(2);

    bool selected = option.state & QStyle::State_Selected;

    // The border rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin,
                                          attrBox_->topMargin + attrBox_->topPadding,
                                          0,
                                          -attrBox_->bottomMargin - attrBox_->bottomPadding);

    int currentRight = contRect.x();
    int multiCnt     = val.count('\n');

    QRect avisoRect;
    QRect nameRect;
    QRect valRect, valRestRect;

    QFont nameFont = attrFont_;
    nameFont.setBold(true);
    QFont valFont = attrFont_;
    QString valFirst, valRest;
    QString full;

    auto avisoPix = IconProvider::pixmapToHeight(avisoPixId_, contRect.height());
    avisoRect     = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    avisoRect.setWidth(avisoPix.width());

    if (multiCnt == 0) {
        // The text rectangle
        QFontMetrics fm(nameFont);
        int nameWidth = ViewerUtil::textWidth(fm, name);
        nameRect      = contRect;
        nameRect.setX(avisoRect.x() + avisoRect.width() + attrBox_->spacing);
        nameRect.setWidth(nameWidth);

        // The value rectangle
        fm           = QFontMetrics(valFont);
        int valWidth = ViewerUtil::textWidth(fm, val);
        valRect      = nameRect;
        valRect.setX(nameRect.x() + nameRect.width() + attrBox_->spacing);
        valRect.setWidth(valWidth);

        // Adjust the filled rect width
        currentRight = valRect.x() + valRect.width();
    }
    else {
        // The text rectangle
        QFontMetrics fm(nameFont);
        int nameWidth = ViewerUtil::textWidth(fm, name);
        nameRect      = contRect;
        nameRect.setX(avisoRect.x() + avisoRect.width() + attrBox_->spacing);
        nameRect.setWidth(nameWidth);
        nameRect.setHeight(attrBox_->height - attrBox_->topPadding - attrBox_->bottomPadding);

        // The value rectangles
        fm = QFontMetrics(valFont);

        // First row comes after the name rect!
        QStringList valLst = val.split("\n");
        Q_ASSERT(valLst.count() > 0);
        valFirst = valLst[0];

        valRect = nameRect;
        valRect.setX(nameRect.x() + nameRect.width() + attrBox_->spacing);
        valRect.setWidth(ViewerUtil::textWidth(fm, valFirst));

        // The rest of the rows
        valLst.takeFirst();
        valRest       = valLst.join("\n");
        QSize valSize = fm.size(0, valRest);

        valRestRect = QRect(nameRect.x(), nameRect.y() + nameRect.height() + 2, valSize.width(), valSize.height());

        currentRight = qMax(valRect.x() + valRect.width(), valRestRect.x() + valRestRect.width());

        val = valFirst + " " + valRest;
    }

    // Define clipping
    int rightPos           = currentRight + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.left();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    QPen fontPen(Qt::black);
    LabelStyle* labelStyle = labelStyle_[DefaultLabel];
    QList<LabelType> types;
    types << ErrorLabel << WarningLabel << InfoLabel;
    full = name + " " + val;
    Q_FOREACH (LabelType t, types) {
        if (labelStyle_[t]->enabled_) {
            if (full.contains(labelStyle_[t]->regex_)) {
                labelStyle = labelStyle_[t];
                break;
            }
        }
    }

    if (labelStyle) {
        if (labelStyle->enabled_) {
            fontPen = labelStyle->fontPen_;
            // draw bg
            if (labelStyle->enabledBg_) {
                QRect sr = option.rect;
                sr.setWidth(rightPos - sr.x());
                painter->fillRect(attrBox_->adjustSelectionRect(sr), labelStyle->bgBrush_);
            }
        }
    }

    // aviso pixmap
    painter->drawPixmap(avisoRect, avisoPix);

    // Draw name
    painter->setPen(fontPen);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    // Draw value
    painter->setPen(fontPen);
    painter->setFont(valFont);

    if (multiCnt == 0)
        painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, val);
    else {
        painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, valFirst);
        painter->drawText(valRestRect, Qt::AlignLeft | Qt::AlignVCenter, valRest);
    }

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size = QSize(totalWidth, labelHeight(multiCnt + 1));
}

void NodeViewDelegate::renderMirror(QPainter* painter,
                                    QStringList data,
                                    const QStyleOptionViewItem& option,
                                    QSize& size,
                                    [[maybe_unused]] const QColor& fg) const {

    int totalWidth = 0;
    size           = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 2)
        return;

    QString name = data.at(1); // + ":";
    QString val;
    if (data.count() > 2)
        val = data.at(2);

    bool selected = option.state & QStyle::State_Selected;

    // The border rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin,
                                          attrBox_->topMargin + attrBox_->topPadding,
                                          0,
                                          -attrBox_->bottomMargin - attrBox_->bottomPadding);

    int currentRight = contRect.x();
    int multiCnt     = val.count('\n');

    QRect mirrorRect;
    QRect nameRect;
    QRect valRect, valRestRect;

    QFont nameFont = attrFont_;
    nameFont.setBold(true);
    QFont valFont = attrFont_;
    QString valFirst, valRest;
    QString full;

    auto avisoPix = IconProvider::pixmapToHeight(mirrorPixId_, contRect.height());
    mirrorRect    = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    mirrorRect.setWidth(avisoPix.width());

    if (multiCnt == 0) {
        // The text rectangle
        QFontMetrics fm(nameFont);
        int nameWidth = ViewerUtil::textWidth(fm, name);
        nameRect      = contRect;
        nameRect.setX(mirrorRect.x() + mirrorRect.width() + attrBox_->spacing);
        nameRect.setWidth(nameWidth);

        // The value rectangle
        fm           = QFontMetrics(valFont);
        int valWidth = ViewerUtil::textWidth(fm, val);
        valRect      = nameRect;
        valRect.setX(nameRect.x() + nameRect.width() + attrBox_->spacing);
        valRect.setWidth(valWidth);

        // Adjust the filled rect width
        currentRight = valRect.x() + valRect.width();
    }
    else {
        // The text rectangle
        QFontMetrics fm(nameFont);
        int nameWidth = ViewerUtil::textWidth(fm, name);
        nameRect      = contRect;
        nameRect.setX(mirrorRect.x() + mirrorRect.width() + attrBox_->spacing);
        nameRect.setWidth(nameWidth);
        nameRect.setHeight(attrBox_->height - attrBox_->topPadding - attrBox_->bottomPadding);

        // The value rectangles
        fm = QFontMetrics(valFont);

        // First row comes after the name rect!
        QStringList valLst = val.split("\n");
        Q_ASSERT(valLst.count() > 0);
        valFirst = valLst[0];

        valRect = nameRect;
        valRect.setX(nameRect.x() + nameRect.width() + attrBox_->spacing);
        valRect.setWidth(ViewerUtil::textWidth(fm, valFirst));

        // The rest of the rows
        valLst.takeFirst();
        valRest       = valLst.join("\n");
        QSize valSize = fm.size(0, valRest);

        valRestRect = QRect(nameRect.x(), nameRect.y() + nameRect.height() + 2, valSize.width(), valSize.height());

        currentRight = qMax(valRect.x() + valRect.width(), valRestRect.x() + valRestRect.width());

        val = valFirst + " " + valRest;
    }

    // Define clipping
    int rightPos           = currentRight + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.left();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    QPen fontPen(Qt::black);
    LabelStyle* labelStyle = labelStyle_[DefaultLabel];
    QList<LabelType> types;
    types << ErrorLabel << WarningLabel << InfoLabel;
    full = name + " " + val;
    Q_FOREACH (LabelType t, types) {
        if (labelStyle_[t]->enabled_) {
            if (full.contains(labelStyle_[t]->regex_)) {
                labelStyle = labelStyle_[t];
                break;
            }
        }
    }

    if (labelStyle) {
        if (labelStyle->enabled_) {
            fontPen = labelStyle->fontPen_;
            // draw bg
            if (labelStyle->enabledBg_) {
                QRect sr = option.rect;
                sr.setWidth(rightPos - sr.x());
                painter->fillRect(attrBox_->adjustSelectionRect(sr), labelStyle->bgBrush_);
            }
        }
    }

    // aviso pixmap
    painter->drawPixmap(mirrorRect, avisoPix);

    // Draw name
    painter->setPen(fontPen);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    // Draw value
    painter->setPen(fontPen);
    painter->setFont(valFont);

    if (multiCnt == 0)
        painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, val);
    else {
        painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, valFirst);
        painter->drawText(valRestRect, Qt::AlignLeft | Qt::AlignVCenter, valRest);
    }

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size = QSize(totalWidth, labelHeight(multiCnt + 1));
}

void NodeViewDelegate::labelSize(QStringList data, int& totalWidth, int& totalHeight) const {
    if (data.count() < 2)
        return;

    QString name = data.at(1) + ":";
    QString val;
    if (data.count() > 2)
        val = data.at(2);

    int currentRight  = attrBox_->leftMargin;
    int currentBottom = attrBox_->topMargin + attrBox_->topPadding;

    int multiCnt   = val.count('\n');
    QFont nameFont = attrFont_;
    nameFont.setBold(true);
    QFont valFont = attrFont_;

    if (multiCnt == 0) {
        // The text rectangle
        QFontMetrics fm(nameFont);
        int nameWidth = ViewerUtil::textWidth(fm, name);
        currentRight += attrBox_->leftPadding + nameWidth;
        currentBottom += attrBox_->height - attrBox_->topPadding - attrBox_->bottomPadding;

        // The value rectangle
        fm           = QFontMetrics(valFont);
        int valWidth = ViewerUtil::textWidth(fm, val);
        currentRight += attrBox_->spacing + valWidth;
    }

    else {
        // The text rectangle
        QFontMetrics fm(nameFont);
        int nameWidth = ViewerUtil::textWidth(fm, name);
        int startX    = currentRight + attrBox_->leftPadding;
        currentRight += attrBox_->leftPadding + nameWidth;
        currentBottom += attrBox_->height - attrBox_->topPadding - attrBox_->bottomPadding;

        // The value rectangle
        fm = QFontMetrics(valFont);

        // First row comes after the name rect!
        QStringList valLst = val.split("\n");
        Q_ASSERT(valLst.count() > 0);

        currentRight += attrBox_->spacing + ViewerUtil::textWidth(fm, valLst[0]);

        // The rest of the rows
        valLst.takeFirst();
        QString valRest = valLst.join("\n");
        QSize valSize   = fm.size(0, valRest);

        currentRight = qMax(currentRight, startX + valSize.width());
        currentBottom += 2 + valSize.height();
    }

    totalWidth  = currentRight + attrBox_->rightPadding + attrBox_->rightMargin;
    totalHeight = currentBottom + attrBox_->bottomPadding + attrBox_->bottomMargin;
}

int NodeViewDelegate::labelHeight(int lineNum) const {
    if (lineNum <= 1)
        return attrBox_->sizeHintCache.height();

    int currentBottom = attrBox_->topMargin + attrBox_->topPadding;

    // text rect
    currentBottom += attrBox_->height - attrBox_->topPadding - attrBox_->bottomPadding;

    // value rect
    QStringList lst;
    for (int i = 0; i < lineNum - 1; i++)
        lst << "1";

    QFontMetrics fm(attrFont_);
    QSize valSize = fm.size(0, lst.join(QString("\n")));
    currentBottom += 2 + valSize.height();

    return currentBottom + attrBox_->bottomPadding + attrBox_->bottomMargin;
}

//========================================================
// data is encoded as a QStringList as follows:
// "event" name  value
//========================================================

void NodeViewDelegate::renderEvent(QPainter* painter,
                                   QStringList data,
                                   const QStyleOptionViewItem& option,
                                   QSize& size,
                                   [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 2)
        return;

    QString name = data[1];
    bool val     = false;
    if (data.count() > 2)
        val = (data[2] == "1");

    // Add full path
    if (data.count() > 3) {
        name.prepend(data[3] + ":");
    }

    bool selected = option.state & QStyle::State_Selected;
    QFont font    = attrFont_;
    QFontMetrics fm(font);

    // The border rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The control rect
    auto ctHeight    = static_cast<int>(static_cast<float>(fm.ascent()) * 0.8);
    int ctHeightDiff = qMax((contRect.height() - ctHeight) / 2, 2);

    QRect ctRect = contRect.adjusted(attrBox_->leftPadding, ctHeightDiff, 0, -ctHeightDiff);
    ctRect.setWidth(ctRect.height());

    // The text rectangle
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect;
    nameRect.setX(ctRect.x() + ctRect.width() + attrBox_->spacing);
    nameRect.setWidth(nameWidth);

    // Define clipping
    int rightPos           = nameRect.x() + nameRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.left();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw control
    painter->setPen(Qt::black);
    painter->setBrush((val) ? eventFillBrush_ : eventBgBrush_);
    painter->drawRect(ctRect);

    // Draw name
    painter->setPen(Qt::black);
    painter->setFont(font);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderVarCore(QPainter* painter,
                                     QStringList data,
                                     const QStyleOptionViewItem& option,
                                     QSize& size,
                                     QColor textCol) const {
    int totalWidth = 0;
    QString text;

    if (data.count() > 1)
        text += data.at(1) + "=";
    if (data.count() > 2)
        text += data.at(2);

    if (data.count() == 4)
        text.prepend(data[3] + ":");

    bool selected = option.state & QStyle::State_Selected;

    // The contents rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The text rectangle
    QFont font = attrFont_;
    QFontMetrics fm(font);
    int textWidth  = ViewerUtil::textWidth(fm, text);
    QRect textRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    textRect.setWidth(textWidth);

    // Define clipping
    int rightPos           = textRect.x() + textRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.left();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw text
    painter->setPen(textCol);
    painter->setFont(font);
    painter->drawText(attrBox_->adjustTextRect(textRect), Qt::AlignLeft | Qt::AlignVCenter, text);

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size = QSize(totalWidth, attrBox_->fullHeight);
}

void NodeViewDelegate::renderVar(QPainter* painter,
                                 QStringList data,
                                 const QStyleOptionViewItem& option,
                                 QSize& size,
                                 const QColor& fg) const {
    renderVarCore(painter, data, option, size, fg);
}

void NodeViewDelegate::renderGenvar(QPainter* painter,
                                    QStringList data,
                                    const QStyleOptionViewItem& option,
                                    QSize& size,
                                    [[maybe_unused]] const QColor& fg) const {
    renderVarCore(painter, data, option, size, Qt::blue);
}

void NodeViewDelegate::renderLimit(QPainter* painter,
                                   QStringList data,
                                   const QStyleOptionViewItem& option,
                                   QSize& size,
                                   [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 4) {
        return;
    }

    // The data
    int val        = data.at(2).toInt();
    int maxVal     = data.at(3).toInt();
    QString name   = data.at(1) + ":";
    QString valStr = QString::number(val) + "/" + QString::number(maxVal);
    int totalVal   = qMax(val, maxVal); // val can be larger than maxVal!!

    if (data.count() == 5)
        name.prepend(data[4] + ":");

    bool selected = option.state & QStyle::State_Selected;

    // The contents rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    QFontMetrics fm(attrFont_);
    int itemOffset = 0;
    int itemWidth  = 0;
    int itemHeight = 0;

    if (limitShape_ == RectLimitShape) {
        itemWidth  = ViewerUtil::textWidth(fm, 'p') / 2;
        itemOffset = qMax(itemWidth / 2, 2);
        itemHeight = static_cast<float>(contRect.height()) * 0.8;
    }
    else {
        itemWidth  = limitFillPix_.width();
        itemHeight = itemWidth;
        itemOffset = 0;
    }

    // The text rectangle
    QFont nameFont = attrFont_;
    nameFont.setBold(true);
    fm             = QFontMetrics(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    nameRect.setWidth(nameWidth);

    // The value rectangle
    QFont valFont = attrFont_;
    fm            = QFontMetrics(valFont);
    int valWidth  = ViewerUtil::textWidth(fm, valStr);
    QRect valRect = nameRect;
    valRect.setX(nameRect.x() + nameRect.width() + attrBox_->spacing);
    valRect.setWidth(valWidth);

    int xItem = valRect.x() + valRect.width() + attrBox_->spacing;

    int itemNum = totalVal;
    if (maxLimitItems_ > 0) {
        itemNum = std::min(itemNum, maxLimitItems_);
    }

    int rightPos = xItem + itemNum * (itemWidth + itemOffset) + itemOffset;

    QRect trailRect = valRect;
    QString trailTxt("...");
    bool allItems = (itemNum == totalVal);
    if (!allItems) {
        trailRect.setX(rightPos + 2 * itemOffset);
        trailRect.setWidth(ViewerUtil::textWidth(fm, trailTxt) + itemOffset);
        rightPos = trailRect.x() + trailRect.width();
    }

    rightPos += attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth = rightPos - option.rect.x();

    // Define clipping
    const bool setClipRect = rightPos > option.rect.right();

    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw name
    painter->setPen(Qt::black);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    // Draw value
    if (val < maxVal)
        painter->setPen(Qt::black);
    else if (val == maxVal)
        painter->setPen(QColor(14, 148, 26));
    else
        painter->setPen(Qt::red);

    painter->setFont(valFont);
    painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, valStr);

    // Draw items
    if (limitShape_ == RectLimitShape) {
        int yItem = option.rect.y() + (option.rect.height() - itemHeight) / 2;
        painter->setRenderHint(QPainter::Antialiasing, false);

        painter->setPen(Qt::NoPen);
        painter->setBrush(limitFillBrush_);

        for (int i = 0; i < itemNum; i++) {
            QRect r(xItem, yItem, itemWidth, itemHeight);

            if (i == val && i < maxVal) {
                painter->setPen(QPen(QColor(190, 190, 190), 0));
                painter->setBrush(Qt::NoBrush);
            }
            else if (i == maxVal)
                painter->setBrush(limitExtraFillBrush_);

            painter->drawRect(r);

            if (i < val) {
                painter->setPen(painter->brush().color().darker(120));
                painter->drawLine(xItem, yItem, xItem + itemWidth, yItem);
                painter->drawLine(xItem, yItem, xItem, yItem + itemHeight);

                painter->setPen(painter->brush().color().darker(170));
                painter->drawLine(xItem + itemWidth, yItem, xItem + itemWidth, yItem + itemHeight);
                painter->drawLine(xItem, yItem + itemHeight, xItem + itemWidth, yItem + itemHeight);
            }

            xItem += itemOffset + itemWidth;
        }
    }
    else {
        int yItem = option.rect.y() + (option.rect.height() - itemHeight) / 2;
        for (int i = 0; i < itemNum; i++) {
            if (i >= maxVal)
                painter->drawPixmap(xItem, yItem, itemWidth, itemHeight, limitExtraFillPix_);
            else if (i >= val)
                painter->drawPixmap(xItem, yItem, itemWidth, itemHeight, limitEmptyPix_);
            else
                painter->drawPixmap(xItem, yItem, itemHeight, itemWidth, limitFillPix_);

            xItem += itemOffset + itemHeight;
        }
    }

    // draw trailing ...
    if (!allItems) {
        painter->setPen(QColor(60, 61, 62));
        painter->setFont(nameFont);
        painter->drawText(attrBox_->adjustTextRect(trailRect), Qt::AlignLeft | Qt::AlignVCenter, trailTxt);
    }

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderLimiter(QPainter* painter,
                                     QStringList data,
                                     const QStyleOptionViewItem& option,
                                     QSize& size,
                                     [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 4)
        return;

    QString name = "inlimit " + data[2] + ":" + data[1];
    if (data[3] != "1")
        name += " " + data[3];

    if (data.count() == 7)
        name.prepend(data[6] + ":");

    bool selected = option.state & QStyle::State_Selected;

    // The contents rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The text rectangle
    QFont nameFont = attrFont_;
    // nameFont.setBold(true);
    QFontMetrics fm(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    nameRect.setWidth(nameWidth);

    // Define clipping
    int rightPos           = nameRect.x() + nameRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw name
    painter->setPen(Qt::black);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderTrigger(QPainter* painter,
                                     QStringList data,
                                     const QStyleOptionViewItem& option,
                                     QSize& size,
                                     const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() != 3)
        return;

    int triggerType = data[1].toInt();
    QString text    = data.at(2);
    bool selected   = option.state & QStyle::State_Selected;

    // The contents rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The text rectangle
    QFont font = attrFont_;
    QFontMetrics fm(font);
    int textWidth  = ViewerUtil::textWidth(fm, text) + 4;
    QRect textRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    textRect.setWidth(textWidth);

    // Define clipping
    int rightPos           = textRect.x() + textRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // draw rect
    if (triggerType == 0) {
        painter->setBrush(triggerBgBrush_);
        painter->setPen(triggerBorderPen_);
    }
    else {
        painter->setBrush(completeBgBrush_);
        painter->setPen(completeBorderPen_);
    }

    // QRect borderRect=option.rect;
    // borderRect.setWidth(rightPos-borderRect.x());
    painter->drawRect(attrBox_->adjustTextBgRect(textRect));

    // Draw text
    if (triggerType == 0)
        painter->setPen(triggerFontPen_);
    else
        painter->setPen(completeFontPen_);

    painter->setFont(font);
    painter->drawText(attrBox_->adjustTextRect(textRect), Qt::AlignHCenter | Qt::AlignVCenter, text);

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setX(textRect.x());
        sr.setWidth(textRect.width());
        // sr.setWidth(rightPos-sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRectNonOpt(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderTime(QPainter* painter,
                                  QStringList data,
                                  const QStyleOptionViewItem& option,
                                  QSize& size,
                                  [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 3)
        return;

    QString name = data[1];

    if (data.count() == 4)
        name.prepend(data[3] + ":");

    bool selected = option.state & QStyle::State_Selected;

    // The contents rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The text rectangle
    QFont nameFont = attrFont_;
    // nameFont.setBold(true);
    QFontMetrics fm(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    nameRect.setWidth(nameWidth);

    int currentRight = nameRect.x() + nameRect.width();

    // Icon
    QPixmap pix;
    QRect pixRect;
    if (data[2] == "0") {
        int xp       = currentRight + attrBox_->iconPreGap;
        int yp       = nameRect.center().y() + 1 - attrBox_->iconSize / 2;
        pix          = IconProvider::pixmap(holdingTimePixId_, attrBox_->iconSize);
        pixRect      = QRect(xp, yp, attrBox_->iconSize, attrBox_->iconSize);
        currentRight = xp + attrBox_->iconSize + attrBox_->iconGap;
    }

    // Define clipping
    int rightPos           = currentRight + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw name
    if (data[2] == "1") {
        painter->setPen(Qt::black);
    }
    else {
        painter->setPen(holdingTimeFontPen_);
    }

    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    // Draw icon
    if (!pix.isNull()) {
        painter->drawPixmap(pixRect, pix);
    }

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderDate(QPainter* painter,
                                  QStringList data,
                                  const QStyleOptionViewItem& option,
                                  QSize& size,
                                  [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 3)
        return;

    QString name = data[1];

    if (data.count() == 4)
        name.prepend(data[3] + ":");

    bool selected = option.state & QStyle::State_Selected;

    // The contents rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The text rectangle
    QFont nameFont = attrFont_;
    // nameFont.setBold(true);
    QFontMetrics fm(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    nameRect.setWidth(nameWidth);

    int currentRight = nameRect.x() + nameRect.width();

    // Icon
    QPixmap pix;
    QRect pixRect;
    if (data[2] == "0") {
        int xp       = currentRight + attrBox_->iconPreGap;
        int yp       = nameRect.center().y() + 1 - attrBox_->iconSize / 2;
        pix          = IconProvider::pixmap(holdingDatePixId_, attrBox_->iconSize);
        pixRect      = QRect(xp, yp, attrBox_->iconSize, attrBox_->iconSize);
        currentRight = xp + attrBox_->iconSize + attrBox_->iconGap;
    }

    // Define clipping
    int rightPos           = currentRight + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw name
    if (data[2] == "1") {
        painter->setPen(Qt::black);
    }
    else {
        painter->setPen(holdingDateFontPen_);
    }

    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    // Draw icon
    if (!pix.isNull()) {
        painter->drawPixmap(pixRect, pix);
    }

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

//========================================================
// data is encoded as a QStringList as follows:
// "repeat" name  value
//========================================================

void NodeViewDelegate::renderRepeat(QPainter* painter,
                                    QStringList data,
                                    const QStyleOptionViewItem& option,
                                    QSize& size,
                                    [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 10)
        return;

    QString type = data.at(1);
    QString name = data.at(2);
    QString val  = data.at(3);
    QString step = data.at(6);
    QString pos  = data.at(8);

    if (data.count() == 11) {
        name.prepend(data[10] + ":");
    }

    bool selected = option.state & QStyle::State_Selected;

    QPen rPen(Qt::black);

    // The contents rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    if (type == "day") {
        rPen = repeatDayPen_;

        QFont nameFont = attrFont_;
        QFontMetrics fm(nameFont);
        name           = "day=" + step;
        int nameWidth  = ViewerUtil::textWidth(fm, name);
        QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
        nameRect.setWidth(nameWidth);

        // Define clipping
        int rightPos           = nameRect.x() + nameRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
        totalWidth             = rightPos - option.rect.x();
        const bool setClipRect = rightPos > option.rect.right();
        if (setClipRect) {
            painter->save();
            painter->setClipRect(option.rect);
        }

        // Draw name
        painter->setPen(rPen);
        painter->setFont(nameFont);
        painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

        if (selected && drawAttrSelectionRect_) {
            QRect sr = option.rect;
            sr.setWidth(rightPos - sr.x());
            renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
        }

        if (setClipRect) {
            painter->restore();
        }
    }
    else {
        if (type == "enumerated") {
            rPen = repeatEnumPen_;
        }
        else if (type == "integer") {
            rPen = repeatIntPen_;
        }
        else if (type == "string") {
            rPen = repeatStringPen_;
        }
        else if (type == "date" || type == "datelist") {
            rPen = repeatDatePen_;
        }

        QString endDot;
        if (pos == "0") {
            name += "=";
            endDot = "...";
        }
        else if (pos == "1") {
            name += "=...";
            endDot = "...";
        }
        else if (pos == "2") {
            name += "=...";
            endDot = "";
        }
        else {
            name += "=";
            endDot = "";
        }

        // The name rectangle
        QFont nameFont = attrFont_;
        QFontMetrics fm(nameFont);
        int nameWidth  = ViewerUtil::textWidth(fm, name);
        QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
        nameRect.setWidth(nameWidth);

        // The value rectangle
        QFont valFont = attrFont_;
        valFont.setBold(true);
        fm            = QFontMetrics(valFont);
        int valWidth  = ViewerUtil::textWidth(fm, val);
        QRect valRect = nameRect;
        if (name.endsWith("..."))
            valRect.setX(nameRect.x() + nameRect.width() + ViewerUtil::textWidth(fm, 'A') / 2);
        else
            valRect.setX(nameRect.x() + nameRect.width() + ViewerUtil::textWidth(fm, ' ') / 2);

        valRect.setWidth(valWidth);

        int rightPos = valRect.x() + valRect.width();

        // End ...
        QRect dotRect;
        if (!endDot.isEmpty()) {
            fm           = QFontMetrics(nameFont);
            int dotWidth = ViewerUtil::textWidth(fm, "...");
            dotRect      = valRect;
            dotRect.setX(rightPos + ViewerUtil::textWidth(fm, 'A') / 2);
            dotRect.setWidth(dotWidth);
            rightPos = dotRect.x() + dotRect.width();
        }

        // Define clipping
        rightPos += +attrBox_->rightPadding + attrBox_->rightMargin;
        totalWidth             = rightPos - option.rect.x();
        const bool setClipRect = rightPos > option.rect.right();
        if (setClipRect) {
            painter->save();
            painter->setClipRect(option.rect);
        }

        // Draw name
        painter->setPen(rPen);
        painter->setFont(nameFont);
        painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

        // Draw value
        painter->setPen(rPen);
        painter->setFont(valFont);
        painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, val);

        // Draw end dots
        if (!endDot.isEmpty()) {
            painter->setPen(rPen);
            painter->setFont(nameFont);
            painter->drawText(attrBox_->adjustTextRect(dotRect), Qt::AlignLeft | Qt::AlignVCenter, "...");
        }

        if (selected && drawAttrSelectionRect_) {
            QRect sr = option.rect;
            sr.setWidth(rightPos - sr.x());
            renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
        }

        if (setClipRect) {
            painter->restore();
        }
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderLate(QPainter* painter,
                                  QStringList data,
                                  const QStyleOptionViewItem& option,
                                  QSize& size,
                                  [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 2)
        return;

    QString name = "late: " + data[1];

    if (data.count() == 3)
        name.prepend(data[2] + ":");

    bool selected = option.state & QStyle::State_Selected;

    // The border rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The text rectangle
    QFont nameFont = attrFont_;
    QFontMetrics fm(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    nameRect.setWidth(nameWidth);

    // Define clipping
    int rightPos           = nameRect.x() + nameRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw name
    painter->setPen(Qt::black);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderAutoArchive(QPainter* painter,
                                         QStringList data,
                                         const QStyleOptionViewItem& option,
                                         QSize& size,
                                         const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 2)
        return;

    QString name = "autoarchive: " + data[1];

    if (data.count() == 3)
        name.prepend(data[2] + ":");

    bool selected = option.state & QStyle::State_Selected;

    // The border rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The text rectangle
    QFont nameFont = attrFont_;
    QFontMetrics fm(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    nameRect.setWidth(nameWidth);

    // Define clipping
    int rightPos           = nameRect.x() + nameRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw name
    painter->setPen(Qt::black);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderAutoCancel(QPainter* painter,
                                        QStringList data,
                                        const QStyleOptionViewItem& option,
                                        QSize& size,
                                        [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 2)
        return;

    QString name = "autocancel: " + data[1];

    if (data.count() == 3)
        name.prepend(data[2] + ":");

    bool selected = option.state & QStyle::State_Selected;

    // The border rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The text rectangle
    QFont nameFont = attrFont_;
    QFontMetrics fm(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    nameRect.setWidth(nameWidth);

    // Define clipping
    int rightPos           = nameRect.x() + nameRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw name
    painter->setPen(Qt::black);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderAutoRestore(QPainter* painter,
                                         QStringList data,
                                         const QStyleOptionViewItem& option,
                                         QSize& size,
                                         const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 2)
        return;

    QString name = "autorestore: " + data[1];

    if (data.count() == 3)
        name.prepend(data[2] + ":");

    bool selected = option.state & QStyle::State_Selected;

    // The border rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    // The text rectangle
    QFont nameFont = attrFont_;
    QFontMetrics fm(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    nameRect.setWidth(nameWidth);

    // Define clipping
    int rightPos           = nameRect.x() + nameRect.width() + attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw name
    painter->setPen(Qt::black);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}

void NodeViewDelegate::renderQueue(QPainter* painter,
                                   QStringList data,
                                   const QStyleOptionViewItem& option,
                                   QSize& size,
                                   [[maybe_unused]] const QColor& fg) const {
    int totalWidth = 0;

    size = QSize(totalWidth, attrBox_->fullHeight);

    if (data.count() < 5)
        return;

    QString name = data.at(1);
    QString val  = data.at(2);
    QString pos  = data.at(4);

    if (data.count() == 6)
        name.prepend(data[5] + ":");

    bool selected = option.state & QStyle::State_Selected;

    QPen rPen(Qt::black);

    // The contents rect (we will adjust its  width)
    QRect contRect = option.rect.adjusted(attrBox_->leftMargin, attrBox_->topMargin, 0, -attrBox_->bottomMargin);

    QString endDot;
    if (pos == "0") {
        name += "=";
        endDot = "...";
    }
    else if (pos == "1") {
        name += "=...";
        endDot = "...";
    }
    else if (pos == "2") {
        name += "=...";
        endDot = "";
    }
    else {
        name += "=";
        endDot = "";
    }

    // The name rectangle
    QFont nameFont = attrFont_;
    QFontMetrics fm(nameFont);
    int nameWidth  = ViewerUtil::textWidth(fm, name);
    QRect nameRect = contRect.adjusted(attrBox_->leftPadding, 0, 0, 0);
    nameRect.setWidth(nameWidth);

    // The value rectangle
    QFont valFont = attrFont_;
    valFont.setBold(true);
    fm            = QFontMetrics(valFont);
    int valWidth  = ViewerUtil::textWidth(fm, val);
    QRect valRect = nameRect;
    if (name.endsWith("..."))
        valRect.setX(nameRect.x() + nameRect.width() + ViewerUtil::textWidth(fm, 'A') / 2);
    else
        valRect.setX(nameRect.x() + nameRect.width() + ViewerUtil::textWidth(fm, ' ') / 2);

    valRect.setWidth(valWidth);

    int rightPos = valRect.x() + valRect.width();

    // End ...
    QRect dotRect;
    if (!endDot.isEmpty()) {
        fm           = QFontMetrics(nameFont);
        int dotWidth = ViewerUtil::textWidth(fm, "...");
        dotRect      = valRect;
        dotRect.setX(rightPos + ViewerUtil::textWidth(fm, 'A') / 2);
        dotRect.setWidth(dotWidth);
        rightPos = dotRect.x() + dotRect.width();
    }

    // Define clipping
    rightPos += +attrBox_->rightPadding + attrBox_->rightMargin;
    totalWidth             = rightPos - option.rect.x();
    const bool setClipRect = rightPos > option.rect.right();
    if (setClipRect) {
        painter->save();
        painter->setClipRect(option.rect);
    }

    // Draw name
    painter->setPen(rPen);
    painter->setFont(nameFont);
    painter->drawText(attrBox_->adjustTextRect(nameRect), Qt::AlignLeft | Qt::AlignVCenter, name);

    // Draw value
    painter->setPen(rPen);
    painter->setFont(valFont);
    painter->drawText(attrBox_->adjustTextRect(valRect), Qt::AlignLeft | Qt::AlignVCenter, val);

    // Draw end dots
    if (!endDot.isEmpty()) {
        painter->setPen(rPen);
        painter->setFont(nameFont);
        painter->drawText(attrBox_->adjustTextRect(dotRect), Qt::AlignLeft | Qt::AlignVCenter, "...");
    }

    if (selected && drawAttrSelectionRect_) {
        QRect sr = option.rect;
        sr.setWidth(rightPos - sr.x());
        renderSelectionRect(painter, attrBox_->adjustSelectionRect(sr));
    }

    if (setClipRect) {
        painter->restore();
    }

    size.setWidth(totalWidth);
}
