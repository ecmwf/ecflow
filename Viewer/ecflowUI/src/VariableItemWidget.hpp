//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VARIABLEITEMWIDGET_HPP_
#define VARIABLEITEMWIDGET_HPP_

#include "ui_VariablePropDialog.h"
#include "ui_VariableAddDialog.h"
#include "ui_VariableItemWidget.h"

#include "InfoPanelItem.hpp"
#include "VariableModelDataObserver.hpp"
#include "VInfo.hpp"

class LineEdit;
class VariableModel;
class VariableModelData;
class VariableModelDataHandler;
class VariableSortModel;
class VariableSearchLine;
class VProperty;

class VariablePropDialog : public QDialog, public VariableModelDataObserver, private Ui::VariablePropDialog
{
Q_OBJECT

public:
    VariablePropDialog(VariableModelDataHandler* data,int defineIndex,QString name,QString value,bool frozen,QWidget* parent=nullptr);
    ~VariablePropDialog() override;

	QString name() const;
	QString value() const;

    void notifyCleared(VariableModelDataHandler*) override;
    void notifyUpdated(VariableModelDataHandler*) override;

public Q_SLOTS:
	void accept() override;
    void slotSuspendedChanged(bool s);

protected Q_SLOTS:
    void on_nameEdit__textEdited(QString);
    void on_valueEdit__textChanged();

protected:
    void suspendEdit(bool);
    void readSettings();
    void writeSettings();

    bool genVar_;
    VariableModelDataHandler* data_;
    int defineIndex_;
    QString oriName_;
    QString nodeName_;
    QString nodeType_;
    QString nodeTypeCapital_;
    QString defineNodeName_;
    QString defineNodeType_;
    bool cleared_;
    bool suspended_;

};

class VariableAddDialog : public QDialog, public VariableModelDataObserver, private Ui::VariableAddDialog
{
Q_OBJECT

public:
    VariableAddDialog(VariableModelDataHandler* data,QWidget* parent=nullptr);
    VariableAddDialog(VariableModelDataHandler* data,QString name,QString value,QWidget* parent=nullptr);
    ~VariableAddDialog() override;

	QString name() const;
	QString value() const;

    void notifyCleared(VariableModelDataHandler*) override;
    void notifyUpdated(VariableModelDataHandler*) override {}

public Q_SLOTS:
	void accept() override;
    void slotSuspendedChanged(bool s);

protected:
    void init();
    void suspendEdit(bool);
    void readSettings();
    void writeSettings();

    VariableModelDataHandler* data_;
    QString nodeName_;
    QString nodeType_;
    QString nodeTypeCapital_;
    bool cleared_;
    bool suspended_;
};


class VariableItemWidget : public QWidget, public InfoPanelItem, protected Ui::VariableItemWidget
{
Q_OBJECT

public:
	explicit VariableItemWidget(QWidget *parent=nullptr);
	~VariableItemWidget() override;

	void reload(VInfo_ptr) override;
	QWidget* realWidget() override;
    void clearContents() override;

public Q_SLOTS:	
    void slotFilterTextChanged(QString text);
	void slotItemSelected(const QModelIndex& idx,const QModelIndex& prevIdx);

protected Q_SLOTS:
    void on_actionProp_triggered();
    void on_actionAdd_triggered();
    void on_actionDelete_triggered();
    void on_varView_doubleClicked(const QModelIndex& index);
    void on_actionFilter_triggered();
    void on_actionSearch_triggered();
    void on_actionCopy_triggered();
    void on_actionCopyFull_triggered();
    void on_shadowTb_clicked(bool showShadowed);
    void slotVariableEdited();
    void slotVariableAdded();

Q_SIGNALS:
    void suspendedChanged(bool);

protected:
	void checkActionState();
	void editItem(const QModelIndex& index);
	void duplicateItem(const QModelIndex& index);
	void addItem(const QModelIndex& index);
	void removeItem(const QModelIndex& index);
    void updateState(const ChangeFlags&) override;
    void toClipboard(QString txt) const;
    void regainSelection();
    void saveExpandState();
    void restoreExpandState();

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override;
	void defsChanged(const std::vector<ecf::Aspect::Type>&) override;

	VariableModelDataHandler* data_;
	VariableModel* model_;
	VariableSortModel* sortModel_;

	LineEdit* filterLine_;
	VariableSearchLine *searchLine_;

    VProperty* shadowProp_{nullptr};
    VInfo_ptr lastSelection_;
    bool canSaveLastSelection_{true};
    QList<bool> expanded_;
};

#endif

