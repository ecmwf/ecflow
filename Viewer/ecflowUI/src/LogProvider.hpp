/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_LogProvider_HPP
#define ecflow_viewer_LogProvider_HPP

#include <QObject>
#include <QStringList>

#include "InfoProvider.hpp"
#include "VDir.hpp"
#include "VInfo.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

class FileWatcher;

class LogProvider : public QObject, public InfoProvider {
    Q_OBJECT

public:
    explicit LogProvider(InfoPresenter* owner, QObject* parent = nullptr);

    void visit(VInfoServer*) override;
    void clear() override;
    void setAutoUpdate(bool) override;

public Q_SLOTS:
    void slotLinesAppend(QStringList);

private:
    void fetchFile();
    void fetchFile(ServerHandler* server, const std::string& fileName);
    void watchFile(const std::string&, size_t);
    void stopWatchFile();

    FileWatcher* fileWatcher_;
};

#endif /* ecflow_viewer_LogProvider_HPP */
