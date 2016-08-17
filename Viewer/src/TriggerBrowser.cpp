//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerBrowser.hpp"

#include <QFile>
#include <QTextStream>

TriggerBrowser::TriggerBrowser(QWidget* parent) : QTextBrowser(parent)
{
    setOpenExternalLinks(false);
    setOpenLinks(false);
    setReadOnly(true);

    //Set css for the text formatting
    QString cssDoc;
    QFile f(":/viewer/trigger.css");
    QTextStream in(&f);
    if(f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cssDoc=QString(f.readAll());
    }
    f.close();
    document()->setDefaultStyleSheet(cssDoc);
}
