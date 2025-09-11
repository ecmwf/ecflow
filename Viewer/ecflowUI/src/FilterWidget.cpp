/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "FilterWidget.hpp"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "ServerFilter.hpp"
#include "ServerItem.hpp"
#include "UiLog.hpp"
#include "VAttributeType.hpp"
#include "VConfig.hpp"
#include "VFilter.hpp"
#include "VIcon.hpp"
#include "VNState.hpp"

//===========================================
//
// VParamFilterMenu
//
//===========================================

VParamFilterMenu::VParamFilterMenu(QMenu* parent,
                                   VParamSet* filter,
                                   QString title,
                                   ItemMode itemMode,
                                   DecorMode decorMode)
    : menu_(parent),
      filter_(filter),
      itemMode_(itemMode),
      decorMode_(decorMode) {
    buildTitle(title, parent);

    auto* acSep = new QAction(this);
    acSep->setSeparator(true);
    menu_->addAction(acSep);

    // Param name must be unique
    for (auto it : filter_->all()) {
        addAction(it->label(), it->name());
    }

    // For status
    if (decorMode_ == ColourDecor) {
        QLinearGradient grad;
        grad.setCoordinateMode(QGradient::ObjectBoundingMode);
        grad.setStart(0, 0);
        grad.setFinalStop(0, 1);

        int lighter       = 150;
        bool useStateGrad = true;
        if (VProperty* p = VConfig::instance()->find("view.common.node_gradient")) {
            useStateGrad = p->value().toBool();
        }

        QFont f;
        QFontMetrics fm(f);
        int pixSize = fm.height() - 2;

        Q_FOREACH (QAction* ac, menu_->actions()) {
            if (!ac->isSeparator()) {
                if (VNState* vs = VNState::find(ac->data().toString().toStdString())) {
                    // Fill rect
                    QColor bg      = vs->colour();
                    QColor bgLight = bg.lighter(lighter);
                    QColor border  = bg.darker(125);
                    QBrush bgBrush;
                    if (useStateGrad) {
                        grad.setColorAt(0, bgLight);
                        grad.setColorAt(1, bg);
                        bgBrush = QBrush(grad);
                    }
                    else {
                        bgBrush = QBrush(bg);
                    }

                    QPixmap pix(pixSize, pixSize);
                    QPainter painter(&pix);

                    QRect fillRect(0, 0, pixSize, pixSize);
                    painter.fillRect(fillRect, bgBrush);
                    painter.setPen(border);
                    painter.drawRect(fillRect.adjusted(0, 0, -1, -1));
                    ac->setIcon(pix);
                }
            }
        }
    }

    // For icons
    else if (decorMode_ == PixmapDecor) {
        QFont f;
        QFontMetrics fm(f);
        int pixSize = fm.height() - 2;

        Q_FOREACH (QAction* ac, menu_->actions()) {
            if (!ac->isSeparator()) {
                if (VIcon* vs = VIcon::find(ac->data().toString().toStdString())) {
                    QPixmap pix(vs->pixmap(pixSize));
                    ac->setIcon(pix);
                }
            }
        }
    }

    auto* ac = new QAction(this);
    ac->setSeparator(true);
    menu_->addAction(ac);

    selectAllAc_ = new QAction(this);
    if (itemMode_ == FilterMode) {
        selectAllAc_->setText(tr("Select all"));
    }
    else {
        selectAllAc_->setText(tr("Show all"));
    }
    menu_->addAction(selectAllAc_);
    connect(selectAllAc_, SIGNAL(triggered(bool)), this, SLOT(slotSelectAll(bool)));

    unselectAllAc_ = new QAction(this);
    if (itemMode_ == FilterMode) {
        unselectAllAc_->setText(tr("Clear filter"));
    }
    else {
        unselectAllAc_->setText(tr("Hide all"));
    }
    menu_->addAction(unselectAllAc_);
    connect(unselectAllAc_, SIGNAL(triggered(bool)), this, SLOT(slotUnselectAll(bool)));

    reload();
}

