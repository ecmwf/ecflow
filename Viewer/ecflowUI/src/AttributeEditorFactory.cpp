/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "AttributeEditorFactory.hpp"

#include <map>

static std::map<std::string, AttributeEditorFactory*>* makers = nullptr;

AttributeEditorFactory::AttributeEditorFactory(const std::string& type) {
    if (makers == nullptr) {
        makers = new std::map<std::string, AttributeEditorFactory*>;
    }

    (*makers)[type] = this;
}

AttributeEditor* AttributeEditorFactory::create(const std::string& type, VInfo_ptr info, QWidget* parent) {
    auto j = makers->find(type);
    if (j != makers->end()) {
        return (*j).second->make(info, parent);
    }

    return nullptr;
}
