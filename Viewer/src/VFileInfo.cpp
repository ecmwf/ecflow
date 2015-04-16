//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VFileInfo.hpp"

#include <QDateTime>

QString  VFileInfo::formatSize() const
{
	return formatSize(size());
}

QString  VFileInfo::formatModDate() const
{
	QDateTime dt=lastModified();
	return dt.toString("yyyy-MM-dd hh:mm");
}

QString  VFileInfo::formatPermissions() const
{
	QString str(permission(QFile::ReadOwner)?"r":"-");
	str+=(permission(QFile::WriteOwner)?"w":"-");
	str+=(permission(QFile::ExeOwner)?"x":"-");
	str+=(permission(QFile::ReadGroup)?"r":"-");
	str+=(permission(QFile::WriteGroup)?"w":"-");
	str+=(permission(QFile::ExeGroup)?"x":"-");
	str+=(permission(QFile::ReadOther)?"r":"-");
	str+=(permission(QFile::WriteOther)?"w":"-");
	str+=(permission(QFile::ExeOther)?"x":"-");

	return str;
}

QString VFileInfo::formatSize(unsigned int size)
{
  	if(size < 1024)
	  	return QString::number(size) + " B";
	else if(size < 1024*1024)
	  	return QString::number(size/1024) + " KB";
	else if(size < 1024*1024*1024)
	  	return QString::number(size/(1024*1024)) + " MB";
	else
	  	return QString::number(size/(1024*1024*1024)) + " GB";

 	return QString();
}

QString VFileInfo::formatDate(const std::time_t& t)
{
  	QDateTime dt=QDateTime::fromTime_t(t);
	return dt.toString("yyyy-MM-dd hh:mm");
}
