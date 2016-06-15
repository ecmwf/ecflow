//============================================================================
// Copyright 2016 ECMWF.
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

class CustomListWidget;
class NodeQuery;
class NodeQueryDef;
class NodeQueryListModel;
class NodeQueryStringOption;
class NodeQueryListOption;
class StringMatchCombo;

class QLabel;
class QGridLayout;
class QLineEdit;
class QToolButton;
class QWidget;

class NodeQueryOptionEdit : public QObject
{
Q_OBJECT
public:
    NodeQueryOptionEdit(QString optionId,QGridLayout* grid,QWidget *parent);

    virtual void init(NodeQuery*)=0;
    QString optionId() const {return optionId_;}
    virtual void setVisible(bool)=0;

Q_SIGNALS:
    void changed();

protected:
    QString optionId_;
    bool initIsOn_;
    QWidget* parent_;
    QGridLayout* grid_;
};

class NodeQueryStringOptionEdit : public  NodeQueryOptionEdit
{
Q_OBJECT
public:
    NodeQueryStringOptionEdit(QString optionId,QGridLayout* grid,QWidget* parent);
    void init(NodeQuery*);
    void setVisible(bool);

protected Q_SLOTS:
    void slotEdited(QString val);
    void slotMatchChanged(int val);

protected:
    QLabel* label_;
    StringMatchCombo* matchCb_;
    QLineEdit *le_;
    NodeQueryStringOption* option_;
};

class NodeQueryListOptionEdit : public  NodeQueryOptionEdit
{
Q_OBJECT
public:
    NodeQueryListOptionEdit(QString optionId,CustomListWidget* cl,QToolButton*,QWidget*);
    void init(NodeQuery*);
    void setVisible(bool) {}

protected Q_SLOTS:
    void slotListChanged();

protected:
    CustomListWidget* list_;
    QToolButton *resetTb_;
    NodeQueryListOption* option_;
};

#endif // NODEQUERYOPTIONEDIT_HPP

