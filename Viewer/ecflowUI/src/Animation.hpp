/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_Animation_HPP
#define ecflow_viewer_Animation_HPP

#include <QMap>
#include <QModelIndex>
#include <QMovie>

#include "ServerObserver.hpp"
#include "VNode.hpp"

class VNode;

class Animation : public QMovie, public ServerObserver {
    Q_OBJECT

public:
    enum Type { ServerLoadType };
    Animation(QWidget*, Type);

    void addTarget(VNode*);
    void removeTarget(VNode*);
    QList<VNode*> targets() const { return targets_; }

    void notifyDefsChanged(ServerHandler*, const std::vector<ecf::Aspect::Type>&) override {}
    void notifyServerDelete(ServerHandler* server) override;
    void notifyBeginServerClear(ServerHandler* server) override;

Q_SIGNALS:
    void repaintRequest(Animation*);

protected Q_SLOTS:
    void renderFrame(int);

protected:
    QWidget* view_;
    QList<VNode*> targets_;
    Type type_;
};

class AnimationHandler {
public:
    explicit AnimationHandler(QWidget* view);
    ~AnimationHandler();
    Animation* find(Animation::Type, bool makeIt);

protected:
    QWidget* view_;
    QMap<Animation::Type, Animation*> items_;
};

#endif /* ecflow_viewer_Animation_HPP */
