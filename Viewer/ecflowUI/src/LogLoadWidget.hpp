//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef LOGLOADWIDGET_HPP
#define LOGLOADWIDGET_HPP

#include <map>
#include <string>
#include <vector>

#include <QAbstractItemModel>
#include <QByteArray>
#include <QGraphicsItem>
#include <QMap>
#include <QScrollArea>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QWidget>
#include <QtCharts>
#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using namespace QtCharts;
#endif

#include "LogLoadData.hpp"
#include "TimelineFileList.hpp"
#include "VFile.hpp"

class LogLoadData;
class LogLoadDataItem;
class LogLoadSuiteModel;
class LogLoadRequestModel;
class LogModel;
class LogRequestView;
class LogRequestViewHandler;
class ServerLoadView;
class VComboSettings;
class VFileTransfer;
class VProperty;
class QSortFilterProxyModel;
class QHBoxLayout;
class QVBoxLayout;
class QComboBox;
class QToolBox;
class QTextBrowser;
class QTableView;
class QTabWidget;
class QTreeView;
class QSplitter;
class QLabel;

namespace Ui {
class LogLoadWidget;
}

struct LogLoadPrevState
{
    LogLoadPrevState() : valid(false), fullStart(false), fullEnd(false) {}
    bool valid;
    QDateTime startDt;
    QDateTime endDt;
    bool fullStart;
    bool fullEnd;
};

// the main widget containing all components
class LogLoadWidget : public QWidget {
    Q_OBJECT

public:
    explicit LogLoadWidget(QWidget* parent = nullptr);
    ~LogLoadWidget() override;

    void clear();
    void initLoad(QString serverName,
                  QString host,
                  QString port,
                  QString logFile,
                  const std::vector<std::string>& suites,
                  QString remoteUid,
                  int maxReadSize,
                  const std::string& nodePath,
                  bool detached);
    QString logFile() const { return logFile_; }

    enum LogMode { LatestMode, ArchiveMode };
    void setLogMode(LogMode logMode);
    void setDetached(bool);

    void writeSettings(VComboSettings* vs);
    void readSettings(VComboSettings* vs);

protected Q_SLOTS:
    void periodWasReset();
    void periodChanged(qint64 start, qint64 end);
    void resolutionChanged(int);
    void slotReload();
    void slotLogMode(int);
    void slotFileTransferFinished();
    void slotFileTransferFailed(QString);
    void slotFileTransferStdOutput(QString msg);
    void slotLogLoadProgress(size_t current, size_t total);
    void slotCancelFileTransfer();
    void slotLoadCustomFile();
    void showLogView(bool b);
    void slotExpandFileInfo(bool st);

private:
    void clearData(bool usePrevState);
    void reloadLatest(bool canUsePrevState);
    void loadLatest(bool usePrevState);
    void loadArchive();
    void loadCore(QString logFile);
    void initFromData();
    void setAllVisible(bool);
    void updateInfoLabel(bool showDetails = true);
    void checkButtonState();
    void setMaxReadSize(int maxReadSizeInMb);
    bool shouldShowLog() const;
    void updateTimeLabel(QDateTime, QDateTime);
    void initSplitter();

    enum TabIndex { TotalTab = 0, SuiteTab = 1, SubReqTab = 2 };

    Ui::LogLoadWidget* ui_;
    QString serverName_;
    QString host_;
    QString port_;
    QString logFile_;
    VFile_ptr tmpLogFile_;
    size_t maxReadSize_{100 * 1024 * 1024};
    std::vector<std::string> suites_;
    QString remoteUid_;
    LogMode logMode_{LatestMode};
    TimelineFileList archiveLogList_;
    QString currentNodePath_;

    LogRequestViewHandler* viewHandler_;
    QMap<TabIndex, ServerLoadView*> views_;
    LogModel* logModel_{nullptr};
    bool beingCleared_{false};

    bool localLog_{true};
    bool logLoaded_{false};
    bool logTransferred_{false};
    VFileTransfer* fileTransfer_{nullptr};
    QDateTime transferredAt_;

    LogLoadPrevState prevState_;
    bool detached_{false};

    VProperty* expandFileInfoProp_{nullptr};
};

#endif // LOGLOADWIDGET_HPP
