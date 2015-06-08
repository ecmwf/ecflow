//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef PROPERTYEDITOR_INC_
#define PROPERTYEDITOR_INC_

#include <QMap>
#include <QWidget>

class QtTreePropertyBrowser;
class QtVariantEditorFactory;
class QtProperty;

class QtColorEditorFactory;

class VProperty;

#include <QtColorPropertyManager>
#include <QtVariantPropertyManager>

#include "qtpropertybrowser.h"

#include "ui_PropertyEditor.h"


//QT_QTPROPERTYBROWSER_EXPORT

class QtCustomColorPropertyManager : public QtAbstractPropertyManager
{
    Q_OBJECT
public:
    QtCustomColorPropertyManager(QObject *parent = 0);
    ~QtCustomColorPropertyManager();

    QColor value(const QtProperty *property) const;

public Q_SLOTS:
    void setValue(QtProperty *property, const QColor &val);
Q_SIGNALS:
    void valueChanged(QtProperty *property, const QColor &val);

protected:
    QString valueText(const QtProperty *property) const;
    QIcon valueIcon(const QtProperty *property) const;
    virtual void initializeProperty(QtProperty *property);
    virtual void uninitializeProperty(QtProperty *property);

private:
    QMap<const QtProperty *, QColor> vals_;
};

class QtCustomColorEditWidget;

class QtCustomColorEditorFactory : public QtAbstractEditorFactory<QtCustomColorPropertyManager>
{
    Q_OBJECT
public:
    QtCustomColorEditorFactory(QObject *parent = 0);
    ~QtCustomColorEditorFactory();

protected:
    void connectPropertyManager(QtCustomColorPropertyManager *manager);
    QWidget *createEditor(QtCustomColorPropertyManager *manager, QtProperty *property,
                QWidget *parent);
    void disconnectPropertyManager(QtCustomColorPropertyManager *manager);

private Q_SLOTS:
  	  void slotPropertyChanged(QtProperty *property,const QColor &value);
  	  void slotSetValue(const QColor &);
private:
    //Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QColor &))
    //Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
   //Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QColor &))
    QMap<QtProperty*,QList<QtCustomColorEditWidget*> > editors_;
    QMap<QtCustomColorEditWidget*, QtProperty*> props_;
};

class  QLabel;
class  QToolButton;

class QtCustomColorEditWidget : public QWidget {
    Q_OBJECT

public:
    QtCustomColorEditWidget(QWidget *parent);

    bool eventFilter(QObject *obj, QEvent *ev);

public Q_SLOTS:
    void setValue(const QColor &value);

Q_SIGNALS:
    void valueChanged(const QColor &value);

protected:
    void paintEvent(QPaintEvent *);

private Q_SLOTS:
    void buttonClicked();

private:
    QColor m_color;
    QLabel *m_pixmapLabel;
    QLabel *m_label;
    QToolButton *m_button;
};

class VariantManager : public QtVariantPropertyManager
{
    Q_OBJECT

public:
    VariantManager(QObject *parent = 0);
    ~VariantManager();

    virtual QVariant value(const QtProperty *property) const;
    virtual int valueType(int propertyType) const;
    virtual bool isPropertyTypeSupported(int propertyType) const;

    QString valueText(const QtProperty *property) const;

public Q_SLOTS:
    virtual void setValue(QtProperty *property, const QVariant &val);

protected:
    virtual void initializeProperty(QtProperty *property);
    virtual void uninitializeProperty(QtProperty *property);

private Q_SLOTS:
    void slotValueChanged(QtProperty *property, const QVariant &value);
    void slotPropertyDestroyed(QtProperty *property);

private:
    QMap<const QtProperty *, QVariant> data_;
    QList<QtAbstractPropertyManager*> managers_;
};

class VariantEditorFactory : public QtVariantEditorFactory
{
public:
    VariantEditorFactory(QObject *parent = 0);
    ~VariantEditorFactory();

protected:
    void connectPropertyManager(QtVariantPropertyManager *manager);
    QWidget *createEditor(QtVariantPropertyManager *manager, QtProperty *property,QWidget *parent);
    void disconnectPropertyManager(QtVariantPropertyManager *manager);

private:
    QtCustomColorEditorFactory* colorEditorFactory_;
    QMap<QtAbstractEditorFactoryBase *, int> factoryToType_;
    QMap<int, QtAbstractEditorFactoryBase *> typeToFactory_;
};


class PropertyEditor : public QWidget, protected Ui::PropertyEditor
{
public:
    PropertyEditor(QWidget *parent=0);
    ~PropertyEditor();

    void edit(VProperty*);
    void editAccepted();

private:
    void build();
    void addItem(VProperty* vProp,QtProperty* parentProp);
    void syncToConfig(QtProperty*);

    QtVariantEditorFactory* factory_;
    QtTreePropertyBrowser* browser_;

    //This is a map between the properties in the editor and in the config
    QMap<QtProperty*,VProperty*> confMap_;

    VProperty* group_;

};

#endif

