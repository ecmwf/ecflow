//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef ATTRIBUTEEDITORFACTORY_HPP
#define ATTRIBUTEEDITORFACTORY_HPP

#include <string>
#include "VInfo.hpp"

class AttributeEditor;
class QWidget;

class AttributeEditorFactory
{
public:
    explicit AttributeEditorFactory(const std::string& type);
    virtual ~AttributeEditorFactory();

    virtual AttributeEditor* make(VInfo_ptr,QWidget*) = 0;
    static AttributeEditor* create(const std::string&,VInfo_ptr,QWidget*);

private:
    explicit AttributeEditorFactory(const AttributeEditorFactory&);
    AttributeEditorFactory& operator=(const AttributeEditorFactory&);
};

template<class T>
class AttributeEditorMaker : public AttributeEditorFactory
{
    AttributeEditor* make(VInfo_ptr info,QWidget* parent) { return new T(info,parent); }
public:
    explicit AttributeEditorMaker(const std::string& t) : AttributeEditorFactory(t) {}
};

#endif // ATTRIBUTEEDITORFACTORY_HPP
