//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "NodeQueryOptionEdit.hpp"

#include <QDateTimeEdit>
#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QToolButton>
#include <QWidgetAction>
#include <QtGlobal>

#include "ComboMulti.hpp"
#include "CustomListWidget.hpp"
#include "NodeQuery.hpp"
#include "NodeQueryOption.hpp"
#include "StringMatchCombo.hpp"
#include "ViewerUtil.hpp"

NodeQueryOptionEdit::NodeQueryOptionEdit(QString optionId, QGridLayout* grid, QWidget* parent)
    : QObject(parent), optionId_(optionId), initIsOn_(false), parent_(parent), grid_(grid) {
    connect(this, SIGNAL(changed()), parent_, SLOT(slotOptionEditChanged()));
}

void NodeQueryOptionEdit::setQuery(NodeQuery* query) {
    init(query->option(optionId_));
}

NodeQueryStringOptionEdit::NodeQueryStringOptionEdit(NodeQueryOption* option,
                                                     QGridLayout* grid,
                                                     QWidget* parent,
                                                     bool sameRow)
    : NodeQueryOptionEdit(option->name(), grid, parent) {
    label_   = new QLabel(option->label() + ":", parent_);
    matchCb_ = new StringMatchCombo(parent_);
    matchCb_->setProperty("options", "1");

    le_ = new QLineEdit(parent_);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    le_->setClearButtonEnabled(true);
    // QWidgetAction *wa=new QWidgetAction(this);
    // StringMatchTb* tb=new StringMatchTb(parent_);
    // wa->setDefaultWidget(tb);
    // le_->addAction(wa,QLineEdit::LeadingPosition);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    le_->setPlaceholderText(tr("ANY"));
#endif

    int row = grid_->rowCount();
    if (!sameRow) {
        grid_->addWidget(label_, row, 0);
        grid_->addWidget(matchCb_, row, 1);
        grid_->addWidget(le_, row, 2, 1, 1);
    }
    else {
        row = row - 1;
        Q_ASSERT(row >= 0);
        grid_->addWidget(label_, row, 3);
        grid_->addWidget(matchCb_, row, 4);
        grid_->addWidget(le_, row, 5, 1, -1);
    }

    connect(le_, SIGNAL(textChanged(QString)), this, SLOT(slotEdited(QString)));

    connect(matchCb_, SIGNAL(currentIndexChanged(int)), this, SLOT(slotMatchChanged(int)));

    initInternal(option);
    Q_ASSERT(option_);
}

void NodeQueryStringOptionEdit::initInternal(NodeQueryOption* option) {
    initIsOn_ = true;
    option_   = static_cast<NodeQueryStringOption*>(option);
    Q_ASSERT(option_);
    Q_ASSERT(optionId_ == option_->name());
    le_->setText(option_->value());
    matchCb_->setMatchMode(option_->matchMode());
    initIsOn_ = false;
}

void NodeQueryStringOptionEdit::slotEdited(QString val) {
    if (initIsOn_)
        return;

    if (option_) {
        option_->setValue(val);
        Q_EMIT changed();
    }
}

void NodeQueryStringOptionEdit::slotMatchChanged(int val) {
    if (initIsOn_)
        return;

    if (option_) {
        option_->setMatchMode(matchCb_->matchMode(val));
        Q_EMIT changed();
    }
}

void NodeQueryStringOptionEdit::clear() {
    le_->clear();
}

void NodeQueryStringOptionEdit::setVisible(bool st) {
    label_->setVisible(st);
    matchCb_->setVisible(st);
    le_->setVisible(st);
}

//===================================================================
//
//
//==================================================================

NodeQueryListOptionEdit::NodeQueryListOptionEdit(NodeQueryOption* option,
                                                 CustomListWidget* list,
                                                 QToolButton* tb,
                                                 QWidget* parent)
    : NodeQueryOptionEdit(option->name(), nullptr, parent), list_(list), resetTb_(tb) {
    option_ = static_cast<NodeQueryListOption*>(option);
    Q_ASSERT(option_);

    connect(list_, SIGNAL(itemSelectionChanged()), this, SLOT(slotListChanged()));

    connect(resetTb_, SIGNAL(clicked()), list_, SLOT(clearSelection()));

    list_->addItems(option_->values(), false);
    resetTb_->setEnabled(list_->hasSelection());

    initInternal(option_);
}

