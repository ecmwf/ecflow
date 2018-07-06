//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TreeView.hpp"

#include <QPalette>

TreeView::TreeView(QWidget* parent) : QTreeView(parent)
{
	//!!!!We need to do it because:
	//The background colour between the views left border and the nodes cannot be
	//controlled by delegates or stylesheets. It always takes the QPalette::Highlight
	//colour from the palette. Here we set this to transparent so that Qt could leave
	//this are empty and we will fill it appropriately in our delegate.

	QPalette pal=palette();
	pal.setColor(QPalette::Highlight,QColor(255,255,255,0));
	setPalette(pal);
}


