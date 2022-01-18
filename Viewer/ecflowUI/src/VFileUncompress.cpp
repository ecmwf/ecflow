//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VFileUncompress.hpp"

#include <QtGlobal>
#include <QFileInfo>
#include <QProcess>
#include <QCursor>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

#include "UiLog.hpp"

bool VFileUncompress::isCompressed(QString sourceFile)
{
    return sourceFile.endsWith(".gz",Qt::CaseInsensitive) ||
            sourceFile.endsWith(".Z", Qt::CaseInsensitive);
}

VFile_ptr VFileUncompress::uncompress(QString sourceFile, QString& errStr)
{
    QFileInfo info(sourceFile);
    QString suffix = info.suffix().toLower();

    if(suffix != "gz" && suffix != "z")
    {
        VFile_ptr z;
        return z;
    }

    VFile_ptr targetFile = VFile::createTmpFile(true); //will be deleted automatically
    QProcess proc;
    proc.setStandardOutputFile(QString::fromStdString(targetFile->path()));

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

    QString cmd;

    if (suffix == "gz")
    {
        cmd = "gunzip";
    }
    else
    {
        cmd = "uncompress";
    }

    proc.start("/bin/sh",
        QStringList() <<  "-c" << cmd + " -c \'" + sourceFile  + "\' ");

    if(!proc.waitForStarted(1000))
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
        UI_FUNCTION_LOG
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        errStr="Failed to start uncompressing using command:<br> \'" +
                             proc.program() + " " + proc.arguments().join(" ") + "\'";
#else
        errStr="Failed to start " + cmd  + " command!";
#endif
        targetFile.reset();
        return targetFile;
    }

    proc.waitForFinished(60000);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QGuiApplication::restoreOverrideCursor();
#endif

    QString err=proc.readAllStandardError();
    if(proc.exitStatus() != QProcess::NormalExit || !errStr.isEmpty())
    {
        UI_FUNCTION_LOG
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        errStr="Failed to filter output file using command:<br> \'" +
                             proc.program() + " " + proc.arguments().join(" ") + "\'";
#else
        errStr="Failed to run " + cmd + " ommand!";
#endif
        if(!errStr.isEmpty())
            errStr+="<br>Error message: " + err;

        targetFile.reset(); //delete
    }

    return targetFile;
}