void NodeQueryListOptionEdit::initInternal(NodeQueryOption* option) {
    initIsOn_ = true;
    option_   = static_cast<NodeQueryListOption*>(option);
    Q_ASSERT(option_);
    Q_ASSERT(option_->name() == optionId_);
    list_->setSelectionWithList(option_->selection());
    initIsOn_ = false;
}

void NodeQueryListOptionEdit::clear() {
    list_->clearSelection();
}

void NodeQueryListOptionEdit::slotListChanged() {
    resetTb_->setEnabled(list_->hasSelection());
    if (initIsOn_)
        return;

    if (option_) {
        option_->setSelection(list_->selection());
        Q_EMIT changed();
    }
}

//===================================================================
//
// NodeQueryComboOptionEdit
//
//==================================================================

NodeQueryComboOptionEdit::NodeQueryComboOptionEdit(NodeQueryOption* option, QGridLayout* grid, QWidget* parent)
    : NodeQueryOptionEdit(option->name(), grid, parent) {
    option_ = static_cast<NodeQueryComboOption*>(option);
    Q_ASSERT(option_);

    label_  = new QLabel(option->label() + ":", parent_);
    cb_     = new QComboBox(parent_);

    int row = grid_->rowCount();
    grid_->addWidget(label_, row, 0);
    grid_->addWidget(cb_, row, 1, 1, -1);

    connect(cb_, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCbChanged(int)));

    QStringList vals   = option_->values();
    QStringList labels = option_->valueLabels();
    for (int i = 0; i < vals.count(); i++) {
        cb_->addItem(labels[i], vals[i]);
    }

    initInternal(option_);
}

void NodeQueryComboOptionEdit::initInternal(NodeQueryOption* option) {
    initIsOn_ = true;
    option_   = static_cast<NodeQueryComboOption*>(option);
    Q_ASSERT(option_);
    Q_ASSERT(option_->name() == optionId_);

    for (int i = 0; i < cb_->count(); i++) {
        if (cb_->itemData(i).toString() == option_->value()) {
            cb_->setCurrentIndex(i);
            break;
        }
    }
    initIsOn_ = false;
}

void NodeQueryComboOptionEdit::slotCbChanged(int idx) {
    if (initIsOn_)
        return;

    if (option_) {
        option_->setValue(cb_->itemData(idx).toString());
        Q_EMIT changed();
    }
}

void NodeQueryComboOptionEdit::setVisible(bool st) {
    label_->setVisible(st);
    cb_->setVisible(st);
}

//===================================================================
//
// NodeQueryPeriodOptionEdit
//
//==================================================================

NodeQueryPeriodOptionEdit::NodeQueryPeriodOptionEdit(NodeQueryOption* option, QGridLayout* grid, QWidget* parent)
    : NodeQueryOptionEdit(option->name(), grid, parent) {
    int row = grid_->rowCount();

    label_  = new QLabel(option->label() + ": ", parent_);

    modeCb_ = new QComboBox(parent);
    modeCb_->addItem("any time", "any");
    modeCb_->addItem("in the last", "last");
    modeCb_->addItem("in period", "period");
    modeCb_->addItem("older than", "older");

    connect(modeCb_, SIGNAL(currentIndexChanged(int)), this, SLOT(modeChanged(int)));

    holder_  = new QWidget(parent);
    auto* hb = new QHBoxLayout(holder_);
    hb->setContentsMargins(0, 0, 0, 0);

    // last/older period
    valueSpin_ = new QSpinBox(holder_);
    valueSpin_->setRange(1, 100000);
    hb->addWidget(valueSpin_);

    unitsCb_ = new QComboBox(holder_);
    unitsCb_->addItem(tr("minutes"), "minute");
    unitsCb_->addItem(tr("hours"), "hour");
    unitsCb_->addItem(tr("days"), "day");
    unitsCb_->addItem(tr("weeks"), "week");
    unitsCb_->addItem(tr("months"), "month");
    unitsCb_->addItem(tr("years"), "year");
    hb->addWidget(unitsCb_);

    connect(valueSpin_, SIGNAL(valueChanged(int)), this, SLOT(periodValueChanged(int)));

    connect(unitsCb_, SIGNAL(currentIndexChanged(int)), this, SLOT(periodUnitsChanged(int)));

    // Period: from-to
    periodFromDe_ = new QDateTimeEdit(holder_);
    periodFromDe_->setCalendarPopup(true);
    periodFromDe_->setDisplayFormat("yyyy-MM-dd hh:mm");
    periodFromDe_->setDate(QDate::currentDate());
    periodFromDe_->setMaximumDate(QDate::currentDate());
    hb->addWidget(periodFromDe_);

    periodToDe_ = new QDateTimeEdit(holder_);
    periodToDe_->setCalendarPopup(true);
    periodToDe_->setDisplayFormat("yyyy-MM-dd hh:mm");
    periodToDe_->setDate(QDate::currentDate());
    periodToDe_->setMaximumDate(QDate::currentDate());
    periodToDe_->setTime(QTime(23, 59));
    hb->addWidget(periodToDe_);

    connect(periodFromDe_, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(slotFromChanged(QDateTime)));

    connect(periodToDe_, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(slotToChanged(QDateTime)));

    // Add to grid
    grid_->addWidget(label_, row, 0);
    grid_->addWidget(modeCb_, row, 1);
    grid_->addWidget(holder_, row, 2);

    // Init
    modeCb_->setCurrentIndex(0);
    unitsCb_->setCurrentIndex(1); // minutes

    initInternal(option);
    Q_ASSERT(option_);

    // we need to call it to have a proper init!!
    modeChanged(0);
}

