/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RichTextSearchLine.hpp"

#include <cassert>

#include "RichTextSearchInterface.hpp"

RichTextSearchLine::RichTextSearchLine(QWidget* parent)
    : TextEditSearchLine(parent) {
    interface_ = new RichTextSearchInterface;
    TextEditSearchLine::setSearchInterface(interface_);
}

RichTextSearchLine::~RichTextSearchLine() {
    delete interface_;
}

void RichTextSearchLine::setEditor(QTextBrowser* e) {
    auto* pti = static_cast<RichTextSearchInterface*>(interface_);
    assert(pti);
    pti->setEditor(e);
}
