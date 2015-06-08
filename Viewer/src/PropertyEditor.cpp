//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "PropertyEditor.hpp"

#include <QDebug>
#include <QVBoxLayout>

#include <QLabel>
#include <QToolButton>
#include <QColorDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QStyleOption>
#include <QPainter>

#include <QtColorEditorFactory>
#include <QtTreePropertyBrowser>
#include <QtVariantEditorFactory>
#include <QtGroupPropertyManager>

#include "VConfig.hpp"
#include "VProperty.hpp"

#include "qtpropertybrowserutils_p.h"


QtCustomColorPropertyManager::QtCustomColorPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)

{
	  /*connect(this, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
	                this, SLOT(slotValueChanged(QtProperty *, const QVariant &)));
	    connect(this, SIGNAL(propertyDestroyed(QtProperty *)),
	                this, SLOT(slotPropertyDestroyed(QtProperty *)));*/


}

QtCustomColorPropertyManager::~QtCustomColorPropertyManager()
{
    clear();
}

QColor QtCustomColorPropertyManager::value(const QtProperty *property) const
{
	return vals_.value(property, QColor());
}

QString QtCustomColorPropertyManager::valueText(const QtProperty *property) const
{
	QMap<const QtProperty *, QColor>::const_iterator it = vals_.constFind(property);
    if (it == vals_.constEnd())
        return QString();

    QColor c=it.value();

    return QString("[%1, %2, %3]").arg(c.red()).arg(c.green()).arg(c.blue());
}

QIcon QtCustomColorPropertyManager::valueIcon(const QtProperty *property) const
{
	QMap<const QtProperty *, QColor>::const_iterator it = vals_.constFind(property);
    if (it == vals_.constEnd())
        return QIcon();
    return QtPropertyBrowserUtils::brushValueIcon(QBrush(it.value()));
}


void QtCustomColorPropertyManager::setValue(QtProperty *property, const QColor &val)
{
	const QMap<const QtProperty *, QColor>::iterator it = vals_.find(property);
    if (it == vals_.constEnd())
        return;

    if (it.value() == val)
        return;

    it.value() = val;

    Q_EMIT propertyChanged(property);
    Q_EMIT valueChanged(property, val);
}


void QtCustomColorPropertyManager::initializeProperty(QtProperty *property)
{
	 QColor val;
	 vals_[property] = val;

}

void QtCustomColorPropertyManager::uninitializeProperty(QtProperty *property)
{
	 vals_.remove(property);
}


// QtColorEditWidget



QtCustomColorEditWidget::QtCustomColorEditWidget(QWidget *parent) :
    QWidget(parent),
    m_pixmapLabel(new QLabel),
    m_label(new QLabel),
    m_button(new QToolButton)
{
    QHBoxLayout *lt = new QHBoxLayout(this);
    //setupTreeViewEditorMargin(lt);

    lt->setContentsMargins(4, 0, 0, 0);

    lt->setSpacing(0);
    lt->addWidget(m_pixmapLabel);
    lt->addWidget(m_label);
    lt->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));

    m_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    m_button->setFixedWidth(20);
    setFocusProxy(m_button);
    setFocusPolicy(m_button->focusPolicy());
    m_button->setText(tr("..."));
    m_button->installEventFilter(this);
    connect(m_button, SIGNAL(clicked()), this, SLOT(buttonClicked()));
    lt->addWidget(m_button);
    m_pixmapLabel->setPixmap(QtPropertyBrowserUtils::brushValuePixmap(QBrush(m_color)));
    m_label->setText(QtPropertyBrowserUtils::colorValueText(m_color));
}

void QtCustomColorEditWidget::setValue(const QColor &c)
{
    if (m_color != c) {
        m_color = c;
        m_pixmapLabel->setPixmap(QtPropertyBrowserUtils::brushValuePixmap(QBrush(c)));
        m_label->setText(QtPropertyBrowserUtils::colorValueText(c));
    }
}

