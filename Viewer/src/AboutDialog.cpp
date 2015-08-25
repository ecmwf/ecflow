//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AboutDialog.hpp"

#include "Version.hpp"

#include <QDate>
#include <QRegExp>

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);

    QString title="EcflowUI";
    QString ecfVersionTxt=QString::fromStdString(ecf::Version::raw());
    QString desc=QString::fromStdString(ecf::Version::description());
    QString descTxt="<b>ecflow version:</b> " + ecfVersionTxt;

    int pos=0;
    QRegExp rx("boost\\((\\S+)\\)");
    if((pos = rx.indexIn(desc, pos)) != -1)
    {
       descTxt+="<br><b>boost version:</b> " + rx.cap(1);
    }

    rx=QRegExp("compiler\\(([^\\)]+)\\)");
    if((pos = rx.indexIn(desc, pos)) != -1)
    {
        descTxt+="<br><b>compiler</b>: " + rx.cap(1);
    }

    rx=QRegExp("protocol\\((\\S+)\\)");
    if((pos = rx.indexIn(desc, pos)) != -1)
    {
        descTxt+="<br><b>protocol:</b> " + rx.cap(1);
    }

    descTxt+="<br><b>compiled on:</b> " +desc.section("Compiled on",1,1);

    const char *qtv=qVersion();
    if(qtv)
    {
    	descTxt+="<br><b>Qt version: </b>" + QString(qtv);
    }

    QString logoTxt;
    logoTxt+="<h3>&nbsp;&nbsp;" + title + "</h3>";
    if(!ecfVersionTxt.isEmpty())
    {
    	logoTxt+="<p>&nbsp;&nbsp;ecflow version: " + ecfVersionTxt + "</p>";
    }

    logoLabel_->setText(logoTxt);

    versionLabel_->setText(descTxt);

    QString licenseText="Copyright 2009-" + QString::number(QDate::currentDate().year()) + " ECMWF.";
    licenseText+=" This software is licensed under the terms of the Apache Licence version 2.0";
    licenseText+=" which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.";
    licenseText+=" In applying this licence, ECMWF does not waive the privileges and immunities";
    licenseText+=" granted to it by virtue of its status as an intergovernmental organisation";
    licenseText+=" nor does it submit to any jurisdiction.";

    licenseLabel_->setText(licenseText);
}
