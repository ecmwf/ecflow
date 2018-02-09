//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TextFilterWidget.hpp"
#include "TextFilterHandlerDialog.hpp"

#include <QtGlobal>
#include <QCompleter>
#include <QLabel>
#include <QLinearGradient>
#include <QMenu>
#include <QPalette>
#include <QPainter>
#include <QStyleOption>
#include <QTreeView>
#include <QWidgetAction>

#include "TextFormat.hpp"
#include "ViewerUtil.hpp"

TextFilterWidget::TextFilterWidget(QWidget *parent) :
    QWidget(parent),
    status_(EditStatus),
    statusTb_(0),
    optionTb_(0)
{
    setupUi(this);

    setProperty("textFilter","1");

    //match
    matchCb_->addItem(tr("matching"),0);
    matchCb_->addItem(tr("unmatching"),1);

    //Editor
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    le_->setPlaceholderText(tr(" Regexp (grep)"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    le_->setClearButtonEnabled(true);
#endif

    oriBrush_=QBrush(le_->palette().brush(QPalette::Base));
    redBrush_=ViewerUtil::lineEditRedBg();
    greenBrush_=ViewerUtil::lineEditGreenBg();

    completer_=new QCompleter(this);
    //completer_->setCompletionMode(QCompleter::UnfilteredPopupCompletion);

    //QTreeView *view=new QTreeView(this);
    //view->setFixedHeight(100);
    //view->setRootIsDecorated(false);

    //completerModel_=new TextFilterCompleterModel(this);
    //completerModel_->setData(TextFilterHandler::Instance()->items());

    //completer_->setModel(completerModel_);
    //completer_->setPopup(view);
    le_->setCompleter(completer_);

    QIcon icon;
    icon.addPixmap(QPixmap(":/viewer/close_grey.svg"),QIcon::Normal);
    icon.addPixmap(QPixmap(":/viewer/close_red.svg"),QIcon::Active);
    closeTb_->setIcon(icon);
}

bool TextFilterWidget::isActive() const
{
    return !le_->text().isEmpty();
}

bool TextFilterWidget::isCaseSensitive() const
{
    return caseCb_->isChecked();
}

bool TextFilterWidget::isMatched() const
{
    return matchCb_->currentIndex() == 0;
}

QString TextFilterWidget::filterText() const
{
    return le_->text();
}

void TextFilterWidget::init(const TextFilterItem& item)
{
    matchCb_->setCurrentIndex(item.matched()?0:1);
    caseCb_->setChecked(item.caseSensitive());
    le_->setText(QString::fromStdString(item.filter()));
}

void TextFilterWidget::setExternalButtons(QToolButton* statusTb,QToolButton* optionTb)
{
    Q_ASSERT(statusTb);
    Q_ASSERT(optionTb);

    statusTb_=statusTb;
    optionTb_=optionTb;

    connect(optionTb_,SIGNAL(clicked()),
            this,SLOT(slotOptionTb()));
}

void TextFilterWidget::refreshCompleter()
{
    //completerModel_->setData(TextFilterHandler::Instance()->items());
}

void TextFilterWidget::slotFilterEditor()
{

}

void TextFilterWidget::buildMenu(QToolButton *tb)
{
    QMenu* menu=new QMenu(tb);

    QAction *manageAc=new QAction(menu);
    manageAc->setText(tr("Manage filters ..."));
    manageAc->setIcon(QPixmap(":/viewer/configure.svg"));
    menu->addAction(manageAc);

    QAction *saveAc=0;
    if(isActive() && isVisible())
    {
        saveAc=new QAction(menu);
        saveAc->setText(tr("Save filter"));
        saveAc->setIcon(QPixmap(":/viewer/filesaveas.svg"));
        menu->addAction(saveAc);
    }

    QAction *clearAc=new QAction(menu);
    clearAc->setText(tr("Clear filter"));
    if(!isActive()) clearAc->setEnabled(false);
    menu->addAction(clearAc);

    QAction *sep=new QAction(menu);
    sep->setSeparator(true);
    menu->addAction(sep);

    addMenuSection(menu,TextFilterHandler::Instance()->items(),tr("Saved"),"s");
    addMenuSection(menu,TextFilterHandler::Instance()->latestItems(),tr("Recent"),"r");

    if(QAction *ac=menu->exec(QCursor::pos()))
    {      
        if(ac == manageAc)
        {
            TextFilterHandlerDialog diag;
            diag.exec();
            refreshCompleter();
        }
        else if(ac == saveAc)
        {
            std::string filter=filterText().toStdString();
            bool matchMode=isMatched();
            bool caseSensitive=isCaseSensitive();

            if(TextFilterHandler::Instance()->contains(filter,matchMode,caseSensitive))
            {
                return;
            }

            TextFilterHandler::Instance()->add(filter,matchMode,caseSensitive,true);
            refreshCompleter();
        }
        else if(ac == clearAc)
        {

            refreshCompleter();
        }
        else
        {
            QStringList id=ac->data().toString().split("_");
            if(id.count() == 2)
            {
                std::size_t pos=id[1].toInt();
                TextFilterItem item("","");
                if(id[0] == "s")
                {
                    if(pos >=0 && TextFilterHandler::Instance()->items().size())
                    {
                        item=TextFilterHandler::Instance()->items()[pos];
                    }
                }

                else if(id[0] == "r")
                {
                    if(pos >=0 && TextFilterHandler::Instance()->latestItems().size())
                    {
                        item=TextFilterHandler::Instance()->latestItems()[pos];
                    }
                }

                if(!item.filter().empty())
                {
                    init(item);
                    Q_EMIT runRequested(QString::fromStdString(item.filter()),item.matched(),item.caseSensitive());
                }
            }
        }

    }

    menu->clear();
    menu->deleteLater();
}

void TextFilterWidget::addMenuSection(QMenu* menu,const std::vector<TextFilterItem>& items,QString title,QString data)
{
    if(items.empty())
        return;

    QAction *sep1=new QAction(menu);
    sep1->setSeparator(true);
    menu->addAction(sep1);

    if(!title.isEmpty())
    {
        QAction* acTitle = new QAction(menu);
        acTitle->setText(title);
        QFont f=acTitle->font();
        f.setBold(true);
        acTitle->setFont(f);
        menu->addAction(acTitle);
    }

    for(std::size_t i=0 ; i < items.size(); i++)
    {
        //QLabel *label = new QLabel("<i>first</i>second", this);

        // init widget action
        //QWidgetAction *wAc= new QWidgetAction(this);
        //wAc->setDefaultWidget(label);
        //menu->addAction(wAc);

        //QWidgetAction *wAc= new QWidgetAction(this);
        QAction* ac=new QAction(this);
        //QLabel *label = new QLabel(this);

        QString txt=QString::fromStdString(items[i].filter()) +
                    "   <" + QString(items[i].matched()?"m":"u") + "," +
                     QString(items[i].caseSensitive()?"cs":"ci") + ">";

        //label->setText(txt);
        //wAc->setDefaultWidget(label);
        ac->setText(txt);
        ac->setData(data + "_" + QString::number(i)); //set an id for the action
        menu->addAction(ac);

        //menu->addAction(wAc);
    }
}

void TextFilterWidget::on_closeTb__clicked()
{
    hide();
    Q_EMIT closeRequested();
}

void TextFilterWidget::on_le__returnPressed()
{
    QString t=le_->text();
    if(!t.isEmpty())
        Q_EMIT runRequested(t,isMatched(),isCaseSensitive());
}

void TextFilterWidget::on_le__textChanged()
{
    if(status_ != EditStatus)
        setStatus(EditStatus);

    if(!isActive())
        clearRequested();
}

void TextFilterWidget::slotOptionTb()
{
    Q_ASSERT(optionTb_);
    buildMenu(optionTb_);
}

void TextFilterWidget::setEditFocus()
{
    le_->setFocus();
}

void TextFilterWidget::setStatus(FilterStatus status)
{
    if(status_ != status)
    {
        status_=status;

        QBrush br=oriBrush_;
        QPalette p=le_->palette();
        switch(status_)
        {
        case EditStatus:
            br=oriBrush_;
            if(statusTb_) statusTb_->setIcon(QPixmap(":/viewer/filter_decor.svg"));
            break;
        case FoundStatus:
            br=greenBrush_;
            if(statusTb_) statusTb_->setIcon(QPixmap(":/viewer/filter_decor_green.svg"));
            addCurrentToLatest();
            break;
        case NotFoundStatus:
            br=redBrush_;
            if(statusTb_) statusTb_->setIcon(QPixmap(":/viewer/filter_decor_red.svg"));
            break;
        default:
            break;
        }

        p.setBrush(QPalette::Base,br);
        le_->setPalette(p);
    }
    //Q_EMIT statusChanged(status_);
}

void TextFilterWidget::addCurrentToLatest()
{
    TextFilterHandler::Instance()->addLatest(filterText().simplified().toStdString(),
                                             isMatched(),isCaseSensitive(),true);
}

void TextFilterWidget::paintEvent(QPaintEvent *)
{
     QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

#if 0


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

#endif


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
