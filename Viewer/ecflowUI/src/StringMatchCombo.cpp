/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "StringMatchCombo.hpp"

#include <QMenu>
#include <QVariant>

StringMatchTb::StringMatchTb(QWidget* parent)
    : QToolButton(parent) {
    QIcon ic(QPixmap(":/viewer/edit.svg"));
    setIcon(ic);
    setAutoRaise(true);
    auto* menu = new QMenu(this);
    menu->addAction("Contains"); //,StringMatchMode::ContainsMatch);
    menu->addAction("Matches");  //,StringMatchMode::WildcardMatch);
    menu->addAction("Regexp");   //,StringMatchMode::RegexpMatch);
    setMenu(menu);
    setPopupMode(QToolButton::InstantPopup);
}

StringMatchCombo::StringMatchCombo(QWidget* parent)
    : QComboBox(parent) {
    addItem("contains", StringMatchMode::ContainsMatch);
    addItem("matches", StringMatchMode::WildcardMatch);
    addItem("regexp", StringMatchMode::RegexpMatch);

    setCurrentIndex(1);
}

StringMatchMode::Mode StringMatchCombo::matchMode(int index) const {
    if (index >= 0 && index < count()) {
        return static_cast<StringMatchMode::Mode>(itemData(index).toInt());
    }
    return StringMatchMode::WildcardMatch;
}

StringMatchMode::Mode StringMatchCombo::currentMatchMode() const {
    return matchMode(currentIndex());
}

void StringMatchCombo::setMatchMode(const StringMatchMode& mode) {
    int im = mode.toInt();
    for (int i = 0; i < count(); i++) {
        if (itemData(i).toInt() == im) {
            setCurrentIndex(i);
        }
    }
}
