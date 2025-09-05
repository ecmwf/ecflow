/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "PropertyDialog.hpp"

#include <QAbstractButton>
#include <QCloseEvent>
#include <QSettings>

#include "ConfigListDelegate.hpp"
#include "IconProvider.hpp"
#include "PropertyEditor.hpp"
#include "SessionHandler.hpp"
#include "VConfig.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "ViewerUtil.hpp"
#include "WidgetNameProvider.hpp"

VProperty* PropertyDialog::prop_ = nullptr;

PropertyDialog::PropertyDialog(QWidget* parent) : QDialog(parent) {
    setupUi(this);

    QString wt = windowTitle();
    wt += "  -  " + QString::fromStdString(VConfig::instance()->appLongName());
    setWindowTitle(wt);

    QFont f;
    f.setBold(true);
    QFontMetrics fm(f);
    int maxW = ViewerUtil::textWidth(fm, "Server options ATAT");
    list_->setMaximumWidth(maxW + 6);
    list_->setFont(f);

    list_->setItemDelegate(new ConfigListDelegate(32, maxW, this));

    connect(list_,
            SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
            this,
            SLOT(slotChangePage(QListWidgetItem*, QListWidgetItem*)));

    connect(buttonBox_, SIGNAL(clicked(QAbstractButton*)), this, SLOT(slotButton(QAbstractButton*)));

    build();

    readSettings();

    if (list_->count() > 0 && list_->currentRow() == -1) {
        list_->setCurrentRow(0);
    }

    // Assign name to each object
    WidgetNameProvider::nameChildren(this);
}

void PropertyDialog::closeEvent(QCloseEvent* event) {
    event->accept();
    writeSettings();
}

// Build the property tree from the the definitions
void PropertyDialog::build() {
    if (prop_) {
        Q_FOREACH (VProperty* vPage, prop_->children()) {
            if (vPage->param("visible") == "false") {
                continue;
            }

            QPixmap pix(32, 32);
            QPixmap edPix;
            QString iconStr = vPage->param("icon");

            if (!iconStr.isEmpty()) {
                IconProvider::add(":/viewer/" + iconStr, iconStr);
                pix   = IconProvider::pixmap(iconStr, 32);
                edPix = IconProvider::pixmap(iconStr, 20);
            }

            auto* ed = new PropertyEditor(this);
            ed->setObjectName(vPage->param("label"));
            ed->edit(vPage, edPix);

            addPage(ed, pix, vPage->param("label"));
            editors_ << ed;
        }
    }
}

void PropertyDialog::apply() {
    manageChange(true);
}

void PropertyDialog::accept() {
    manageChange(false);
    writeSettings();
    QDialog::accept();
}

void PropertyDialog::reject() {
    writeSettings();
    QDialog::reject();
}

void PropertyDialog::addPage(QWidget* w, QPixmap pix, QString txt) {
    auto* item = new QListWidgetItem(list_);
    item->setData(Qt::DecorationRole, pix);
    item->setData(Qt::DisplayRole, txt);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    page_->addWidget(w);
}

void PropertyDialog::slotChangePage(QListWidgetItem* current, QListWidgetItem* previous) {
    if (!current) {
        current = previous;
    }

    page_->setCurrentIndex(list_->row(current));
}

void PropertyDialog::slotButton(QAbstractButton* pb) {
    if (buttonBox_->buttonRole(pb) == QDialogButtonBox::ApplyRole) {
        apply();
    }
}

void PropertyDialog::showPage(QString path) {
    QStringList lst = path.split(".");
    if (lst.count() > 0) {
        QString pageName = lst[0];
        for (int i = 0; i < editors_.count(); i++) {
            PropertyEditor* ed = editors_[i];
            if (VProperty* prop = ed->property()) {
                if (prop->name() == pageName) {
                    list_->setCurrentRow(i);
                    return;
                }
            }
        }
    }
}

void PropertyDialog::manageChange(bool inApply) {
    bool hasChange = false;
    Q_FOREACH (PropertyEditor* ed, editors_) {
        if (ed->applyChange()) {
            hasChange    = true;
            VProperty* p = ed->property();
            if (p && p->name() != "server") {
                if (inApply) {
                    Q_EMIT configChanged();
                }
                else {
                    configChanged_ = true;
                }
            }
        }
    }

    if (hasChange) {
        VConfig::instance()->saveSettings();
    }
}

void PropertyDialog::load(VProperty* p) {
    prop_ = p;
}

void PropertyDialog::writeSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("PropertyDialog")), QSettings::NativeFormat);

    // We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size", size());

    int idx = list_->currentRow();
    settings.setValue("current", idx);
    if (idx >= 0 && idx < editors_.count()) {
        settings.setValue("currentPageTab", editors_[idx]->currentTopLevelTabIndex());
    }
    settings.endGroup();
}

void PropertyDialog::readSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("PropertyDialog")), QSettings::NativeFormat);

    settings.beginGroup("main");
    if (settings.contains("size")) {
        resize(settings.value("size").toSize());
    }
    else {
        resize(QSize(550, 540));
    }

    if (settings.contains("current")) {
        int current = settings.value("current").toInt();
        if (current >= 0) {
            list_->setCurrentRow(current);
            if (settings.contains("currentPageTab")) {
                int currentPageTab = settings.value("currentPageTab").toInt();
                if (currentPageTab >= 0 && currentPageTab < editors_.count()) {
                    editors_[current]->setCurrentTopLevelTabIndex(currentPageTab);
                }
            }
        }
    }
    settings.endGroup();
}

static SimpleLoader<PropertyDialog> loader("gui");
