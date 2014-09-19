#ifndef NODEVIEWHANDLER_HPP_
#define NODEVIEWHANDLER_HPP_

#include <map>
#include <QString>
#include "Viewer.hpp"
#include "ViewNodeInfo.hpp"

class QStackedLayout;
class QWidget;
class NodeViewBase;

using namespace std;

class NodeViewHandler
{
public:
    NodeViewHandler(QStackedLayout*);

	void add(Viewer::ViewMode,NodeViewBase*);
	Viewer::ViewMode currentMode() const {return currentMode_;}
	bool setCurrentMode(Viewer::ViewMode);
	bool setCurrentMode(int);
	NodeViewBase* currentBase() const {return base(currentMode_);}
	QList<QWidget*> uniqueWidgets();
	ViewNodeInfo_ptr currentSelection();

private:
	NodeViewBase* base(Viewer::ViewMode) const;
	QWidget* widget(Viewer::ViewMode);

	Viewer::ViewMode  currentMode_;
  	std::map<Viewer::ViewMode,NodeViewBase*> bases_;
	std::map<Viewer::ViewMode,QWidget*> widgets_;
	std::map<Viewer::ViewMode,int> indexes_;
	QStackedLayout* stacked_;
};

#endif
