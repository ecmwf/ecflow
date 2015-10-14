//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef MODELCOLUMN_H
#define MODELCOLUMN_H

class VProperty;

#include <assert.h>

#include <QList>
#include <QString>


class ModelColumnItem
{
friend class ModelColumn;
public:
	explicit ModelColumnItem(const std::string& id);

protected:
	QString label_;
	QString id_;
	int index_;
	QString icon_;
	QString tooltip_;
};


class ModelColumn
{
public:
	explicit ModelColumn(const std::string& id);

	int count() const {return items_.size();}
	int indexOf(const std::string&) const;
	QString id(int i) const {assert(i>=0 && i < count()); return items_.at(i)->id_;}
	QString label(int i) const {assert(i>=0 && i < count()); return items_.at(i)->label_;}

	static ModelColumn* def(const std::string& id);

	//Called from VConfigLoader
	static void load(VProperty* group);

protected:
	std::string id_;
	QList<ModelColumnItem*> items_;
};


#endif
