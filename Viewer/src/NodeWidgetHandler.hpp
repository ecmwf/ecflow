//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef NODEWIDGETHANDLER_HPP_
#define NODEWIDGETHANDLER_HPP_

#include "Viewer.hpp"

#include <QSplitter>
#include <QMainWindow>

#include "VInfo.hpp"
#include "VSettings.hpp"

#include <boost/property_tree/ptree.hpp>

class NodeViewHandler;
class NodeWidget;
class VConfig;
class LayoutManager;
class VSettings;

class NodeWidgetHandler : public QMainWindow
{
    Q_OBJECT

public:
  	NodeWidgetHandler(QString,QWidget* parent=0);
	~NodeWidgetHandler();

	void reload();

	Viewer::ViewMode viewMode();
	void setViewMode(Viewer::ViewMode);
	VConfig* config() const {return config_;}
	VInfo_ptr currentSelection();
	void currentSelection(VInfo_ptr n);

	void writeSettings(VSettings*);
	void readSettings(VSettings*);

public Q_SLOTS:
	//void slotFolderReplacedInView(Folder*);
	//void slotFolderChanged(Folder*);

Q_SIGNALS:

    void selectionChanged(VInfo_ptr);

private:

//NodeViewHandler* handler_;
	VConfig* config_;

	LayoutManager* lm_;

	QList<NodeWidget*> widgets_;
};

class LayoutManager
{
public:
	LayoutManager(QWidget *parent);
	void add(QWidget*);

protected:
	QWidget* parent_;
	QList<QWidget*> widgets_;
	QSplitter* splitter_;
	QSplitter* splitterRight_;
};






#endif
