//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEVIEWBASE_HPP_
#define NODEVIEWBASE_HPP_

#include "Viewer.hpp"
#include "ViewNodeInfo.hpp"

class QWidget;

class NodeViewBase
{
public:
		NodeViewBase();
		virtual ~NodeViewBase(){};

		virtual void reload()=0;
		virtual QWidget* realWidget()=0;
		virtual ViewNodeInfo_ptr currentSelection()=0;

protected:
		Viewer::ViewMode id_;

};

#endif