void VParamFilterMenu::buildTitle(QString title, QMenu* parent) {
    auto* titleLabel = new QLabel(title, menu_);
    QFont f          = menu_->font();
    f.setBold(true);
    titleLabel->setFont(f);
    titleLabel->setAlignment(Qt::AlignHCenter);
    titleLabel->setAutoFillBackground(true);
    QPalette pal = titleLabel->palette();
    pal.setColor(QPalette::Window, QColor(237, 238, 238));
    titleLabel->setPalette(pal);

    int titlePadding = 3;
    int topMargin    = 2;
    if (parent && parent->isTearOffEnabled()) {
        titlePadding = 1;
        topMargin    = 0;
    }

    QString titleQss = "QLabel {padding: " + QString::number(titlePadding) + "px;}";
    titleLabel->setStyleSheet(titleQss);

    auto* w  = new QWidget(menu_);
    auto* vb = new QVBoxLayout(w);
    vb->setContentsMargins(2, topMargin, 2, 2);
    // vb->addSpacing(2);
    vb->addWidget(titleLabel);
    // vb->addSpacing(2);

    auto* titleAc = new QWidgetAction(menu_);
    // Qt doc says: the ownership of the widget is passed to the widgetaction.
    // So when the action is deleted it will be deleted as well.
    titleAc->setDefaultWidget(w);
    // titleAc->setEnabled(false);
    menu_->addAction(titleAc);
}

void VParamFilterMenu::addAction(QString name, QString id) {
    auto* ac = new QAction(this);
    ac->setText(name);
    ac->setData(id);
    ac->setCheckable(true);
    ac->setChecked(false);

    menu_->addAction(ac);

    // It will not be emitted when setChecked is called!
    connect(ac, SIGNAL(triggered(bool)), this, SLOT(slotChanged(bool)));
}

void VParamFilterMenu::slotSelectAll(bool) {
    Q_FOREACH (QAction* ac, menu_->actions()) {
        if (!ac->isSeparator() && ac->isCheckable()) {
            ac->setChecked(true);
        }
    }

    slotChanged(true);
}

void VParamFilterMenu::slotUnselectAll(bool) {
    Q_FOREACH (QAction* ac, menu_->actions()) {
        if (!ac->isSeparator() && ac->isCheckable()) {
            ac->setChecked(false);
        }
    }

    slotChanged(true);
}

void VParamFilterMenu::slotChanged(bool) {
    std::vector<std::string> items;
    Q_FOREACH (QAction* ac, menu_->actions()) {
        if (!ac->isSeparator() && ac->isCheckable() && ac->isChecked()) {
            items.push_back(ac->data().toString().toStdString());
        }
    }

    if (filter_) {
        filter_->setCurrent(items);
    }

    checkActionState();
}

void VParamFilterMenu::reload() {
    Q_FOREACH (QAction* ac, menu_->actions()) {
        if (!ac->isSeparator()) {
            ac->setChecked(filter_->isSet(ac->data().toString().toStdString()));
        }
    }
    checkActionState();
}

void VParamFilterMenu::checkActionState() {
    if (filter_) {
        selectAllAc_->setEnabled(!filter_->isComplete());
        unselectAllAc_->setEnabled(!filter_->isEmpty());
    }
}

//===========================================
//
// ServerFilterMenu
//
//===========================================

ServerFilterMenu::ServerFilterMenu(QMenu* parent) : menu_(parent), filter_(nullptr) {
    loadFont_.setBold(true);

    allMenu_ = new QMenu("All servers", menu_);
    menu_->addMenu(allMenu_);

    auto* acFavSep = new QAction(this);
    acFavSep->setSeparator(true);
    menu_->addAction(acFavSep);

    auto* acFavTitle = new QAction(this);
    acFavTitle->setText(tr("Favourite or loaded servers"));
    QFont f = acFavTitle->font();
    f.setBold(true);
    acFavTitle->setFont(f);

    menu_->addAction(acFavTitle);

    init();

    ServerList::instance()->addObserver(this);
}

ServerFilterMenu::~ServerFilterMenu() {
    ServerList::instance()->removeObserver(this);
    if (filter_) {
        filter_->removeObserver(this);
    }
}

void ServerFilterMenu::aboutToDestroy() {
    ServerList::instance()->removeObserver(this);
    if (filter_) {
        filter_->removeObserver(this);
    }

    clear();
}

void ServerFilterMenu::clear() {
    Q_FOREACH (QAction* ac, acAllMap_) {
        allMenu_->removeAction(ac);
        delete ac;
    }
    acAllMap_.clear();

    clearFavourite();
}

void ServerFilterMenu::clearFavourite() {
    Q_FOREACH (QAction* ac, acFavMap_) {
        menu_->removeAction(ac);
        delete ac;
    }
    acFavMap_.clear();
}

