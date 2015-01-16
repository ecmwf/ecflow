#ifndef NODEVIEWHANDLER_HPP_
#define NODEVIEWHANDLER_HPP_

#include <map>
#include <QString>
#include <QWidget>

#include "Viewer.hpp"
#include "VInfo.hpp"

class QStackedLayout;
class QWidget;

class AbstractNodeModel;
class NodeFilterModel;
class NodeViewBase;
class VConfig;

class NodeWidget : public QWidget
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
	NodeWidget(): model_(0), filterModel_(0), view_(0) {};

	AbstractNodeModel* model_;
	NodeFilterModel* filterModel_;
	NodeViewBase* view_;
};

class TreeNodeWidget : public NodeWidget
{
public:
	TreeNodeWidget(VConfig*,QWidget* parent=0);
};

class TableNodeWidget : public NodeWidget
{
public:
	TableNodeWidget(VConfig*,QWidget* parent=0);
};

class NodeViewHandler
{
public:
    NodeViewHandler(QStackedLayout*);

	void add(Viewer::ViewMode,NodeWidget*);
	Viewer::ViewMode currentMode() const {return currentMode_;}
	bool setCurrentMode(Viewer::ViewMode);
	bool setCurrentMode(int);
	NodeWidget* currentControl() const {return control(currentMode_);}

private:
	NodeWidget* control(Viewer::ViewMode) const;

	Viewer::ViewMode  currentMode_;
  	std::map<Viewer::ViewMode,NodeWidget*> controls_;
	std::map<Viewer::ViewMode,int> indexes_;
	QStackedLayout* stacked_;
};

#endif
