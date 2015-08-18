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
#include "VInfo.hpp"

class QWidget;
class NodeFilterModel;
class NodeFilterDef;
class VSettings;

class NodeViewBase
{
public:
		explicit NodeViewBase(NodeFilterModel *model,NodeFilterDef*filterDef);
		virtual ~NodeViewBase(){};

		virtual void reload()=0;
		virtual void rerender()=0;
		virtual QWidget* realWidget()=0;
		virtual VInfo_ptr currentSelection()=0;
		virtual void selectFirstServer()=0;
		virtual void currentSelection(VInfo_ptr n)=0;

		virtual void readSettings(VSettings* vs)=0;

protected:
		NodeFilterModel* model_;
		NodeFilterDef* filterDef_;
};

#endif
