/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_JobStatusItemWidget_HPP
#define ecflow_viewer_JobStatusItemWidget_HPP

#include "CodeItemWidget.hpp"
#include "InfoPanelItem.hpp"

class MessageLabel;
class QTimer;

class JobStatusItemWidget : public CodeItemWidget, public InfoPanelItem {
    Q_OBJECT
public:
    explicit JobStatusItemWidget(QWidget* parent = nullptr);
    ~JobStatusItemWidget() override;

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;

    // From VInfoPresenter
    void infoReady(VReply*) override;
    void infoFailed(VReply*) override;
    void infoProgress(VReply*) override;

    void nodeChanged(const VNode* n, const std::vector<ecf::Aspect::Type>& aspect) override;
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

protected Q_SLOTS:
    void fetchJobStatusFile();

protected:
    void updateState(const ChangeFlags&) override;
    void reloadRequested() override;
    void commandRequested() override;
    void startFileFetchTask();
    void finishFileFetchTask();
    void startStatusCommandTask();
    bool checkStatusCommandTask(VReply* reply);

    enum TaskMode { NoTask, FetchFileTask, StatusCommandTask };
    enum StatusCommandMode { UnsetCommandMode, EnabledCommandMode, DisabledCommandMode };

    InfoProvider* statusProvider_;
    MessageLabel* statusCommandLabel_;
    QTimer* timer_;
    int timeout_;
    int timeoutCount_;
    int maxTimeoutCount_;
    TaskMode taskMode_;
    StatusCommandMode nodeStatusMode_;
};

#endif /* ecflow_viewer_JobStatusItemWidget_HPP */
