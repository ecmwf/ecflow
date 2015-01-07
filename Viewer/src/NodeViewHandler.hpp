#ifndef NODEVIEWHANDLER_HPP_
#define NODEVIEWHANDLER_HPP_

#include <map>
#include <QString>
#include "Viewer.hpp"
#include "VInfo.hpp"

class QStackedLayout;
class QWidget;

class AbstractNodeModel;
class NodeFilterModel;
class NodeViewBase;
class VConfig;

class NodeViewControl
{
public:
	void active(bool);
	bool active() const;
	NodeViewBase* view() const {return view_;}
	QWidget* widget();
	VInfo_ptr currentSelection();
	void currentSelection(VInfo_ptr info);
	void reload();

protected:
	NodeViewControl(): model_(0), filterModel_(0), view_(0) {};

	AbstractNodeModel* model_;
	NodeFilterModel* filterModel_;
	NodeViewBase* view_;
};

class TreeNodeViewControl : public NodeViewControl
{
public:
	TreeNodeViewControl(VConfig*,QWidget* parent=0);
};

class TableNodeViewControl : public NodeViewControl
{
public:
	TableNodeViewControl(VConfig*,QWidget* parent=0);
};

class NodeViewHandler
{
public:
    NodeViewHandler(QStackedLayout*);

	void add(Viewer::ViewMode,NodeViewControl*);
	Viewer::ViewMode currentMode() const {return currentMode_;}
	bool setCurrentMode(Viewer::ViewMode);
	bool setCurrentMode(int);
	NodeViewControl* currentControl() const {return control(currentMode_);}

private:
	NodeViewControl* control(Viewer::ViewMode) const;

	Viewer::ViewMode  currentMode_;
  	std::map<Viewer::ViewMode,NodeViewControl*> controls_;
	std::map<Viewer::ViewMode,int> indexes_;
	QStackedLayout* stacked_;
};

#endif
