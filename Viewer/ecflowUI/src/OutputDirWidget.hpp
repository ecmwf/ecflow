//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef OUTPUTDIRWIDGET_HPP
#define OUTPUTDIRWIDGET_HPP

#include <QWidget>

#include "VFile.hpp"
#include "VDir.hpp"

class QModelIndex;
class QTimer;

class OutputFileProvider;
class OutputDirProvider;
class OutputFetchInfo;
class OutputModel;
class OutputSortModel;
class VProperty;
class DirWidgetState;
class VReply;

namespace Ui {
    class OutputDirWidget;
}

class OutputDirWidget : public QWidget
{
Q_OBJECT
    friend class DirWidgetState;
    friend class DirWidgetSuccessState;
    friend class DirWidgetFirstFailedState;
    friend class DirWidgetFailedState;
    friend class DirWidgetIdleState;
    friend class DirWidgetDisabledState;

public:
    explicit OutputDirWidget(QWidget *parent);

    void clear();
    void load(VReply* reply, const std::string& joboutFile);
    void failed(VReply* reply, const std::string& joboutFile);

    bool isEmpty() const;

    bool currentSelection(std::string& fPath, VDir::FetchMode& dMode) const;
    void adjustCurrentSelection(VFile_ptr loadedFile);

    void startTimer();
    void stopTimer();

public Q_SLOTS:
    void showIt(bool);
    void reload();

protected Q_SLOTS:
    void closeByButton();
    void slotItemSelected(const QModelIndex& /*idx1*/,const QModelIndex& /*idx2*/);

Q_SIGNALS:
    void updateRequested();
    void itemSelected();
    void closedByButton();

protected:
    void updateContents(const std::vector<VDir_ptr>&, bool);
    QString formatErrors(const std::vector<std::string>& errorVec) const;
    void transitionTo(DirWidgetState* state);

    Ui::OutputDirWidget* ui_{nullptr};
    DirWidgetState* state_{nullptr};
    OutputModel* dirModel_{nullptr};
    OutputSortModel* dirSortModel_{nullptr};
    std::string joboutFile_;
    bool ignoreOutputSelection_{false};
    QTimer* updateTimer_{nullptr};
    static int updateDirTimeout_;
    OutputFetchInfo* fetchInfo_{nullptr};
    bool dirColumnsAdjusted_{false};

private:
     void adjustCurrentSelection(const std::string& fPath, VFile::FetchMode fMode);
     void setCurrentSelection(const std::string& fPath, VFile::FetchMode fMode);
};

#endif // OUTPUTDIRWIDGET_HPP
