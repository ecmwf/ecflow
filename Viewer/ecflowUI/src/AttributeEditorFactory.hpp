/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_AttributeEditorFactory_HPP
#define ecflow_viewer_AttributeEditorFactory_HPP

#include <string>

#include "VInfo.hpp"

class AttributeEditor;
class QWidget;

class AttributeEditorFactory {
public:
    explicit AttributeEditorFactory(const std::string& type);
    virtual ~AttributeEditorFactory() = default;

    virtual AttributeEditor* make(VInfo_ptr, QWidget*) = 0;
    static AttributeEditor* create(const std::string&, VInfo_ptr, QWidget*);

private:
    explicit AttributeEditorFactory(const AttributeEditorFactory&)   = delete;
    AttributeEditorFactory& operator=(const AttributeEditorFactory&) = delete;
};

template <class T>
class AttributeEditorMaker : public AttributeEditorFactory {
    AttributeEditor* make(VInfo_ptr info, QWidget* parent) override { return new T(info, parent); }

public:
    explicit AttributeEditorMaker(const std::string& t)
        : AttributeEditorFactory(t) {}
};

#endif /* ecflow_viewer_AttributeEditorFactory_HPP */
