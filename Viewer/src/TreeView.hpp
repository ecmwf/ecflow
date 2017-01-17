//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_TREEVIEW_HPP_
#define VIEWER_SRC_TREEVIEW_HPP_

#include <QTreeView>

class TreeView : public QTreeView
{
public:
	explicit TreeView(QWidget* parent=0);
};

#endif /* VIEWER_SRC_TREEVIEW_HPP_ */
