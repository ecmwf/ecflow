//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VFILEINFO_H_
#define VFILEINFO_H_

#include <QFileInfo>

#include <ctime>

class VFileInfo : public QFileInfo
{
public:
	explicit VFileInfo(const QString& file) : QFileInfo(file) {}

	QString formatSize() const;
	QString formatModDate() const;
	QString formatPermissions() const;

	static QString formatSize(unsigned int size);
	static QString formatDate(const std::time_t& t);
	static QString formatDateAgo(const std::time_t& t);
};

#endif
