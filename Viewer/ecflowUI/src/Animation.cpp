/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Animation.hpp"

#include <QWidget>

#include "ServerHandler.hpp"
#include "VNode.hpp"

//=====================================================
//
// Animation
//
//=====================================================

Animation::Animation(QWidget* view, Type type)
    : view_(view),
      type_(type) {
    if (type_ == ServerLoadType) {
        setFileName(":/viewer/spinning_wheel.gif");
    }

    setScaledSize(QSize(12, 12));

    connect(this, SIGNAL(frameChanged(int)), this, SLOT(renderFrame(int)));

    connect(this, SIGNAL(repaintRequest(Animation*)), view_, SLOT(slotRepaint(Animation*)));
}

void Animation::addTarget(VNode* n) {
    Q_ASSERT(n);
    if (!targets_.contains(n)) {
        targets_ << n;
    }

    ServerHandler* s = n->server();
    Q_ASSERT(s);
    s->addServerObserver(this);

    start();
}

void Animation::removeTarget(VNode* n) {
    targets_.removeAll(n);

    if (targets_.isEmpty()) {
        QMovie::stop();
    }
}

void Animation::renderFrame(int /*frame*/) {
    if (targets_.isEmpty()) {
        QMovie::stop();
    }

    Q_EMIT(repaintRequest(this));
}

void Animation::notifyServerDelete(ServerHandler* server) {
    removeTarget(server->vRoot());
    server->removeServerObserver(this);
    // TODO: make it work for nodes as well
}

void Animation::notifyBeginServerClear(ServerHandler* /*server*/) {
    // TODO: make it work for nodes
}

//=====================================================
//
// AnimationHandler
//
//=====================================================

AnimationHandler::AnimationHandler(QWidget* view)
    : view_(view) {
}

AnimationHandler::~AnimationHandler() {
    Q_FOREACH (Animation* an, items_) {
        delete an;
    }
}

Animation* AnimationHandler::find(Animation::Type type, bool makeIt) {
    QMap<Animation::Type, Animation*>::iterator it = items_.find(type);
    if (it != items_.end()) {
        return it.value();
    }
    else if (makeIt) {
        auto* an     = new Animation(view_, type);
        items_[type] = an;
        return an;
    }

    return nullptr;
}