void ServerFilterMenu::init() {
    clear();

    for (int i = 0; i < ServerList::instance()->count(); i++) {
        ServerItem* item = ServerList::instance()->itemAt(i);
        QString name     = QString::fromStdString(item->name());
        QAction* ac      = createAction(name, i);
        acAllMap_[name]  = ac;
    }

    Q_FOREACH (QAction* ac, acAllMap_) {
        allMenu_->addAction(ac);
    }

    buildFavourite();
}

void ServerFilterMenu::buildFavourite() {
    clearFavourite();

    for (int i = 0; i < ServerList::instance()->count(); i++) {
        ServerItem* item = ServerList::instance()->itemAt(i);
        if (item->isFavourite() || (filter_ && filter_->isFiltered(item))) {
            QString name    = QString::fromStdString(item->name());
            acFavMap_[name] = createAction(name, i);
        }
    }

    Q_FOREACH (QAction* ac, acFavMap_) {
        menu_->addAction(ac);
    }
}

QAction* ServerFilterMenu::createAction(QString name, int id) {
    auto* ac = new QAction(this);
    ac->setText(name);
    ac->setData(id);
    ac->setCheckable(true);
    ac->setChecked(false);

    // It will not be emitted when setChecked is called!!
    connect(ac, SIGNAL(triggered(bool)), this, SLOT(slotChanged(bool)));

    return ac;
}

void ServerFilterMenu::slotChanged(bool) {
    if (!filter_) {
        return;
    }

    if (auto* ac = static_cast<QAction*>(sender())) {
        if (ac->isSeparator()) {
            return;
        }

        if (ServerItem* item = ServerList::instance()->itemAt(ac->data().toInt())) {
            QString name = ac->text();
            bool checked = ac->isChecked();
            if (checked) {
                filter_->addServer(item);
            }
            else {
                filter_->removeServer(item);
            }

            // At this point the action (ac) might be deleted so
            // we need to use the name and check state for syncing
            syncActionState(name, checked);
        }
    }
}

void ServerFilterMenu::syncActionState(QString name, bool checked) {
    QMap<QString, QAction*>::const_iterator it = acAllMap_.find(name);
    if (it != acAllMap_.end() && it.value()->isChecked() != checked) {
        // Triggered() will not be called!!
        it.value()->setChecked(checked);
    }

    it = acFavMap_.find(name);
    if (it != acFavMap_.end() && it.value()->isChecked() != checked) {
        // Triggered() will not be called!!
        it.value()->setChecked(checked);
    }
}

// Reset actions state when a new filter is loaded
void ServerFilterMenu::reload(ServerFilter* filter) {
    // hide to avoid crashing. See ECFLOW-1839
    menu_->hide();

    if (filter_) {
        filter_->removeObserver(this);
    }

    filter_ = filter;

    if (filter_) {
        filter_->addObserver(this);
    }

    reload(false);
}

// Reset actions state when a new filter is loaded
void ServerFilterMenu::reload(bool favouriteBuilt) {
    // hide to avoid crashing. See ECFLOW-1839
    menu_->hide();

    if (!favouriteBuilt) {
        buildFavourite();
    }

    QMap<QString, QAction*>::const_iterator it = acAllMap_.constBegin();
    while (it != acAllMap_.constEnd()) {
        if (ServerItem* item = ServerList::instance()->find(it.value()->text().toStdString())) {
            bool current = it.value()->isChecked();
            if (current != filter_->isFiltered(item)) {
                // Triggered() will not be called!!
                it.value()->setChecked(!current);
                it.value()->setFont(current ? font_ : loadFont_);
            }
        }
        ++it;
    }

    it = acFavMap_.constBegin();
    while (it != acFavMap_.constEnd()) {
        if (ServerItem* item = ServerList::instance()->find(it.value()->text().toStdString())) {
            bool current = it.value()->isChecked();
            if (current != filter_->isFiltered(item)) {
                // Triggered() will not be called!!
                it.value()->setChecked(!current);
                it.value()->setFont(current ? font_ : loadFont_);
            }
        }
        ++it;
    }
}

void ServerFilterMenu::notifyServerListChanged() {
    init();
    reload(true);
}

void ServerFilterMenu::notifyServerListFavouriteChanged(ServerItem*) {
    reload(false);
}

void ServerFilterMenu::notifyServerFilterAdded(ServerItem*) {
    reload(false);
}

void ServerFilterMenu::notifyServerFilterRemoved(ServerItem*) {
    reload(false);
}

void ServerFilterMenu::notifyServerFilterChanged(ServerItem*) {
    reload(false);
}

void ServerFilterMenu::notifyServerFilterDelete() {
    reload(nullptr);
}
