#include "ComboMulti.hpp"

#include <QApplication>
#include <QAbstractItemView>
#include <QDebug>
#include <QCheckBox>
#include <QItemDelegate>
#include <QPainter>
#include <QStylePainter>

//==========================================================
//
// ComboMulti
//
//==========================================================

ComboMulti::ComboMulti(QWidget *widget) :
    QComboBox(widget),
    dpyText_("")
{
    setSizeAdjustPolicy(QComboBox::AdjustToContents);

    ComboMultiDelegate* del=new ComboMultiDelegate(this);

    connect(del,SIGNAL(itemChecked()),
            this,SLOT(slotChecked()));

         // set delegate items view
    view()->setItemDelegate(del);
    //view()->setStyleSheet("  padding: 15px; ");
    // Enable editing on items view
    view()->setEditTriggers(QAbstractItemView::CurrentChanged);

    // set "CheckBoxList::eventFilter" as event filter for items view
    view()->viewport()->installEventFilter(this);

    // it just cool to have it as defualt ;)
    view()->setAlternatingRowColors(true);
}


ComboMulti::~ComboMulti()
{

}

bool ComboMulti::eventFilter(QObject *object, QEvent *event)
{
    // don't close items view after we release the mouse button
    // by simple eating MouseButtonRelease in viewport of items view
    if(event->type() == QEvent::MouseButtonRelease && object==view()->viewport())
    {
        return true;
    }
    return QComboBox::eventFilter(object,event);
}

void ComboMulti::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    // draw the combobox frame, focusrect and selected etc.
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    // if no display text been set , use "..." as default
    if(dpyText_.isEmpty())
        opt.currentText = "ANY";
    else
    {
        qDebug() << "paint" << dpyText_;
        opt.currentText = dpyText_;
    }
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);

}

void ComboMulti::slotChecked()
{
    QString s;
    selection_.clear();

    int cnt=0;

    for(int i=0; i < model()->rowCount(); i++)
    {
        if(model()->data(model()->index(i,0),Qt::UserRole).toBool())
        {
            selection_ << model()->data(model()->index(i,0),Qt::DisplayRole).toString();
        }
    }

    if(selection_.count() == 0)
        s="";
    else
    	s=selection_.join(", ");

    setDisplayText(s);

    qDebug() << "text" << s;

    update();

    Q_EMIT selectionChanged();
}

void ComboMulti::clearSelection()
{
	for(int i=0; i < count(); i++)
	{
	    setItemData(i,false,Qt::UserRole);
	}

	slotChecked();
}

void ComboMulti::setDisplayText(QString text)
{
    dpyText_ = text;
}

QString ComboMulti::displayText() const
{
    return dpyText_;
}


//==========================================================
//
// ComboMultiDelegate
//
//==========================================================

ComboMultiDelegate::ComboMultiDelegate(QObject *parent)
   : QItemDelegate(parent)
{
}

void ComboMultiDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    //Get item data
    bool value = index.data(Qt::UserRole).toBool();
    QString text = index.data(Qt::DisplayRole).toString();

    // fill style options with item data
    const QStyle *style = QApplication::style();
    QStyleOptionButton opt;
    opt.state |= value ? QStyle::State_On : QStyle::State_Off;
    opt.state |= QStyle::State_Enabled;
    opt.text = text;
    opt.rect = option.rect;

    style->drawControl(QStyle::CE_CheckBox,&opt,painter);
}

QWidget* ComboMultiDelegate::createEditor(QWidget *parent,
         const QStyleOptionViewItem & option ,
         const QModelIndex & index ) const
{
     QCheckBox *editor = new QCheckBox(parent);
     return editor;
}

void ComboMultiDelegate::setEditorData(QWidget *editor,
                                         const QModelIndex &index) const
{
	//set editor data
	QCheckBox *myEditor = static_cast<QCheckBox*>(editor);
	myEditor->setText(index.data(Qt::DisplayRole).toString());
	myEditor->setChecked(index.data(Qt::UserRole).toBool());

	connect(myEditor,SIGNAL(stateChanged(int)),
                 this,SLOT(slotEdited(int)));

}

void ComboMultiDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
             const QModelIndex &index) const
{
	//get the value from the editor (CheckBox)
	QCheckBox *myEditor = static_cast<QCheckBox*>(editor);
    bool value = myEditor->isChecked();

    //set model data
    QMap<int,QVariant> data;
    data.insert(Qt::DisplayRole,myEditor->text());
    data.insert(Qt::UserRole,value);
    model->setItemData(index,data);

    qDebug() << "EDIT" << myEditor->text() << value;

    Q_EMIT itemChecked();

}

void ComboMultiDelegate::updateEditorGeometry(QWidget *editor,
         const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    editor->setGeometry(option.rect);
}

void ComboMultiDelegate::slotEdited(int)
{
    QCheckBox* cb = static_cast<QCheckBox*>(sender());
    if(cb)
    {
      Q_EMIT commitData(cb);
    }
}
