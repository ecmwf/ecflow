// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QComboBox>
#include <QToolButton>

#include "StringMatchMode.hpp"

class StringMatchTb : public QToolButton {
public:
    explicit StringMatchTb(QWidget* parent = nullptr);
};

class StringMatchCombo : public QComboBox {
    Q_OBJECT

public:
    explicit StringMatchCombo(QWidget* parent = nullptr);

    StringMatchMode::Mode matchMode(int) const;
    StringMatchMode::Mode currentMatchMode() const;
    void setMatchMode(const StringMatchMode& mode);
};
