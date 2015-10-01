//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef ANIMATION_HPP_
#define ANIMATION_HPP_

#include <QMap>
#include <QMovie>
#include <QModelIndex>

class Animation : public QMovie
{
Q_OBJECT

public:
	enum Type {ServerLoadType};
	Animation(QWidget*,Type);

	void addTarget(const QModelIndex&);
	void removeTarget(const QModelIndex&);
	QModelIndexList targets() const {return targets_;}

Q_SIGNALS:
	void repaintRequest(Animation*);

protected Q_SLOTS:
	void renderFrame(int);

protected:
	QWidget* view_;
	QModelIndexList targets_;
	Type type_;
};

class AnimationHandler
{
public:
	explicit AnimationHandler(QWidget* view);
	~AnimationHandler();
	Animation* find(Animation::Type,bool makeIt);

protected:
	QWidget* view_;
	QMap<Animation::Type,Animation*> items_;
};

#endif
