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

      void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
      void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

protected Q_SLOTS:
      void fetchJobStatusFile();

protected:
      void updateState(const ChangeFlags&) override;
      void reloadRequested() override;
      void runStatusCommand();

      QTimer* timer_;
      int timeout_;
};

#endif // JOBSTATUSITEMWIDGET_HPP
