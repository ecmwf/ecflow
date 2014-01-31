#ifndef NODEVIEWHANDLER_HPP_
#define NODEVIEWHANDLER_HPP_

#include <map>
#include <QString>
#include "Viewer.hpp"

class QStackedLayout;
class QWidget;
class NodeViewBase;

using namespace std;

class NodeViewHandler
{
public:
    NodeViewHandler(QStackedLayout*);

	void add(Viewer::ViewMode,NodeViewBase*,QWidget*);
	Viewer::ViewMode currentMode() const {return currentMode_;}
	void setCurrentMode(Viewer::ViewMode);
	NodeViewBase* currentBase() const {return base(currentMode_);}

private:
	NodeViewBase* base(Viewer::ViewMode) const;
	QWidget* widget(Viewer::ViewMode);

	Viewer::ViewMode  currentMode_;
  	std::map<Viewer::ViewMode,NodeViewBase*> bases_;
	std::map<Viewer::ViewMode,QWidget*> widgets_;
	QStackedLayout* stacked_;
};

#endif
