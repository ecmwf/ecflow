/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TableNodeView.hpp"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QShortcut>
#include <QStyle>
#include <QToolButton>
#include <QtGlobal>

#include "ActionHandler.hpp"
#include "AddModelColumnDialog.hpp"
#include "FilterWidget.hpp"
#include "IconProvider.hpp"
#include "PropertyMapper.hpp"
#include "TableNodeModel.hpp"
#include "TableNodeSortModel.hpp"
#include "TableNodeViewDelegate.hpp"
#include "UiLog.hpp"
#include "VFilter.hpp"
#include "VNode.hpp"
#include "VSettings.hpp"
#include "ViewerUtil.hpp"

#define _UI_TABLENODEVIEW_DEBUG

TableNodeView::TableNodeView(TableNodeSortModel* model, NodeFilterDef* filterDef, QWidget* parent)
    : QTreeView(parent),
      NodeViewBase(filterDef),
      model_(model),
      needItemsLayout_(false),
      prop_(nullptr),
      setCurrentIsRunning_(false),
      setCurrentAfterUpdateIsRunning_(false),
      autoScrollToSelection_(true) {
    setObjectName("view");
    setProperty("style", "nodeView");
    setProperty("view", "table");

    setRootIsDecorated(false);

    // We enable sorting but do not want to perform it immediately
    setSortingEnabledNoExec(true);

    // setSortingEnabled(false);
    // sortByColumn(-1,Qt::AscendingOrder);

    setAllColumnsShowFocus(true);
    setUniformRowHeights(true);
    setMouseTracking(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    //!!!!We need to do it because:
    // The background colour between the views left border and the nodes cannot be
    // controlled by delegates or stylesheets. It always takes the QPalette::Highlight
    // colour from the palette. Here we set this to transparent so that Qt could leave
    // this area empty and we fill it appropriately in our delegate.
    QPalette pal = palette();
    pal.setColor(QPalette::Highlight, QColor(128, 128, 128, 0));
    setPalette(pal);

    // Context menu
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotContextMenu(const QPoint&)));

    // Selection
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotDoubleClickItem(const QModelIndex)));

    actionHandler_ = new ActionHandler(this, this);

    // expandAll();

    // Header
    header_ = new TableNodeHeader(this);

    setHeader(header_);

    // Set header ContextMenuPolicy
    header_->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(
        header_, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotHeaderContextMenu(const QPoint&)));

    connect(header_, SIGNAL(customButtonClicked(QString, QPoint)), this, SIGNAL(headerButtonClicked(QString, QPoint)));

    // for(int i=0; i < model_->columnCount(QModelIndex())-1; i++)
    //   	resizeColumnToContents(i);

    /*connect(header(),SIGNAL(sectionMoved(int,int,int)),
            this, SLOT(slotMessageTreeColumnMoved(int,int,int)));*/

    QTreeView::setModel(model_);

    // Create delegate to the view
    auto* delegate = new TableNodeViewDelegate(this);
    setItemDelegate(delegate);

    connect(delegate, SIGNAL(sizeHintChangedGlobal()), this, SLOT(slotSizeHintChangedGlobal()));

    // Properties
    std::vector<std::string> propVec;
    propVec.emplace_back("view.table.background");
    prop_ = new PropertyMapper(propVec, this);

    // Initialise bg
    adjustBackground(prop_->find("view.table.background")->value().value<QColor>());

    header_->setSortIndicatorShown(true);
}

TableNodeView::~TableNodeView() {
    delete prop_;
}

void TableNodeView::setTableModel(TableNodeSortModel* model) {
    model_ = model;

    // Set the model.
    QTreeView::setModel(model_);
}

QWidget* TableNodeView::realWidget() {
    return this;
}

QObject* TableNodeView::realObject() {
    return this;
}

// Enable sorting without actually performing it!!!
void TableNodeView::setSortingEnabledNoExec(bool b) {
    if (b) {
        model_->setSkipSort(true);
        setSortingEnabled(true);
        model_->setSkipSort(false);
    }
    else {
        setSortingEnabled(false);
    }
}

// Collects the selected list of indexes
QModelIndexList TableNodeView::selectedList() {
    QModelIndexList lst;
    Q_FOREACH (QModelIndex idx, selectedIndexes()) {
        if (idx.column() == 0) {
            lst << idx;
        }
    }
    return lst;
}

