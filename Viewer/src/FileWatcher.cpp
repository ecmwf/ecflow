//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "FileWatcher.hpp"

FileWatcher::FileWatcher(const std::string& filePath,qint64 offset,QObject* parent) :
   QFileSystemWatcher(parent),
   offset_(offset)
{
	connect(this,SIGNAL(fileChanged(QString)),
			this,SLOT(slotChanged(QString)));

	file_.setFileName(QString::fromStdString(filePath));
	if (!file_.open(QIODevice::ReadOnly | QIODevice::Text))
		   return;
	file_.seek(offset_);

	addPath(file_.fileName());
}

void FileWatcher::slotChanged(const QString& path)
{
	QStringList lst;
	if(path == file_.fileName())
	{
		while (!file_.atEnd())
		    lst << file_.readLine();
	}

	Q_EMIT linesAppended(lst);

}
