//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef INFOPANELITEM_HPP_
#define INFOPANELITEM_HPP_

#include "ViewNodeInfo.hpp"

class QWidget;

class InfoPanelItem
{
public:
	InfoPanelItem(){};
	virtual ~InfoPanelItem(){};

	virtual void reload(ViewNodeInfo_ptr node)=0;
	virtual QWidget* realWidget()=0;
};

#endif