// reimplement virtual function from QTreeView - called when the selection is changed
void TableNodeView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    QModelIndexList lst = selectedIndexes();
    if (lst.count() > 0 && !setCurrentAfterUpdateIsRunning_) {
        VInfo_ptr info = model_->nodeInfo(lst.front());
        if (info && !info->isEmpty()) {
#ifdef _UI_TABLENODEVIEW_DEBUG
            UiLog().dbg() << "TableNodeView::selectionChanged --> emit=" << info->path();
#endif
            Q_EMIT selectionChanged(info);
        }
    }
    QTreeView::selectionChanged(selected, deselected);

    // The model has to know about the selection in order to manage the
    // nodes that are forced to be shown
    model_->selectionChanged(lst);
}

VInfo_ptr TableNodeView::currentSelection() {
    QModelIndexList lst = selectedIndexes();
    if (lst.count() > 0) {
        return model_->nodeInfo(lst.front());
    }
    return {};
}

// Sets the current selection to the given VInfo item.
//  called:
//   -from outside of the view when the selection is broadcast from another view
//   -from within the view after a data update
void TableNodeView::setCurrentSelection(VInfo_ptr info) {
    // While the current is being selected we do not allow
    // another setCurrent call go through
    if (setCurrentIsRunning_) {
        return;
    }

    setCurrentIsRunning_ = true;
    QModelIndex idx      = model_->infoToIndex(info);
    if (idx.isValid()) {
#ifdef _UI_TABLENODEVIEW_DEBUG
        if (info) {
            UiLog().dbg() << "TableNodeView::setCurrentSelection --> " << info->path();
        }
#endif
        setCurrentIndex(idx);
    }
    setCurrentIsRunning_ = false;
}

void TableNodeView::setCurrentSelectionAfterUpdate(VInfo_ptr info) {
    // While the current is being selected we do not allow
    // another setCurrent call go through
    if (setCurrentAfterUpdateIsRunning_) {
        return;
    }

    setCurrentAfterUpdateIsRunning_ = true;
    setCurrentSelection(info);
    setCurrentAfterUpdateIsRunning_ = false;
}

void TableNodeView::slotUpdateBegin() {
    lastSelection_ = currentSelection();
}

void TableNodeView::slotUpdateEnd() {
    if (lastSelection_) {
        lastSelection_->regainData();
        if (lastSelection_->hasData()) {
            bool autoScr = hasAutoScroll();
            if (autoScr != autoScrollToSelection_) {
                setAutoScroll(autoScrollToSelection_);
            }

            setCurrentSelectionAfterUpdate(lastSelection_);

            if (autoScr != autoScrollToSelection_) {
                setAutoScroll(autoScr);
            }
        }

        lastSelection_.reset();
    }
}

void TableNodeView::slotSelectionAutoScrollChanged(bool st) {
    autoScrollToSelection_ = st;
}

void TableNodeView::slotDoubleClickItem(const QModelIndex&) {
}

void TableNodeView::slotContextMenu(const QPoint& position) {
    QModelIndexList lst = selectedList();
    // QModelIndex index=indexAt(position);
    QPoint scrollOffset(horizontalScrollBar()->value(), verticalScrollBar()->value());

    handleContextMenu(indexAt(position), lst, mapToGlobal(position), position + scrollOffset, this);
}

void TableNodeView::handleContextMenu(QModelIndex indexClicked,
                                      QModelIndexList indexLst,
                                      QPoint globalPos,
                                      QPoint /*widgetPos*/,
                                      QWidget* /*widget*/) {
    // Node actions
    if (indexClicked.isValid() && indexClicked.column() == 0) // indexLst[0].isValid() && indexLst[0].column() == 0)
    {
        UiLog().dbg() << "context menu " << indexClicked;

        std::vector<VInfo_ptr> nodeLst;
        for (int i = 0; i < indexLst.count(); i++) {
            VInfo_ptr info = model_->nodeInfo(indexLst[i]);
            if (!info->isEmpty()) {
                nodeLst.push_back(info);
            }
        }

        actionHandler_->contextMenu(nodeLst, globalPos);
    }

    // Desktop actions
    else {}
}