void NodeQueryPeriodOptionEdit::initInternal(NodeQueryOption* option) {
    initIsOn_ = true;

    option_   = static_cast<NodeQueryPeriodOption*>(option);
    Q_ASSERT(option_);
    Q_ASSERT(optionId_ == option_->name());
    if (option_->mode() == NodeQueryPeriodOption::LastPeriodMode) {
        modeCb_->setCurrentIndex(1);
        valueSpin_->setValue(option_->period());
        ViewerUtil::initComboBoxByData(option_->periodUnits(), unitsCb_);
    }
    else if (option_->mode() == NodeQueryPeriodOption::OlderPeriodMode) {
        modeCb_->setCurrentIndex(3);
        valueSpin_->setValue(option_->period());
        ViewerUtil::initComboBoxByData(option_->periodUnits(), unitsCb_);
    }
    else if (option_->mode() == NodeQueryPeriodOption::FixedPeriodMode) {
        modeCb_->setCurrentIndex(2);
        periodFromDe_->setDateTime(option_->fromDate());
        periodToDe_->setDateTime(option_->toDate());
    }
    else {
        modeCb_->setCurrentIndex(0);
    }

    initIsOn_ = false;
}

void NodeQueryPeriodOptionEdit::setVisible(bool) {
}

void NodeQueryPeriodOptionEdit::modeChanged(int) {
    int idx = modeCb_->currentIndex();
    if (idx < 0) {
        holder_->hide();
        updateOptions();
        return;
    }

    QString mode = modeCb_->itemData(idx).toString();
    if (mode == "last" || mode == "older") {
        holder_->show();
        valueSpin_->show();
        unitsCb_->show();
        periodFromDe_->hide();
        periodToDe_->hide();
    }
    else if (mode == "period") {
        holder_->show();
        valueSpin_->hide();
        unitsCb_->hide();
        periodFromDe_->show();
        periodToDe_->show();
    }
    else {
        holder_->hide();
    }

    updateOptions();
}

void NodeQueryPeriodOptionEdit::periodValueChanged(int) {
    updateOptions();
}

void NodeQueryPeriodOptionEdit::periodUnitsChanged(int) {
    updateOptions();
}

void NodeQueryPeriodOptionEdit::slotFromChanged(QDateTime) {
    updateOptions();
}

void NodeQueryPeriodOptionEdit::slotToChanged(QDateTime) {
    updateOptions();
}

void NodeQueryPeriodOptionEdit::updateOptions() {
    if (initIsOn_)
        return;

    if (option_) {
        int modeIdx = modeCb_->currentIndex();
        if (modeIdx < 0) {
            return;
        }
        QString mode = modeCb_->itemData(modeIdx).toString();

        // last/older period
        if (mode == "last" || mode == "older") {
            int val = valueSpin_->value();
            int idx = unitsCb_->currentIndex();
            QString units;
            if (idx > -1)
                units = unitsCb_->itemData(idx).toString();
            else
                val = -1;

            if (mode == "last") {
                option_->setLastPeriod(val, units);
            }
            else {
                option_->setOlderPeriod(val, units);
            }
        }

        // fixed period
        else if (mode == "period") {
            option_->setPeriod(periodFromDe_->dateTime(), periodToDe_->dateTime());
        }
        else {
            option_->clear();
        }

        Q_EMIT changed();
    }
}
