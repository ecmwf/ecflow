//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TEXTFILTERWIDGET_HPP
#define TEXTFILTERWIDGET_HPP

#include "ui_TextFilterWidget.h"

#include <QWidget>
#include <QCompleter>
#include <QToolButton>

#include "TextFilterHandler.hpp"

class TextFilterCompleterModel;

class TextFilterWidget : public QWidget, private Ui::TextFilterWidget
{
Q_OBJECT

public:
    explicit TextFilterWidget(QWidget *parent=nullptr);
    ~TextFilterWidget() override = default;

    enum FilterStatus {EditStatus,FoundStatus,NotFoundStatus};
    void setStatus(FilterStatus,bool force=false);

    void setEditFocus();
    void buildMenu(QToolButton *tb);
    void setExternalButtons(QToolButton* statusTb,QToolButton* optionTb);
    void setEnabledExternalButtons(bool);
    QString filterText() const;
    bool isActive() const;
    bool isCaseSensitive() const;
    bool isMatched() const;
    void closeIt();

public Q_SLOTS:
    void slotFilterEditor();
    void on_le__textChanged();
    void on_le__returnPressed();
    void on_closeTb__clicked();
    void on_matchCb__currentIndexChanged(int);
    void on_caseCb__stateChanged(int);
    void slotOptionTb();
    void slotStatusTb(bool);

Q_SIGNALS:
    void runRequested(QString,bool,bool);
    void clearRequested();
    void closeRequested();
    void hideRequested();
    void statusChanged(FilterStatus);

protected:
    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *) override;

private:
    void init(const TextFilterItem& item);
    void runIt();
    void refreshCompleter();
    void addCurrentToLatest();
    bool isCurrentSaved() const;
    void adjustToolTip();
    void addMenuSection(QMenu* menu,const std::vector<TextFilterItem>& items,
                        QString title,QString data);

    FilterStatus status_{EditStatus};
    QBrush oriBrush_;
    QBrush redBrush_;
    QBrush greenBrush_;
    QCompleter* completer_;
    TextFilterCompleterModel* completerModel_;
    QToolButton* statusTb_{nullptr};
    QToolButton* optionTb_{nullptr};
};

#endif // TEXTFILTERWIDGET_HPP