void TableNodeView::slotCommandShortcut() {
    if (auto* sc = static_cast<QShortcut*>(QObject::sender())) {
        QModelIndexList indexLst = selectedList();
        std::vector<VInfo_ptr> nodeLst;
        for (int i = 0; i < indexLst.count(); i++) {
            VInfo_ptr info = model_->nodeInfo(indexLst[i]);
            if (info && !info->isEmpty()) {
                nodeLst.push_back(info);
            }
        }
        actionHandler_->runCommand(nodeLst, sc->property("id").toInt());
    }
}

void TableNodeView::slotViewCommand(VInfo_ptr /*info*/, QString /*cmd*/) {
}

void TableNodeView::rerender() {
    if (needItemsLayout_) {
        doItemsLayout();
        needItemsLayout_ = false;
    }
    else {
        viewport()->update();
    }
}

void TableNodeView::slotRerender() {
    rerender();
}

void TableNodeView::slotSizeHintChangedGlobal() {
    needItemsLayout_ = true;
}

void TableNodeView::adjustBackground(QColor col) {
    if (col.isValid()) {
        QString sh = "QTreeView { background : " + col.name() + ";}";
        setStyleSheet(sh);
    }
}

void TableNodeView::notifyChange(VProperty* p) {
    if (p->path() == "view.table.background") {
        adjustBackground(p->value().value<QColor>());
    }
}

//=========================================
// Header
//=========================================

void TableNodeView::collectVariableNames(std::set<std::string>& vars) {
    Q_ASSERT(model_);

    // collect the list of avialable variables using the selected node
    // or if it is not valid the first row in the table
    QModelIndex idx = currentIndex();
    if (!idx.isValid()) {
        idx = model_->index(0, 0);
    }

    if (idx.isValid()) {
        VInfo_ptr info = model_->nodeInfo(idx);
        if (info) {
            if (VNode* vn = info->node()) {
                vn->collectInheritedVariableNames(vars);
            }
        }
    }
}

void TableNodeView::slotAddVariableColumn() {
    Q_ASSERT(model_);

    std::set<std::string> vars;
    collectVariableNames(vars);

    AddModelColumnDialog d(this);
    d.init(model_->columns(), vars);
    d.exec();
}

void TableNodeView::changeVariableColumn(QString varName) {
    Q_ASSERT(model_);

    std::set<std::string> vars;
    collectVariableNames(vars);

    ChangeModelColumnDialog d(this);
    d.init(model_->columns(), vars, varName);
    d.setColumn(varName);
    d.exec();
}

void TableNodeView::slotHeaderContextMenu(const QPoint& position) {
    int section = header_->logicalIndexAt(position);
    if (section < 0 || section >= header_->count()) {
        return;
    }

    int visCnt = 0;
    for (int i = 0; i < header_->count(); i++) {
        if (!header_->isSectionHidden(i)) {
            visCnt++;
        }
    }

    QList<QAction*> lst;
    auto* menu  = new QMenu(this);
    QAction* ac = nullptr;

    // Show/hide current columns
    QString name = header_->model()->headerData(section, Qt::Horizontal).toString();
    ac           = new QAction(menu);
    ac->setData("show_current");
    bool vis = !header_->isSectionHidden(section);
    ac->setText(((!vis) ? "Show column \'" : "Hide column \'") + name + "\'");
    if (vis && visCnt <= 1) {
        ac->setEnabled(false);
    }
    menu->addAction(ac);

    menu->addSeparator();

    // Add special menu for variable columns
    bool varColumn = header_->model()->headerData(section, Qt::Horizontal, AbstractNodeModel::VariableRole).toBool();
    QString varName;

    if (varColumn) {
        varName = header_->model()->headerData(section, Qt::Horizontal).toString();

        ac = new QAction(menu);
        ac->setText(tr("Change column \'") + varName + "\'");
        ac->setData("change");
        menu->addAction(ac);

        ac = new QAction(menu);
        ac->setText(tr("Remove column \'") + varName + "\'");
        ac->setIcon(QPixmap(":viewer/remove.svg"));
        ac->setData("remove");
        menu->addAction(ac);

        menu->addSeparator();
    }

    // Submenu to control the visibility of other columns
    QMenu* visMenu = menu->addMenu("Show/hide other columns");
    for (int i = 0; i < header_->count(); i++) {
        if (i != section) {
            name = header_->model()->headerData(i, Qt::Horizontal).toString();
            ac   = new QAction(visMenu);
            ac->setText(name);
            ac->setCheckable(true);
            ac->setData(i);

            bool vis = !header_->isSectionHidden(i);
            ac->setChecked(vis);

            if (vis && visCnt <= 1) {
                ac->setEnabled(false);
            }

            visMenu->addAction(ac);
        }
    }

    // Show the context menu and check selected action
    ac = menu->exec(header_->mapToGlobal(position));
    if (ac && ac->isEnabled()) {
        if (ac->data().toString() == "change") {
            changeVariableColumn(varName);
        }
        else if (ac->data().toString() == "remove") {
            delete menu;
            if (QMessageBox::question(nullptr,
                                      tr("Confirm: remove column"),
                                      tr("Are you sure that you want to remove column <b>") + varName + "</b>?",
                                      QMessageBox::Ok | QMessageBox::Cancel,
                                      QMessageBox::Cancel) == QMessageBox::Ok) {
                model_->removeColumn(varName);
            }
            return;
        }
        else if (ac->data().toString() == "show_current") {
            header_->setSectionHidden(section, !header_->isSectionHidden(section));
        }
        else if (ac->isCheckable()) {
            int i = ac->data().toInt();
            header_->setSectionHidden(i, !ac->isChecked());
        }
    }
    delete menu;
}

