/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VParam.hpp"

#include "VProperty.hpp"

static uint idCounter = 0;

VParam::VParam(const std::string& name)
    : name_(name),
      qName_(QString::fromStdString(name)),
      prop_(nullptr),
      colourPropName_("fill_colour"),
      fontColourPropName_("font_colour"),
      typeColourPropName_("type_colour"),
      id_(idCounter++) {
}

VParam::~VParam() {
    if (prop_) {
        prop_->removeObserver(this);
    }
}

void VParam::setProperty(VProperty* prop) {
    prop_ = prop;

    label_ = prop->param("label");

    if (!colourPropName_.isEmpty()) {
        if (VProperty* p = prop->findChild(colourPropName_)) {
            p->addObserver(this);
            colour_ = p->value().value<QColor>();
        }
    }

    if (!fontColourPropName_.isEmpty()) {
        if (VProperty* p = prop->findChild(fontColourPropName_)) {
            p->addObserver(this);
            fontColour_ = p->value().value<QColor>();
        }
    }
    if (!typeColourPropName_.isEmpty()) {
        if (VProperty* p = prop->findChild(typeColourPropName_)) {
            p->addObserver(this);
            typeColour_ = p->value().value<QColor>();
        }
    }
}

void VParam::notifyChange(VProperty* prop) {
    if (prop->name() == colourPropName_) {
        colour_ = prop->value().value<QColor>();
    }

    else if (prop->name() == fontColourPropName_) {
        fontColour_ = prop->value().value<QColor>();
    }

    else if (prop->name() == typeColourPropName_) {
        typeColour_ = prop->value().value<QColor>();
    }
}
