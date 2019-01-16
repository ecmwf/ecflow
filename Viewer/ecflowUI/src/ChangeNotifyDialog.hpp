//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef CHANGENOTIFYDIALOG_HPP_
#define CHANGENOTIFYDIALOG_HPP_

#include <QDialog>
#include <QLinearGradient>
#include <QSettings>
#include <QToolButton>
#include <QWidget>

#include "ui_ChangeNotifyDialog.h"
#include "ui_ChangeNotifyDialogWidget.h"

#include "VInfo.hpp"

class ChangeNotify;
class VProperty;

class QButtonGroup;
class QHBoxLayout;
class QLabel;

class ChangeNotifyDialogButton : public QToolButton
{
Q_OBJECT

public:
    explicit ChangeNotifyDialogButton(QWidget* parent=nullptr);

    void setNotifier(ChangeNotify*);
    void updateSettings();

public Q_SLOTS:
    void slotAppend();
    void slotRemoveRow(int);
    void slotReset();

protected:
    ChangeNotify* notifier_{nullptr};
};

class ChangeNotifyDialogWidget : public QWidget, protected Ui::ChangeNotifyDialogWidget
{
 Q_OBJECT

public:
	explicit ChangeNotifyDialogWidget(QWidget* parent=nullptr);
    ~ChangeNotifyDialogWidget() override = default;

	void init(ChangeNotify*);
    void updateSettings();
	ChangeNotify* notifier() const {return notifier_;}
    void writeSettings(QSettings& settings);
    void readSettings(const QSettings& settings);

protected Q_SLOTS:
    void slotSelectItem(const QModelIndex&);
    void slotDoubleClickItem(const QModelIndex&);

Q_SIGNALS:
    void selectionChanged(VInfo_ptr);

protected:
	ChangeNotify* notifier_{nullptr};
};

class ChangeNotifyDialog : public QDialog, protected Ui::ChangeNotifyDialog
{
Q_OBJECT

public:
	explicit ChangeNotifyDialog(QWidget *parent=nullptr);
	~ChangeNotifyDialog() override;

    void add(ChangeNotify*);
    void setCurrent(ChangeNotify*);
    void setEnabled(ChangeNotify*,bool b);
	void updateSettings(ChangeNotify*);

public Q_SLOTS:
	void on_closePb__clicked(bool b);
	void on_clearPb__clicked(bool b);

protected Q_SLOTS:
    void slotSelectionChanged(VInfo_ptr);
    void slotOptions();
    void slotButtonToggled(int,bool);

protected:
    ChangeNotify* indexToNtf(int idx);
    int ntfToIndex(ChangeNotify* ntf);
	void closeEvent(QCloseEvent*) override;
    void clearCurrentData();
	void writeSettings();
	void readSettings();
    void readNtfWidgetSettings(int tabIndex);

    QList<ChangeNotifyDialogWidget*> ntfWidgets_;
    QList<ChangeNotifyDialogButton*> ntfButtons_;
    QButtonGroup* buttonGroup_;
	bool ignoreCurrentChange_{false};
	QLinearGradient grad_;
    QHBoxLayout* buttonHb_;
    VProperty* switchWsProp_{nullptr};
};

#endif
