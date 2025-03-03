/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "AboutDialog.hpp"

#include <QDate>
#include <QProcessEnvironment>
#include <QTreeWidgetItem>

#include "DirectoryHandler.hpp"
#include "WidgetNameProvider.hpp"
#include "ecflow/core/Version.hpp"

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
    #include <QRegExp>
#endif
#include <QRegularExpression>
#include <QRegularExpressionMatch>

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setupUi(this);

    QString title         = "EcflowUI";
    QString ecfVersionTxt = QString::fromStdString(ecf::Version::full());
    QString desc          = QString::fromStdString(ecf::Version::description());
    QString descTxt       = "<b>ecflow version:</b> " + ecfVersionTxt;

#ifdef ECF_OPENSSL
    descTxt += "<br><b>OpenSSL:</b> enabled";
#else
    descTxt += "<br><b>OpenSSL:</b> disabled";
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    QRegularExpression rx(R"(boost\((\S+)\))");
    auto match = rx.match(desc);
    if (match.hasMatch()) {
        descTxt += "<br><b>boost version:</b> " + match.captured(1);
    }

    rx    = QRegularExpression(R"(compiler\(([^\)]+)\))");
    match = rx.match(desc);
    if (match.hasMatch()) {
        descTxt += "<br><b>compiler</b>: " + match.captured(1);
    }

    rx    = QRegularExpression(R"(protocol\((\S+)\))");
    match = rx.match(desc);
    if (match.hasMatch()) {
        descTxt += "<br><b>protocol:</b> " + match.captured(1);
    }

#else
    int pos = 0;
    QRegExp rx("boost\\((\\S+)\\)");
    if ((pos = rx.indexIn(desc, pos)) != -1) {
        descTxt += "<br><b>boost version:</b> " + rx.cap(1);
    }

    rx = QRegExp("compiler\\(([^\\)]+)\\)");
    if ((pos = rx.indexIn(desc, pos)) != -1) {
        descTxt += "<br><b>compiler</b>: " + rx.cap(1);
    }

    rx = QRegExp("protocol\\((\\S+)\\)");
    if ((pos = rx.indexIn(desc, pos)) != -1) {
        descTxt += "<br><b>protocol:</b> " + rx.cap(1);
    }
#endif
    descTxt += "<br><b>compiled on:</b> " + desc.section("Compiled on", 1, 1);

    const char* qtv = qVersion();
    if (qtv) {
        descTxt += "<br><b>Qt version: </b>" + QString(qtv);
    }

    QString logoTxt = "<table><tr><td><img src=\':/viewer/logo.png\'></td>&nbsp;&nbsp;&nbsp;&nbsp;<td></td><td>";
    logoTxt += "<h2>" + title + "</h2>";
    if (!ecfVersionTxt.isEmpty()) {
        logoTxt += "<p>ecflow version: <b>" + ecfVersionTxt + "</b><br>";
        logoTxt += "<i>Copyright 2009-" + QString::number(QDate::currentDate().year()) + " ECMWF</i><p>";
    }

    logoTxt += "</td></tr></table>";
    logoLabel_->setText(logoTxt);

    versionLabel_->setText(descTxt);

    QString licenseText = "Copyright 2009-" + QString::number(QDate::currentDate().year()) + " ECMWF.";
    licenseText += " This software is licensed under the terms of the Apache Licence version 2.0";
    licenseText += " which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.";
    licenseText += " In applying this licence, ECMWF does not waive the privileges and immunities";
    licenseText += " granted to it by virtue of its status as an intergovernmental organisation";
    licenseText += " nor does it submit to any jurisdiction.";

    licenseLabel_->setText(licenseText);

    WidgetNameProvider::nameChildren(this);

    // Paths
    std::string pathText = "<b>Log file:</b> ";

    if (DirectoryHandler::uiLogFileName().empty())
        pathText += "<i>not defined (log is written to stdout)</i><br>";
    else
        pathText += DirectoryHandler::uiLogFileName() + " <br>";

    pathText += "<b>UI event log file:</b> " + DirectoryHandler::uiEventLogFileName() + " <br>";
    pathText += "<b>Socket directory:</b> " + DirectoryHandler::socketDir() + " <br>";

    pathLabel_->setText(QString::fromStdString(pathText));

    // Env vars
    envTree_->setRootIsDecorated(false);
    envTree_->setColumnCount(2);
    QStringList envCols;
    envCols << "Variable"
            << "Value";
    envTree_->setHeaderLabels(envCols);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    Q_FOREACH (QString envKey, env.keys()) {
        if (envKey.startsWith("ECFLOWUI_") || envKey.contains("PROXY") || envKey == "ECFLOW_SYSTEM_SERVERS_LIST") {
            QString envVal = env.value(envKey);
            auto* item     = new QTreeWidgetItem(envTree_);
            item->setText(0, envKey);
            item->setText(1, envVal);
        }
    }

    envTree_->resizeColumnToContents(0);

    tabWidget->setCurrentIndex(0);
}