void QtCustomColorEditWidget::buttonClicked()
{
    bool ok = false;
    QRgb oldRgba = m_color.rgba();
    QRgb newRgba = QColorDialog::getRgba(oldRgba, &ok, this);
    if (ok && newRgba != oldRgba) {
        setValue(QColor::fromRgba(newRgba));
        Q_EMIT valueChanged(m_color);
    }
}

bool QtCustomColorEditWidget::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_button) {
        switch (ev->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease: { // Prevent the QToolButton from handling Enter/Escape meant control the delegate
            switch (static_cast<const QKeyEvent*>(ev)->key()) {
            case Qt::Key_Escape:
            case Qt::Key_Enter:
            case Qt::Key_Return:
                ev->ignore();
                return true;
            default:
                break;
            }
        }
            break;
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void QtCustomColorEditWidget::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

// QtColorEditorFactoryPrivate

/*class QtCustomColorEditorFactoryPrivate : public EditorFactoryPrivate<QtColorEditWidget>
{
    QtColorEditorFactory *q_ptr;
    Q_DECLARE_PUBLIC(QtColorEditorFactory)
public:

    void slotPropertyChanged(QtProperty *property, const QColor &value);
    void slotSetValue(const QColor &value);
};

void QtColorEditorFactoryPrivate::slotPropertyChanged(QtProperty *property,
                const QColor &value)
{
    const PropertyToEditorListMap::iterator it = m_createdEditors.find(property);
    if (it == m_createdEditors.end())
        return;
    QListIterator<QtColorEditWidget *> itEditor(it.value());

    while (itEditor.hasNext())
        itEditor.next()->setValue(value);
}

void QtColorEditorFactoryPrivate::slotSetValue(const QColor &value)
{
    QObject *object = q_ptr->sender();
    const EditorToPropertyMap::ConstIterator ecend = m_editorToProperty.constEnd();
    for (EditorToPropertyMap::ConstIterator itEditor = m_editorToProperty.constBegin(); itEditor != ecend; ++itEditor)
        if (itEditor.key() == object) {
            QtProperty *property = itEditor.value();
            QtColorPropertyManager *manager = q_ptr->propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
}
*/
/*!
    \class QtColorEditorFactory

    \brief The QtColorEditorFactory class provides color editing  for
    properties created by QtColorPropertyManager objects.

    \sa QtAbstractEditorFactory, QtColorPropertyManager
*/

/*!
    Creates a factory with the given \a parent.
*/
QtCustomColorEditorFactory::QtCustomColorEditorFactory(QObject *parent) :
    QtAbstractEditorFactory<QtCustomColorPropertyManager>(parent)
{

}



/*!
    Destroys this factory, and all the widgets it has created.
*/
QtCustomColorEditorFactory::~QtCustomColorEditorFactory()
{

	// qDeleteAll(d_ptr->m_editorToProperty.keys());
   // delete d_ptr;
}

void QtCustomColorEditorFactory::connectPropertyManager(QtCustomColorPropertyManager *manager)
{
    connect(manager, SIGNAL(valueChanged(QtProperty*,QColor)),
            this, SLOT(slotPropertyChanged(QtProperty*,QColor)));
}

QWidget *QtCustomColorEditorFactory::createEditor(QtCustomColorPropertyManager *manager,
        QtProperty *property, QWidget *parent)
{
	QtCustomColorEditWidget *editor=new QtCustomColorEditWidget(parent);
	editor->setValue(manager->value(property));

	connect(editor, SIGNAL(valueChanged(QColor)), this, SLOT(slotSetValue(QColor)));
	connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));


	/*QtCustomColorEditWidget *editor = new QtCustomColorEditWidget(this);  //createEditor(property, parent);
    editor->setValue(manager->value(property));
    connect(editor, SIGNAL(valueChanged(QColor)), this, SLOT(slotSetValue(QColor)));
    connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));
    return editor;*/
}

