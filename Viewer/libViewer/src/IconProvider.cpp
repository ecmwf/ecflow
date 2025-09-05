/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "IconProvider.hpp"

#include <QDebug>
#include <QImage>
#include <QImageReader>
#include <QPainter>

#include "UiLog.hpp"

static UnknownIconItem unknownIcon(":/desktop/unknown.svg");
static IconItem linkIcon(":/desktop/link.svg");
static IconItem linkBrokenIcon(":/desktop/link_broken.svg");
static IconItem lockIcon(":/viewer/padlock.svg");
static IconItem warningIcon(":/viewer/warning.svg");
static IconItem errorIcon(":/viewer/error.svg");
static IconItem bookmarkGroupIcon(":/desktop/bookmark_group.svg");
static IconItem embeddedIcon(":/desktop/embedded.svg");
static IconItem infoIcon(":/viewer/info.svg");

std::map<QString, IconItem*> IconProvider::icons_;
std::map<int, IconItem*> IconProvider::iconsById_;

static int idCnt = 0;

//===========================================
//
// IconItem
//
//===========================================

IconItem::IconItem(QString path) : path_(path), id_(idCnt++) {
}

QPixmap IconItem::pixmap(int size) {
    auto it = pixmaps_.find(size);
    if (it != pixmaps_.end()) {
        return it->second;
    }
    else {
        QPixmap pix;
        QImageReader imgR(path_);
        if (imgR.canRead()) {
            imgR.setScaledSize(QSize(size, size));
            QImage img = imgR.read();
            pix        = QPixmap::fromImage(img);
        }
        else {
            pix = unknown(size);
        }

        pixmaps_[size] = pix;
        return pix;
    }
    return {};
}

QPixmap IconItem::pixmapToHeight(int size) {
    auto it = pixmapsByHeight_.find(size);
    if (it != pixmapsByHeight_.end()) {
        return it->second;
    }
    else {
        QPixmap pix;
        QImageReader imgR(path_);
        if (imgR.canRead()) {
            int w   = imgR.size().width();
            int h   = imgR.size().height();
            float r = static_cast<float>(w) / static_cast<float>(h);
            imgR.setScaledSize(QSize(size * r, size));
            QImage img = imgR.read();
            pix        = QPixmap::fromImage(img);
        }
        else {
            pix = unknown(size);
        }

        pixmapsByHeight_[size] = pix;
        return pix;
    }
    return {};
}

QPixmap IconItem::unknown(int size) {
    return unknownIcon.pixmap(size);
}

UnknownIconItem::UnknownIconItem(QString path) : IconItem(path) {
}

QPixmap UnknownIconItem::unknown(int /*size*/) {
    return {};
}

//===========================================
//
// IconProvider
//
//===========================================

IconProvider::IconProvider() = default;

IconProvider::~IconProvider() {
    // Important!
    //
    // The life time of the singleton instance 'iconProvider' is used to manage the life time of loaded Icons.
    // This means that when the application closes, after main() has finished, the destruction of the instance clears
    // the Icons.
    //
    for (auto icon : icons_) {
        delete icon.second;
    }
}

QString IconProvider::path(int id) {
    auto it = iconsById_.find(id);
    if (it != iconsById_.end()) {
        return it->second->path();
    }

    return {};
}

int IconProvider::add(QString path, QString name) {
    auto it = icons_.find(name);
    if (it == icons_.end()) {
        auto* p             = new IconItem(path);
        icons_[name]        = p;
        iconsById_[p->id()] = p;
        return p->id();
    }

    return it->second->id();
}

IconItem* IconProvider::icon(QString name) {
    auto it = icons_.find(name);
    if (it != icons_.end()) {
        return it->second;
    }

    return &unknownIcon;
}

IconItem* IconProvider::icon(int id) {
    auto it = iconsById_.find(id);
    if (it != iconsById_.end()) {
        return it->second;
    }

    return &unknownIcon;
}

QPixmap IconProvider::pixmap(QString name, int size) {
    return icon(name)->pixmap(size);
}

QPixmap IconProvider::pixmap(int id, int size) {
    return icon(id)->pixmap(size);
}

QPixmap IconProvider::pixmapToHeight(int id, int size) {
    return icon(id)->pixmapToHeight(size);
}

QPixmap IconProvider::lockPixmap(int size) {
    return lockIcon.pixmap(size);
}

QPixmap IconProvider::warningPixmap(int size) {
    return warningIcon.pixmap(size);
}

QPixmap IconProvider::errorPixmap(int size) {
    return errorIcon.pixmap(size);
}

QPixmap IconProvider::infoPixmap(int size) {
    return infoIcon.pixmap(size);
}

static IconProvider iconProvider;