void TableNodeView::readSettings(VSettings* vs) {
    vs->beginGroup("column");

    std::vector<std::string> orderVec;
    std::vector<int> visVec, wVec;

    vs->get("order", orderVec);
    vs->get("visible", visVec);
    vs->get("width", wVec);

    int sortColumn = vs->get<int>("sortColumn", 0);
    int sortOrder  = vs->get<int>("sortOrder", 0);

    vs->endGroup();

    if (orderVec.size() != visVec.size() || orderVec.size() != wVec.size()) {
        return;
    }

    for (size_t i = 0; i < orderVec.size(); i++) {
        std::string id = orderVec[i];
        for (int j = 0; j < model_->columnCount(QModelIndex()); j++) {
            if (model_->headerData(j, Qt::Horizontal, Qt::UserRole).toString().toStdString() == id) {
                if (visVec[i] == 0) {
                    header()->setSectionHidden(j, true);
                }

                else if (wVec[i] > 0) {
                    setColumnWidth(j, wVec[i]);
                }

                break;
            }
        }
    }

    if (header_->count() > 0) {
        int visCnt = 0;
        for (int i = 0; i < header_->count(); i++) {
            if (!header_->isSectionHidden(i)) {
                visCnt++;
            }
        }

        if (visCnt == 0) {
            header()->setSectionHidden(0, false);
        }
    }

    sortByColumn(sortColumn, (sortOrder == 0) ? Qt::AscendingOrder : Qt::DescendingOrder);
}

void TableNodeView::writeSettings(VSettings* vs) {
    vs->beginGroup("column");

    std::vector<std::string> orderVec;
    std::vector<int> visVec, wVec;
    for (int i = 0; i < model_->columnCount(QModelIndex()); i++) {
        std::string id = model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString().toStdString();
        orderVec.push_back(id);
        visVec.push_back((header()->isSectionHidden(i)) ? 0 : 1);
        wVec.push_back(columnWidth(i));
    }

    vs->put("order", orderVec);
    vs->put("visible", visVec);
    vs->put("width", wVec);
    vs->put("sortColumn", header_->sortIndicatorSection());
    vs->put("sortOrder", header_->sortIndicatorOrder());

    vs->endGroup();
}

//=========================================
// TableNodeHeader
//=========================================

TableNodeHeader::TableNodeHeader(QWidget* parent) : QHeaderView(Qt::Horizontal, parent) {
    setStretchLastSection(true);

    connect(this, SIGNAL(sectionResized(int, int, int)), this, SLOT(slotSectionResized(int)));

    int pixId = IconProvider::add(":viewer/filter_decor.svg", "filter_decor");

    customPix_ = IconProvider::pixmap(pixId, 10);

    // connect(this, SIGNAL(sectionMoved(int, int, int)), this,
    //         SLOT(handleSectionMoved(int, int, int)));

    // setMovable(true);
}

void TableNodeHeader::showEvent(QShowEvent* e) {
    /*  for(int i=0;i<count();i++)
      {


         if(1)
         {
             widgets_[i]->setGeometry(sectionViewportPosition(i),0,
                                  sectionSize(i),height());
             widgets_[i]->show();
         }
      }
      */

    QHeaderView::showEvent(e);
}

