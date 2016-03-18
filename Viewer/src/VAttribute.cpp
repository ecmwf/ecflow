//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VAttribute.hpp"

#include "VNode.hpp"

VAttribute::VAttribute(VNode* parent,int index) : VItem(parent), index_(index)
{
    data_=parent_->getAttributeData(index_,type_) ;
}

