//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEWIDGET_HPP_
#define NODEWIDGET_HPP_

#include "Viewer.hpp"

#include <QWidget>

#include "ViewNodeInfo.hpp"

#include <boost/property_tree/ptree.hpp>

class NodeViewHandler;
class VConfig;

class NodeWidget : public QWidget
{
    Q_OBJECT

public:
  	NodeWidget(QString,QWidget* parent=0);
	~NodeWidget();

	void reload();

	Viewer::ViewMode viewMode();
	void setViewMode(Viewer::ViewMode);
	VConfig* config() const {return config_;}

	void save(boost::property_tree::ptree &pt);
	void load(const boost::property_tree::ptree &pt);

public slots:
	//void slotFolderReplacedInView(Folder*);
	//void slotFolderChanged(Folder*);

signals:
	//void iconCommandRequested(QString,IconObjectH);
	//void desktopCommandRequested(QString,QPoint);
	//void itemInfoChanged(QString);
	//void pathChanged();
    void selectionChanged(ViewNodeInfo_ptr);

private:
	//QModelIndex changeFolder(const QModelIndex&);

	NodeViewHandler* views_;
	VConfig* config_;
};

#endif