void TableNodeHeader::slotSectionResized(int /*i*/) {
    /*for (int j=visualIndex(i);j<count();j++)
    {
        int logical = logicalIndex(j);

        if(combo_[logical])
        {
                combo_[logical]->setGeometry(sectionViewportPosition(logical), height()/2,
                                   sectionSize(logical) - 16, height());
        }
   }*/
}

QSize TableNodeHeader::sizeHint() const {
    return QHeaderView::sizeHint();

    QSize s = size();
    // s.setHeight(headerSections[0]->minimumSizeHint().height() + 35);
    // s.setHeight(2*35);
    return s;
}

void TableNodeHeader::setModel(QAbstractItemModel* model) {
    if (model) {
        for (int i = 0; i < model->columnCount(); i++) {
            QString id = model->headerData(i, Qt::Horizontal, Qt::UserRole).toString();
            if (id == "status") {
                customButton_.insert(i, TableNodeHeaderButton(id));
            }
        }
    }
    QHeaderView::setModel(model);
}

void TableNodeHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const {
    painter->save();
    // QHeaderView::paintSection(painter, rect, logicalIndex);
    // painter->restore();

    /*QPixmap customPix(":viewer/filter_decor.svg");
QRect cbRect(0,0,12,12);
    cbRect.moveCenter(QPoint(rect.right()-16-6,rect.center().y()));
    customButton_[logicalIndex].setRect(cbRect);
    painter->drawPixmap(cbRect,pix);*/

    if (!rect.isValid()) {
        return;
    }

    QStyleOptionHeader opt;
    initStyleOption(&opt);
    QStyle::State state = QStyle::State_None;
    if (isEnabled()) {
        state |= QStyle::State_Enabled;
    }
    if (window()->isActiveWindow()) {
        state |= QStyle::State_Active;
    }

    bool clickable;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    clickable = sectionsClickable();
#else
    clickable = isClickable();
#endif

    if (clickable) {
        /*if (logicalIndex == d->hover)
           state |= QStyle::State_MouseOver;
       if (logicalIndex == d->pressed)
           state |= QStyle::State_Sunken;
       else if (d->highlightSelected) {
           if (d->sectionIntersectsSelection(logicalIndex))
               state |= QStyle::State_On;
           if (d->isSectionSelected(logicalIndex))
               state |= QStyle::State_Sunken;
       }*/
    }

    // if(isSortIndicatorShown() && sortIndicatorSection() == logicalIndex)
    //        opt.sortIndicator = (sortIndicatorOrder() == Qt::AscendingOrder)
    //                            ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;

    // setup the style options structure
    // QVariant textAlignment = model->headerData(logicalIndex, d->orientation,
    //                                                 Qt::TextAlignmentRole);
    opt.rect    = rect;
    opt.section = logicalIndex;
    opt.state |= state;
    // opt.textAlignment = Qt::Alignment(textAlignment.isValid()
    //                                      ? Qt::Alignment(textAlignment.toInt())
    //                                      : d->defaultAlignment);

    // opt.text = model()->headerData(logicalIndex, Qt::Horizontal),
    //                                     Qt::DisplayRole).toString();

    QVariant foregroundBrush;
    if (foregroundBrush.canConvert<QBrush>()) {
        opt.palette.setBrush(QPalette::ButtonText, qvariant_cast<QBrush>(foregroundBrush));
    }

    QPointF oldBO = painter->brushOrigin();
    QVariant backgroundBrush;
    if (backgroundBrush.canConvert<QBrush>()) {
        opt.palette.setBrush(QPalette::Button, qvariant_cast<QBrush>(backgroundBrush));
        opt.palette.setBrush(QPalette::Window, qvariant_cast<QBrush>(backgroundBrush));
        painter->setBrushOrigin(opt.rect.topLeft());
    }

    // the section position
    int visual = visualIndex(logicalIndex);
    assert(visual != -1);

    if (count() == 1) {
        opt.position = QStyleOptionHeader::OnlyOneSection;
    }
    else if (visual == 0) {
        opt.position = QStyleOptionHeader::Beginning;
    }
    else if (visual == count() - 1) {
        opt.position = QStyleOptionHeader::End;
    }
    else {
        opt.position = QStyleOptionHeader::Middle;
    }

    opt.orientation = Qt::Horizontal;

    // the selected position
    /*bool previousSelected = d->isSectionSelected(logicalIndex(visual - 1));
        bool nextSelected =  d->isSectionSelected(logicalIndex(visual + 1));
        if (previousSelected && nextSelected)
            opt.selectedPosition = QStyleOptionHeader::NextAndPreviousAreSelected;
        else if (previousSelected)
            opt.selectedPosition = QStyleOptionHeader::PreviousIsSelected;
        else if (nextSelected)
            opt.selectedPosition = QStyleOptionHeader::NextIsSelected;
        else
            opt.selectedPosition = QStyleOptionHeader::NotAdjacent;
    */

    // draw the section
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);
    painter->setBrushOrigin(oldBO);

    painter->restore();

    int rightPos = rect.right();
    if (isSortIndicatorShown() && sortIndicatorSection() == logicalIndex) {
        opt.sortIndicator =
            (sortIndicatorOrder() == Qt::AscendingOrder) ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;
    }

    if (opt.sortIndicator != QStyleOptionHeader::None) {
        QStyleOptionHeader subopt = opt;
        subopt.rect               = style()->subElementRect(QStyle::SE_HeaderArrow, &opt, this);
        rightPos                  = subopt.rect.left();
        style()->drawPrimitive(QStyle::PE_IndicatorHeaderArrow, &subopt, painter, this);
    }

    QMap<int, TableNodeHeaderButton>::iterator it = customButton_.find(logicalIndex);
    if (it != customButton_.end()) {
        // Custom button
        QStyleOptionButton optButton;

        // visPbOpt.text="Visualise";
        optButton.state = QStyle::State_AutoRaise; // QStyle::State_Active | QStyle::State_Enabled;
        // optButton.icon=customIcon_;
        // optButton.iconSize=QSize(12,12);

        int buttonWidth  = customPix_.width();
        int buttonHeight = buttonWidth;
        optButton.rect =
            QRect(rightPos - 4 - buttonWidth, (rect.height() - buttonWidth) / 2, buttonWidth, buttonHeight);

        painter->drawPixmap(optButton.rect, customPix_);

        rightPos = optButton.rect.left();
        it.value().setRect(optButton.rect);
    }

    // Text is left aligned, a decoration icon might be added to left just before the text
    QString text   = model()->headerData(logicalIndex, Qt::Horizontal).toString();
    QRect textRect = rect;
    textRect.setRight(rightPos - 5);
    textRect.adjust(2, 0, 0, 0);

    // Draw icon to the left of the text
    QVariant pixVa = model()->headerData(logicalIndex, Qt::Horizontal, Qt::DecorationRole);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (pixVa.typeId() == QMetaType::QPixmap) {
#else
    if (pixVa.type() == QVariant::Pixmap) {
#endif
        auto pix = pixVa.value<QPixmap>();
        int pixH = qMin(pix.height(), rect.height() - 2);

        QFont f;
        QFontMetrics fm(f);
        QRect pixRect = QRect(rect.x() + 3, rect.center().y() - pixH / 2, pix.width(), pix.height());
        if (pixRect.x() + pixRect.width() + ViewerUtil::textWidth(fm, text) + 4 < textRect.x() + textRect.width()) {
            painter->drawPixmap(pixRect, pix);
            textRect.setX(pixRect.x() + pixRect.width() + 3);
        }
    }

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    // style()->drawControl(QStyle::CE_PushButton, &optButton,painter,this);
}

void TableNodeHeader::mousePressEvent(QMouseEvent* event) {
    QMap<int, TableNodeHeaderButton>::const_iterator it = customButton_.constBegin();
    while (it != customButton_.constEnd()) {
        if (it.value().rect_.contains(event->pos())) {
            UiLog().dbg() << "header " << it.key() << " clicked";

            Q_EMIT customButtonClicked(it.value().id(),
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                                       event->globalPosition().toPoint());
#else
                                       event->globalPos());
#endif
        }
        ++it;
    }

    QHeaderView::mousePressEvent(event);
}

/*void TableNodeHeader::mouseMoveEvent(QMouseEvent *event)
{
        int prevIndex=hoverIndex_;
        QMap<int,TableNodeHeaderButton>::const_iterator it = customButton_.constBegin();
        while(it != customButton_.constEnd())
        {
                if(it.value().rect_.contains(event->pos()))
                {
                        hoverIndex_=it.key();
                        if(hoveIndex != prevIndex)
                        {
                                rerender;
                        }
                }
            ++it;
        }

        if(preIndex !=-1)
        {

        }
        hoverIndex_=-1;
}*/
