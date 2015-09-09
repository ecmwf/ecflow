//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_FILEWATCHER_HPP_
#define VIEWER_SRC_FILEWATCHER_HPP_

#include <QFileSystemWatcher>
#include <QFile>

class FileWatcher : public QFileSystemWatcher
{
Q_OBJECT

public:
	FileWatcher(const std::string& filePath,qint64 offset,QObject* parent);

protected Q_SLOTS:
	void slotChanged(const QString& path);

Q_SIGNALS:
	void linesAppended(QStringList);

protected:
	QFile file_;
	qint64 offset_;
};


#endif /* VIEWER_SRC_FILEWATCHER_HPP_ */
