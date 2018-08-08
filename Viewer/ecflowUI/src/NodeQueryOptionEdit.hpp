//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef NODEQUERYOPTIONEDIT_HPP
#define NODEQUERYOPTIONEDIT_HPP

#include <QObject>
#include <QDateTime>

class CustomListWidget;
class NodeQuery;
class NodeQueryDef;
class NodeQueryListModel;
class NodeQueryOption;
class NodeQueryStringOption;
class NodeQueryListOption;
class NodeQueryComboOption;
class StringMatchCombo;
class NodeQueryPeriodOption;

class QComboBox;
class QDateTimeEdit;
class QLabel;
class QGridLayout;
class QLineEdit;
class QSpinBox;
class QToolButton;
class QWidget;

class NodeQueryOptionEdit : public QObject
{
Q_OBJECT
public:
    NodeQueryOptionEdit(QString optionId,QGridLayout* grid,QWidget *parent);

    void init(NodeQuery*);
    QString optionId() const {return optionId_;}
    virtual void setVisible(bool)=0;
    virtual void clear() {}

Q_SIGNALS:
    void changed();

protected:
    virtual void init(NodeQueryOption*)=0;

    QString optionId_;
    bool initIsOn_;
    QWidget* parent_;
    QGridLayout* grid_;
};

class NodeQueryStringOptionEdit : public  NodeQueryOptionEdit
{
Q_OBJECT
public:
    NodeQueryStringOptionEdit(NodeQueryOption* option,QGridLayout* grid,QWidget* parent, bool sameRow);
    void setVisible(bool) override;
    void clear() override;

protected Q_SLOTS:
    void slotEdited(QString val);
    void slotMatchChanged(int val);

protected:
    void init(NodeQueryOption*) override;

    QLabel* label_;
    StringMatchCombo* matchCb_;
    QLineEdit *le_;
    NodeQueryStringOption* option_;
};

class NodeQueryListOptionEdit : public  NodeQueryOptionEdit
{
Q_OBJECT
public:
    NodeQueryListOptionEdit(NodeQueryOption* option,CustomListWidget* cl,QToolButton*,QWidget*);
    void setVisible(bool) override {}
    void clear() override;

protected Q_SLOTS:
    void slotListChanged();

protected:
    void init(NodeQueryOption*) override;

    CustomListWidget* list_;
    QToolButton *resetTb_;
    NodeQueryListOption* option_;
};

class NodeQueryComboOptionEdit : public  NodeQueryOptionEdit
{
Q_OBJECT
public:
    NodeQueryComboOptionEdit(NodeQueryOption* option,QGridLayout* grid,QWidget*);
    void setVisible(bool) override;

protected Q_SLOTS:
    void slotCbChanged(int);

protected:
    void init(NodeQueryOption*) override;

    QLabel* label_;
    QComboBox* cb_;
    NodeQueryComboOption* option_;
};

class NodeQueryPeriodOptionEdit : public NodeQueryOptionEdit
{
Q_OBJECT
public:
    NodeQueryPeriodOptionEdit(NodeQueryOption* option,QGridLayout* grid,QWidget *parent);
    void setVisible(bool) override;

protected:
    void init(NodeQueryOption*) override;

protected Q_SLOTS:
    void updateOptions();
    void modeChanged(int);
    void lastValueChanged(int);
    void lastUnitsChanged(int);
    void slotFromChanged(QDateTime);
    void slotToChanged(QDateTime);

private:
    NodeQueryPeriodOption* option_;
    QLabel* label_;
    QComboBox* modeCb_;
    QSpinBox* lastValueSpin_;
    QComboBox* lastUnitsCb_;
    QDateTimeEdit* periodFromDe_;
    QDateTimeEdit* periodToDe_;
    QWidget* holder_;
};



#endif // NODEQUERYOPTIONEDIT_HPP

