//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "Animation.hpp"

#include <QWidget>

//=====================================================
//
// Animation
//
//=====================================================

Animation::Animation(QWidget *view,Type type) :
   view_(view),
   type_(type)
{
	if(type_ == ServerLoadType)
		setFileName(":/viewer/spinning_wheel.gif");

	setScaledSize(QSize(12,12));

	connect(this,SIGNAL(frameChanged(int)),
			this,SLOT(renderFrame(int)));

	connect(this,SIGNAL(repaintRequest(Animation*)),
			view_,SLOT(slotRepaint(Animation*)));
}

void Animation::addTarget(const QModelIndex& idx)
{
	if(!targets_.contains(idx))
		targets_ << idx;

	start();
}

void Animation::removeTarget(const QModelIndex& idx)
{
	targets_.removeAll(idx);

	if(targets_.isEmpty())
		QMovie::stop();
}

void Animation::renderFrame(int frame)
{
	if(targets_.isEmpty())
		QMovie::stop();

	Q_EMIT(repaintRequest(this));
}

//=====================================================
//
// AnimationHandler
//
//=====================================================

AnimationHandler::AnimationHandler(QWidget* view) : view_(view)
{

}

AnimationHandler::~AnimationHandler()
{
	Q_FOREACH(Animation* an,items_)
	{
		delete an;
	}
}

Animation* AnimationHandler::find(Animation::Type type, bool makeIt)
{
	QMap<Animation::Type,Animation*>::iterator it=items_.find(type);
	if(it != items_.end())
	{
		return it.value();
	}
	else if(makeIt)
	{
		Animation* an=new Animation(view_,type);
		items_[type]=an;
		return an;
	}

	return 0;
}
