/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_WhyItemWidget_HPP
#define ecflow_viewer_WhyItemWidget_HPP

#include <QPlainTextEdit>

#include "HtmlItemWidget.hpp"
#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

class VNode;

class WhyItemWidget : public HtmlItemWidget, public InfoPanelItem {
    Q_OBJECT

public:
    explicit WhyItemWidget(QWidget* parent = nullptr);
    WhyItemWidget(const WhyItemWidget&) = delete;
    WhyItemWidget(WhyItemWidget&&)      = delete;
    ~WhyItemWidget() override;

    WhyItemWidget& operator=(const WhyItemWidget&) = delete;
    WhyItemWidget& operator=(WhyItemWidget&&)      = delete;

    void reload(VInfo_ptr) override;
    QWidget* realWidget() override;
    void clearContents() override;

    // From VInfoPresenter
    void infoReady(VReply*) override;
    void infoFailed(VReply*) override;
    void infoProgress(VReply*) override {}

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

protected Q_SLOTS:
    void anchorClicked(const QUrl& link);

protected:
    void updateState(const ChangeFlags&) override;
    void serverSyncFinished() override;

private:
    void load();
    QString why() const;
    QString makeHtml(const std::vector<std::string>&, const std::vector<std::string>&) const;
    QString makeHtml(const std::vector<std::string>&) const;

    QMap<QString, QString> stateMap_;
};

#endif /* ecflow_viewer_WhyItemWidget_HPP */