void QtCustomColorEditorFactory::slotPropertyChanged(QtProperty *property,
                const QColor &value)
{
	QMap<QtProperty*,QList<QtCustomColorEditWidget*> >::iterator it = editors_.find(property);
    if (it == editors_.end())
        return;

    Q_FOREACH(QtCustomColorEditWidget *w,it.value())
    {
        w->setValue(value);
    }
}

void QtCustomColorEditorFactory::slotSetValue(const QColor &value)
{
    QObject *object = QObject::sender();

    const QMap<QtCustomColorEditWidget*, QtProperty*>::ConstIterator ecend = props_.constEnd();
    for (QMap<QtCustomColorEditWidget*, QtProperty*>::ConstIterator itEditor = props_.constBegin(); itEditor != ecend; ++itEditor)
    {
    	if (itEditor.key() == object)
    	{
            QtProperty *property = itEditor.value();
            QtCustomColorPropertyManager *manager = propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
    }
}


/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void QtCustomColorEditorFactory::disconnectPropertyManager(QtCustomColorPropertyManager *manager)
{
    disconnect(manager, SIGNAL(valueChanged(QtProperty*,QColor)),
    	 this, SLOT(slotPropertyChanged(QtProperty*,QColor)));
}

/*void QtCustomColorEditorFactory::slotSetValue(QColor color)
{

}*/


























/*class QtCustomColorEditorFactory : public QtAbstractEditorFactory<QtCustomColorPropertyManager>
{
public:
    QtCustomColorEditorFactory(QObject *parent = 0) : QtCustomColorEditorFactory(parent) {};
    ~QtCustomColorEditorFactory();
};*/


VariantManager::VariantManager(QObject *parent)
    : QtVariantPropertyManager(parent)
{
    /*connect(this, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
                this, SLOT(slotValueChanged(QtProperty *, const QVariant &)));
    connect(this, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));*/

    //managers_ << new QtCustomColorPropertyManager(this);
}

VariantManager::~VariantManager()
{

}

void VariantManager::slotValueChanged(QtProperty *property, const QVariant &value)
{
   /* if (xToProperty.contains(property)) {
        QtProperty *pointProperty = xToProperty[property];
        QVariant v = this->value(pointProperty);
        QPointF p = v.value<QPointF>();
        p.setX(value.value<double>());
        setValue(pointProperty, p);
    } else if (yToProperty.contains(property)) {
        QtProperty *pointProperty = yToProperty[property];
        QVariant v = this->value(pointProperty);
        QPointF p = v.value<QPointF>();
        p.setY(value.value<double>());
        setValue(pointProperty, p);
    }*/
}

void VariantManager::slotPropertyDestroyed(QtProperty *property)
{
    /*if (xToProperty.contains(property)) {
        QtProperty *pointProperty = xToProperty[property];
        propertyToData[pointProperty].x = 0;
        xToProperty.remove(property);
    } else if (yToProperty.contains(property)) {
        QtProperty *pointProperty = yToProperty[property];
        propertyToData[pointProperty].y = 0;
        yToProperty.remove(property);
    }*/
}

bool VariantManager::isPropertyTypeSupported(int propertyType) const
{
    //if (propertyType == QVariant::PointF)
    //    return true;
    return QtVariantPropertyManager::isPropertyTypeSupported(propertyType);
}

int VariantManager::valueType(int propertyType) const
{
    //if (propertyType == QVariant::PointF)
    //    return QVariant::PointF;
    return QtVariantPropertyManager::valueType(propertyType);
}

QVariant VariantManager::value(const QtProperty *property) const
{
   /* if (data_.contains(property))
        return data_[property];*/
    return QtVariantPropertyManager::value(property);
}

QString VariantManager::valueText(const QtProperty *property) const
{
   /* if (data_.contains(property))
    {
        QVariant v = data_[property];
        QColor c= v.value<QColor>();
        return c.name();
    }*/

    return QtVariantPropertyManager::valueText(property);
}

void VariantManager::setValue(QtProperty *property, const QVariant &val)
{
   /* if (data_.contains(property))
    {
        if (val.type() != QVariant::Color)
            return;
        QColor c = val.value<QColor>();
        //QVariant d = data_[property];

        data_[property] = c;

        Q_EMIT propertyChanged(property);
        Q_EMIT valueChanged(property, c);
        return;
    }*/
    QtVariantPropertyManager::setValue(property, val);
}

void VariantManager::initializeProperty(QtProperty *property)
{
   /* if (propertyType(property) == QVariant::Color)
    {
        QColor c;
        data_[property] = c;
    }*/
    QtVariantPropertyManager::initializeProperty(property);
}

void VariantManager::uninitializeProperty(QtProperty *property)
{
   /* if (data_.contains(property)) {
        data_.remove(property);
    }*/
    QtVariantPropertyManager::uninitializeProperty(property);
}


VariantEditorFactory::VariantEditorFactory(QObject *parent)
    : QtVariantEditorFactory(parent)
{
    /*colorEditorFactory_ = new QtCustomColorEditorFactory(this);
    factoryToType_[colorEditorFactory_] = QVariant::Color;
    typeToFactory_[QVariant::Color] = colorEditorFactory_;*/
}

/*!
    Destroys this factory, and all the widgets it has created.
*/
VariantEditorFactory::~VariantEditorFactory()
{

}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void VariantEditorFactory::connectPropertyManager(QtVariantPropertyManager *manager)
{
	/*QList<QtCustomColorPropertyManager *> colorPropertyManagers = qFindChildren<QtCustomColorPropertyManager *>(manager);
    QListIterator<QtCustomColorPropertyManager *> itColor(colorPropertyManagers);
    while (itColor.hasNext()) {
        QtCustomColorPropertyManager *manager = itColor.next();
        colorEditorFactory_->addPropertyManager(manager);

    }*/

    QtVariantEditorFactory::connectPropertyManager(manager);
}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/

QWidget* VariantEditorFactory::createEditor(QtVariantPropertyManager *manager, QtProperty *property, QWidget *parent)
{
    return QtVariantEditorFactory::createEditor(manager,property,parent);

}

/*!
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
void VariantEditorFactory::disconnectPropertyManager(QtVariantPropertyManager *manager)
{
    /*QList<QtCustomColorPropertyManager *> colorPropertyManagers = qFindChildren<QtCustomColorPropertyManager *>(manager);
    Q_FOREACH(QtCustomColorPropertyManager *m,colorPropertyManagers)
    {
        //QtCustomColorPropertyManager *manager = itColor.next();
        colorEditorFactory_->removePropertyManager(m);
    }*/

    QtVariantEditorFactory::disconnectPropertyManager(manager);
}


