//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AttributeEditorFactory.hpp"

#include <map>

static std::map<std::string,AttributeEditorFactory*>* makers = 0;

AttributeEditorFactory::AttributeEditorFactory(const std::string& type)
{
    if(makers == 0)
        makers = new std::map<std::string,AttributeEditorFactory*>;

    (*makers)[type] = this;
}

AttributeEditorFactory::~AttributeEditorFactory()
{
    // Not called
}

AttributeEditor* AttributeEditorFactory::create(const std::string& type,VInfo_ptr info,QWidget* parent)
{
    auto j = makers->find(type);
    if(j != makers->end())
        return (*j).second->make(info,parent);

    return 0;
}
