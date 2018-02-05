//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TextFilterWidget.hpp"

#include <QtGlobal>
#include <QCompleter>
#include <QMenu>
#include <QPalette>
#include <QTreeView>

TextFilterWidget::TextFilterWidget(QWidget *parent) :
    QWidget(parent),
    status_(EditStatus)
{
    setupUi(this);

    //Editor

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    le_->setPlaceholderText(tr(" Enter regexp to filter"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    le_->setClearButtonEnabled(true);
#endif

    oriColour_=QColor(le_->palette().color(QPalette::Base));
    redColour_=QColor(210,24,24);
    greenColour_=QColor(25,143,0);

    completer_=new QCompleter(this);
    completer_->setCompletionMode(QCompleter::UnfilteredPopupCompletion);

    QTreeView *view=new QTreeView(this);
    view->setFixedHeight(100);
    completerModel_=new TextFilterCompleterModel(this);
    completerModel_->setData(TextFilterHandler::Instance()->items());

    completer_->setModel(completerModel_);
    completer_->setPopup(view);
    le_->setCompleter(completer_);

    QIcon icon;
    icon.addPixmap(QPixmap(":/viewer/close_grey.svg"),QIcon::Normal);
    icon.addPixmap(QPixmap(":/viewer/close_red.svg"),QIcon::Active);
    closeTb_->setIcon(icon);
}

void TextFilterWidget::slotFilterEditor()
{

}

void TextFilterWidget::on_confTb__clicked()
{
    QMenu* menu=new QMenu(confTb_);

    QAction *manageAc=new QAction(menu);
    manageAc->setText("Manage text filters ...");
    //connect(ac,SIGNAL(triggered()),this,SLOT(slotFilterEditor()));
    menu->addAction(manageAc);

    QAction *sep=new QAction(menu);
    sep->setSeparator(true);
    menu->addAction(sep);

    QAction* acSavedTitle = new QAction(menu);
    acSavedTitle->setText(tr("Saved"));
    QFont f=acSavedTitle->font();
    f.setBold(true);
    acSavedTitle->setFont(f);
    menu->addAction(acSavedTitle);

    const std::vector<TextFilterItem>& items=TextFilterHandler::Instance()->items();
    for(std::size_t i=0 ; i < items.size(); i++)
    {
        QAction* ac=new QAction(this);
        ac->setText(QString::fromStdString(items[i].filter()));
        menu->addAction(ac);
    }

    if(QAction *ac=menu->exec(QCursor::pos()))
    {
        //int index=ac->data().toInt();
        //if(index >=0 && index < count())
        //{
            //setCurrentIndex(index);
        //}
    }

    menu->clear();
    menu->deleteLater();
}

void TextFilterWidget::on_runTb__clicked()
{
    QString t=le_->text();
    if(!t.isEmpty())
        Q_EMIT runRequested(t);
}

void TextFilterWidget::on_closeTb__clicked()
{
    hide();
}

void TextFilterWidget::on_le__textChanged()
{
    if(status_ != EditStatus)
        setStatus(EditStatus);
}

void TextFilterWidget::setEditFocus()
{
    le_->setFocus();
}

void TextFilterWidget::setStatus(FilterStatus status)
{
    status_=status;
    QColor col=oriColour_;
    QPalette p=le_->palette();
    switch(status_)
    {
    case EditStatus:
        col=oriColour_;
        break;
    case FoundStatus:
        col=greenColour_;
        break;
    case NotFoundStatus:
        col=redColour_;
        break;
    default:
        col=oriColour_;
        break;
    }

    p.setColor(QPalette::Text,col);
    le_->setPalette(p);
}

TextFilterCompleterModel::TextFilterCompleterModel(QObject *parent) :
          QAbstractItemModel(parent)
{}

TextFilterCompleterModel::~TextFilterCompleterModel()
{
}

void TextFilterCompleterModel::setData(const std::vector<TextFilterItem>& data)
{
    beginResetModel();
    data_=data;
    endResetModel();
}

#if 0
bool TextFilterCompleterModel::updateData(const std::vector<Zombie>& data)
{
    bool sameAs=false;
    if(hasData() && data.size() == data_.size())
    {
        sameAs=true;
        for(std::vector<Zombie>::const_iterator it=data.begin(); it != data.end(); it++)
        {
            bool hasIt=false;
            std::string p=(*it).path_to_task();
            for(std::vector<Zombie>::const_iterator itM=data_.begin(); itM != data_.end(); itM++)
            {
                if(p == (*itM).path_to_task())
                {
                    hasIt=true;
                    break;
                }
            }

            if(!hasIt)
            {
                sameAs=false;
                break;
            }
        }
    }

    if(sameAs)
    {
        data_=data;
        Q_EMIT dataChanged(index(0,0),index(data_.size()-1,columns_->count()));
        return false;
    }
    else
    {
        beginResetModel();
        data_=data;
        endResetModel();
        return true;
    }
}


void TextFilterCompleterModel::clearData()
{
    beginResetModel();
    data_.clear();
    endResetModel();
}
#endif


bool TextFilterCompleterModel::hasData() const
{
    return !data_.empty();
}

int TextFilterCompleterModel::columnCount( const QModelIndex& /*parent */) const
{
     return 2;
}

int TextFilterCompleterModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid())
    {
        return static_cast<int>(data_.size());
    }

    return 0;
}

Qt::ItemFlags TextFilterCompleterModel::flags ( const QModelIndex & index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TextFilterCompleterModel::data( const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData())
    {
        return QVariant();
    }

    int row=index.row();
    int col=index.column();
    if(row < 0 || row >= static_cast<int>(data_.size()))
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        if(col== 0)
            return QString::fromStdString(data_[row].name());
        else if(col == 1)
            return QString::fromStdString(data_[row].filter());
        else
            return QVariant();
    }
    else if(role == Qt::EditRole)
    {
        if(col == 0)
            return QString::fromStdString(data_[row].filter());
        else if(col == 1)
            return QString::fromStdString(data_[row].filter());
        else
            return QVariant();
    }
    else if(role == Qt::ForegroundRole)
    {
        if(col == 0)
            return QColor(150,150,150);
        else
            return QVariant();
    }

    return QVariant();
}

QVariant TextFilterCompleterModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if ( orient != Qt::Horizontal || (role != Qt::DisplayRole && role != Qt::UserRole ))
              return QAbstractItemModel::headerData( section, orient, role );

    if(role == Qt::DisplayRole)
        return "Name";
    else if(role == Qt::UserRole)
        return "Filter regexp";

    return QVariant();
}

QModelIndex TextFilterCompleterModel::index( int row, int column, const QModelIndex & parent ) const
{
    if(!hasData() || row < 0 || column < 0)
    {
        return QModelIndex();
    }

    //When parent is the root this index refers to a node or server
    if(!parent.isValid())
    {
        return createIndex(row,column);
    }

    return QModelIndex();

}

QModelIndex TextFilterCompleterModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

#if 0
Zombie TextFilterCompleterModel::indexToZombie(const QModelIndex& idx) const
{
    if(idx.isValid() && hasData())
    {
        int row=idx.row();
        if(row >= 0 || row < static_cast<int>(data_.size()))
            return data_[row];
    }
    return Zombie();
}
#endif
