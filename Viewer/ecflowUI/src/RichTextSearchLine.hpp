// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QTextBrowser>

#include "TextEditSearchLine.hpp"

class AbstractTextSearchInterface;

class RichTextSearchLine : public TextEditSearchLine {
public:
    explicit RichTextSearchLine(QWidget* parent = nullptr);
    ~RichTextSearchLine() override;
    void setEditor(QTextBrowser*);

private:
    // The interface is set internally
    void setSearchInterface(AbstractTextSearchInterface*) {}
};
