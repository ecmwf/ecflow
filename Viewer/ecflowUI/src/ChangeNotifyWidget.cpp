/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ChangeNotifyWidget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSignalMapper>
#include <QToolButton>

#include "ChangeNotify.hpp"
#include "VNodeList.hpp"
#include "ViewerUtil.hpp"
// #include "VProperty.hpp"

#include <vector>

std::vector<ChangeNotifyWidget*> ChangeNotifyWidget::widgets_;

ChangeNotifyButton::ChangeNotifyButton(QWidget* parent) : QToolButton(parent) {
    setProperty("notify", "1");
    setAutoRaise(true);
    setIconSize(QSize(20, 20));

    grad_.setCoordinateMode(QGradient::ObjectBoundingMode);
    grad_.setStart(0, 0);
    grad_.setFinalStop(0, 1);
}

void ChangeNotifyButton::setNotifier(ChangeNotify* notifier) {
    notifier_ = notifier;

    setToolTip(notifier_->toolTip());

    connect(this, SIGNAL(clicked(bool)), this, SLOT(slotClicked(bool)));

    connect(notifier_->data(), SIGNAL(endAppendRow()), this, SLOT(slotAppend()));

    connect(notifier_->data(), SIGNAL(endRemoveRow(int)), this, SLOT(slotRemoveRow(int)));

    connect(notifier_->data(), SIGNAL(endReset()), this, SLOT(slotReset()));

    updateIcon();
}

void ChangeNotifyButton::slotAppend() {
    updateIcon();
}

void ChangeNotifyButton::slotRemoveRow(int) {
    updateIcon();
}

void ChangeNotifyButton::slotReset() {
    updateIcon();
}

void ChangeNotifyButton::slotClicked(bool) {
    ChangeNotify::showDialog(notifier_);
}

void ChangeNotifyButton::updateIcon() {
    QString text = notifier_->widgetText();
    QString numText;

    int num = 0;
    if (notifier_->data()) {
        num = notifier_->data()->size();
        if (num > 0 && num < 10) {
            numText = QString::number(num);
        }
        else if (num > 10) {
            numText = "9+";
        }
    }

    QColor bgCol(198, 198, 199);
    QColor fgCol(20, 20, 20);
    QColor countBgCol(58, 126, 194);
    QColor countFgCol(Qt::white);

    if (num > 0) {
        bgCol      = notifier_->fillColour();
        fgCol      = notifier_->textColour();
        countBgCol = notifier_->countFillColour();
        countFgCol = notifier_->countTextColour();
    }

    QFont f;
    // f.setBold(true);
    f.setPointSize(f.pointSize());
    QFontMetrics fm(f);

    QFont fNum;
    fNum.setBold(true);
    fNum.setPointSize(f.pointSize() - 1);
    QFontMetrics fmNum(fNum);

    int w = 0;
    if (!numText.isEmpty()) {
        w = ViewerUtil::textWidth(fm, text) + 6 + ViewerUtil::textWidth(fm, numText) + 2;
    }
    else {
        w = ViewerUtil::textWidth(fm, text) + 6;
    }

    int h = fm.height() + 2;

    QPixmap pix(w, h);
    pix.fill(QColor(255, 255, 255, 0));
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QRect textRect(0, 0, ViewerUtil::textWidth(fm, text) + 6, h);

    QColor bgLight = bgCol.lighter(110);
    grad_.setColorAt(0, bgLight);
    grad_.setColorAt(1, bgCol);

    painter.setBrush(QBrush(grad_));
    // painter.setBrush(bgCol);
    painter.setPen(bgCol.darker(170));
    painter.drawRoundedRect(textRect, 2, 2);
    painter.setPen(fgCol);
    painter.setFont(f);
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, text);

    if (!numText.isEmpty()) {
        QRect numRect(textRect.right() - 1, 0, ViewerUtil::textWidth(fmNum, numText) + 4, fmNum.ascent() + 2);
        painter.setBrush(countBgCol);
        painter.setPen(countFgCol);
        painter.drawRoundedRect(numRect, 4, 4);
        painter.setFont(fNum);
        painter.drawText(numRect, Qt::AlignHCenter | Qt::AlignVCenter, numText);
    }

    setIconSize(QSize(w, h));
    setIcon(pix);
}

ChangeNotifyWidget::ChangeNotifyWidget(QWidget* parent) : QWidget(parent) {
    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    auto* label = new QLabel("<b>Notifications</b>: ", this);
    label->setStyleSheet("QLabel{color: " + QColor(60, 60, 60).name() + ";}");
    // QFont f;
    // f.setBold(true);
    // f.setPointSize(f.pointSize()-1);
    // label->setFont(f);
    layout_->addWidget(label);

    ChangeNotify::populate(this);

    widgets_.push_back(this);
}

ChangeNotifyWidget::~ChangeNotifyWidget() {
    auto it = std::find(widgets_.begin(), widgets_.end(), this);
    if (it != widgets_.end()) {
        widgets_.erase(it);
    }
}

ChangeNotifyButton* ChangeNotifyWidget::findButton(const std::string& id) {
    auto it = buttons_.find(id);
    if (it != buttons_.end()) {
        return it->second;
    }

    return nullptr;
}

void ChangeNotifyWidget::addTb(ChangeNotify* notifier) {
    auto* tb = new ChangeNotifyButton(this);
    tb->setNotifier(notifier);
    layout_->addWidget(tb);
    if (!notifier->isEnabled()) {
        tb->setEnabled(false);
        tb->hide();
    }

    buttons_[notifier->id()] = tb;
    updateVisibility();
}

void ChangeNotifyWidget::setEnabled(const std::string& id, bool b) {
    for (auto& widget : widgets_) {
        if (ChangeNotifyButton* tb = widget->findButton(id)) {
            tb->setEnabled(b);
            tb->setVisible(b);
            widget->updateVisibility();
        }
    }
}

void ChangeNotifyWidget::updateVisibility() {
    setVisible(hasVisibleButton());
}

bool ChangeNotifyWidget::hasVisibleButton() const {
    for (const auto& button : buttons_) {
        if (button.second->isEnabled()) {
            return true;
        }
    }
    return false;
}

void ChangeNotifyWidget::updateSettings(const std::string& id) {
    for (auto& widget : widgets_) {
        if (ChangeNotifyButton* tb = widget->findButton(id)) {
            tb->updateIcon();
        }
    }
}