///////////////////////////////////////////////////////////////////////////////////////////


PropertyEditor::PropertyEditor(QWidget* parent) //, group_(vGroup)
{
    setupUi(this);


	/*QVBoxLayout *vb=new QVBoxLayout();
    setLayout(vb);

    browser_=new QtTreePropertyBrowser(this);
    //factory_=new QtVariantEditorFactory(this);
    factory_=new VariantEditorFactory(this);

    browser_->setResizeMode(QtTreePropertyBrowser::Interactive);

    vb->addWidget(browser_);

    build();*/
}

PropertyEditor::~PropertyEditor()
{
}

void PropertyEditor::edit(VProperty * vGroup)
{
	 group_=vGroup;

	 headerLabel_->setText(group_->labelText());
}


//Build the property tree from the the definitions
void PropertyEditor::build()
{
    //Loop over the property groups

	//const std::vector<VProperty*>& groups=VConfig::instance()->groups();

	//for(std::vector<VProperty*>::const_iterator it=groups.begin();it != groups.end(); it++)
   // {
        //VProperty *vGroup=*it;

		VProperty *vGroup=group_;

        //We only handle editable groups
        if(!vGroup->editable())
            //continue;
        	return;

        //Create editor group manager
        //QtGroupPropertyManager *groupManager = new QtGroupPropertyManager;

        //Create an editor property group
       // QtProperty* groupProp = groupManager->addProperty(vGroup->labelText());

        //Register it in the property map
        //confMap_[groupProp]=vGroup;

        //Loop over the children of the group
        Q_FOREACH(VProperty* vProp,vGroup->children())
        {
            //Add each item to the the editor
            //addItem(vProp,groupProp);
        	addItem(vProp,NULL);
        }

        //Add editor group to browser
        //browser_->addProperty(groupProp);
  //  }

	browser_->show();
}

