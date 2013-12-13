#ifndef VIEWERMAINWINDOW_HPP_
#define VIEWERMAINWINDOW_HPP_

//============================================================================
// Copyright 2013 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================

#include <QMainWindow>
#include <QTreeWidget>

#include "Defs.hpp"

class ViewerMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    ViewerMainWindow();
    void printDefTree(const std::string &server, int port);
    void printNode(node_ptr node, int indent, QTreeWidgetItem *parent);


private:
    QTreeWidget *treeWidget_;

};

#endif 
