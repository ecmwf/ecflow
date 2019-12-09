//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef JOBSTATUSITEMWIDGET_HPP
#define JOBSTATUSITEMWIDGET_HPP

#include "InfoPanelItem.hpp"
#include "CodeItemWidget.hpp"

class MessageLabel;
class QTimer;

class JobStatusItemWidget : public CodeItemWidget, public InfoPanelItem
{
    Q_OBJECT
public:
      explicit JobStatusItemWidget(QWidget *parent=nullptr);
      ~JobStatusItemWidget() override;

      void reload(VInfo_ptr) override;
      QWidget* realWidget() override;
      void clearContents() override;

      //From VInfoPresenter
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

      enum TaskMode {NoTask, FetchFileTask, StatusCommandTask};
      enum StatusCommandMode {UnsetCommandMode, EnabledCommandMode, DisabledCommandMode};

      InfoProvider* statusProvider_;
      MessageLabel* statusCommandLabel_;
      QTimer* timer_;
      int timeout_;
      int timeoutCount_;
      int maxTimeoutCount_;
      TaskMode taskMode_;
      StatusCommandMode nodeStatusMode_;
      bool fetchTried_;
};

#endif // JOBSTATUSITEMWIDGET_HPP