void PropertyEditor::addItem(VProperty* vProp,QtProperty* parentProp)
{
    //We only handle editable properties
    if(!vProp->editable())
        return;

    //Is it a group?
    if(vProp->hasChildren())
    {
        //Create an editor group manager
        QtGroupPropertyManager *groupManager = new QtGroupPropertyManager;

        //Create an editor property group
        QtProperty* groupProp = groupManager->addProperty(vProp->labelText());

        //Register it in the property map
        confMap_[groupProp]=vProp;

        //Add theid editor property to its parent
        if(parentProp)
        	parentProp->addSubProperty(groupProp);
        else
        	browser_->addProperty(groupProp);

        //Loop over the children of the group
        Q_FOREACH(VProperty* chProp,vProp->children())
        {
             //Add each item to the the editor
            addItem(chProp,groupProp);
        }
    }
    else
    {
    	QVariant::Type vType=vProp->defaultValue().type();
    	qDebug() << vProp->labelText() << vType << QVariant();


        //We cannot handle these values.
        if(vType == QVariant::Invalid)
        	return;

        if(vType != QVariant::Color)
        {
        	//Create manager
        	//QtVariantPropertyManager *variantManager=new QtVariantPropertyManager;
        	QtVariantPropertyManager *variantManager=new VariantManager;


        	QtVariantProperty* prop =variantManager->addProperty(vType,vProp->labelText());

        	//Register it in the property map
        	confMap_[prop]=vProp;

        	prop->setToolTip(vProp->toolTip());
        	prop->setValue(vProp->value());

        	//Add to group
        	if(parentProp)
        		parentProp->addSubProperty(prop);
        	else
        		browser_->addProperty(prop);

        	//Set factory
        	browser_->setFactoryForManager(variantManager, factory_);
        }
        else
        {
        	QtCustomColorPropertyManager *variantManager=new QtCustomColorPropertyManager;

        	QtProperty* prop =variantManager->addProperty(vProp->labelText());

        	prop->setToolTip(vProp->toolTip());
        	variantManager->setValue(prop,vProp->value().value<QColor>());

        	//Add to group
        	if(parentProp)
        	     parentProp->addSubProperty(prop);
        	else
        	     browser_->addProperty(prop);

        	QtCustomColorEditorFactory* f=new QtCustomColorEditorFactory(this);
        	browser_->setFactoryForManager(variantManager, f);
        }

    }
}

void PropertyEditor::editAccepted()
{
    //Loop over the top level properties (groups) in the browser
    Q_FOREACH(QtProperty* gp, browser_->properties())
    {
        //Sync the changes to VConfig
        syncToConfig(gp);
    }
}

void PropertyEditor::syncToConfig(QtProperty *prop)
{
    qDebug() << " prop:" << prop->propertyName() << prop->propertyName() <<  prop->isModified();

    //If the property value has been changed.
    if(prop->hasValue()/*prop->isModified()*/)
    {
        //We lookup the corresponding VProperty in in VConfig
        //and set its current value.
        QMap<QtProperty*,VProperty*>::iterator it = confMap_.find(prop);
        if(it != confMap_.end())
        {
            if(QtVariantProperty *vp=static_cast<QtVariantProperty*>(prop))
            {
                qDebug() << "   value:  " << vp->value();
                it.value()->setValue(vp->value());
            }
        }
    }

    //Go through all the children.
    Q_FOREACH(QtProperty* sp,prop->subProperties())
    {
        syncToConfig(sp);
    }
}
