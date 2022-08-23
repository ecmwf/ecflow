//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputDirWidget.hpp"

#include <QItemSelectionModel>
#include <QTimer>

#include "OutputModel.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "ViewerUtil.hpp"
#include "VReply.hpp"

#include "ui_OutputDirWidget.h"

#define UI_OUTPUTDIRWIDGET_DEBUG_

int OutputDirWidget::updateDirTimeout_=1000*60;

class DirWidgetState
{
public:
    DirWidgetState(OutputDirWidget* owner) : owner_(owner) {}
    virtual ~DirWidgetState() = default;
    virtual void handleLoad(VReply*);
    virtual void handleFailed(VReply*)=0;
    virtual void handleClear();
    virtual void handleEnable() {}
    virtual void handleDisable();
    virtual bool isDisabled() const {return false;}

protected:
    OutputDirWidget* owner_{nullptr};
};

class DirWidgetSuccessState : public DirWidgetState
{
public:
    DirWidgetSuccessState(OutputDirWidget* owner, VReply*);
    void handleLoad(VReply*) override;
    void handleFailed(VReply*) override;

protected:
    void handleLoadInternal(VReply* reply);
};

class DirWidgetFirstFailedState : public DirWidgetState
{
public:
    DirWidgetFirstFailedState(OutputDirWidget* owner, VReply* reply);
    void handleFailed(VReply*) override;
};

class DirWidgetFailedState : public DirWidgetState
{
public:
    DirWidgetFailedState(OutputDirWidget* owner, VReply* reply);
    void handleFailed(VReply*) override;

protected:
    void handleFailedInternal(VReply* reply);
};

class DirWidgetIdleState : public DirWidgetState
{
public:
    DirWidgetIdleState(OutputDirWidget* owner);
    void handleFailed(VReply*) override;
};

class DirWidgetDisabledState : public DirWidgetState
{
public:
    DirWidgetDisabledState(OutputDirWidget* owner);
    void handleLoad(VReply*) override {}
    void handleFailed(VReply*) override {}
    void handleClear() override {}
    void handleEnable() override;
    void handleDisable() override {}
    bool isDisabled() const override {return true;}
};

//-------------------------------
// DirWidgetState
//-------------------------------

void DirWidgetState::handleLoad(VReply* reply)
{
    owner_->transitionTo(new DirWidgetSuccessState(owner_, reply));
}

void DirWidgetState::handleClear()
{
    owner_->transitionTo(new DirWidgetIdleState(owner_));
}

void DirWidgetState::handleDisable()
{
    owner_->transitionTo(new DirWidgetDisabledState(owner_));
}

//-------------------------------
// DirWidgetSuccessState
//-------------------------------

DirWidgetSuccessState::DirWidgetSuccessState(OutputDirWidget* owner, VReply* reply) :
    DirWidgetState(owner)
{
    handleLoadInternal(reply);
}

void DirWidgetSuccessState::handleLoad(VReply* reply)
{
    handleLoadInternal(reply);
}

void DirWidgetSuccessState::handleLoadInternal(VReply* reply)
{
    //We do not display info/warning here! The dirMessageLabel_ is not part of the dirWidget_
    //and is only supposed to display error messages!
    owner_->ui_->messageLabelTop->hide();
    owner_->ui_->infoLabel->show();
    owner_->ui_->reloadTb->show();
    owner_->ui_->messageLabelBottom->hide();
    owner_->ui_->view->show();

    //Update the dir widget and select the proper file in the list
    owner_->updateContents(reply->directories(), true);

    //Update the dir label
    owner_->ui_->infoLabel->update(reply);

    //Enable the update button
    owner_->ui_->reloadTb->setEnabled(true);

    owner_->show();
}

void DirWidgetSuccessState::handleFailed(VReply* reply)
{
    owner_->transitionTo(new DirWidgetFirstFailedState(owner_, reply));
}

//-------------------------------
// DirWidgetFirstFailedState
//-------------------------------

DirWidgetFirstFailedState::DirWidgetFirstFailedState(OutputDirWidget* owner, VReply* reply) : DirWidgetState(owner)
{
    UI_FN_DBG
    //the timer is stopped. It will be restarted again if we get a local file or
    //a file via the logserver
    owner_->stopTimer();

    //We do not have directories
    owner_->dirModel_->clearData();

    // only show the top row with a messageLabel
    owner_->ui_->messageLabelTop->show();
    owner_->ui_->infoLabel->hide();
    owner_->ui_->reloadTb->show();
    owner_->ui_->messageLabelBottom->hide();
    owner_->ui_->view->hide();

    auto err = owner_->formatErrors(reply->errorTextVec());
    UiLog().dbg() << " err=" << err;
    if (!err.isEmpty()) {
        owner_->ui_->messageLabelTop->showError(err);
    } else {
        owner_->ui_->messageLabelTop->showError("Failed to fetch directory listing");
    }

    owner_->ui_->reloadTb->setEnabled(true);
    owner_->show();

    UiLog().dbg() << " view=" << owner_->ui_->view->isVisible();
}

void DirWidgetFirstFailedState::handleFailed(VReply* reply)
{
    owner_->transitionTo(new DirWidgetFailedState(owner_, reply));
}

//-------------------------------
// DirWidgetFailedState
//-------------------------------

DirWidgetFailedState::DirWidgetFailedState(OutputDirWidget* owner, VReply* reply) : DirWidgetState(owner)
{
    handleFailedInternal(reply);
}

void DirWidgetFailedState::handleFailed(VReply* reply)
{
    handleFailedInternal(reply);
}

void DirWidgetFailedState::handleFailedInternal(VReply* reply)
{
    //the timer is stopped. It will be restarted again if we get a local file or
    //a file via the logserver
    owner_->stopTimer();

    //We do not have directories
    owner_->dirModel_->clearData();

    // only show the top row with a messageLabel
    owner_->ui_->messageLabelTop->show();
    owner_->ui_->infoLabel->hide();
    owner_->ui_->reloadTb->show();
    owner_->ui_->messageLabelBottom->hide();
    owner_->ui_->view->hide();

    owner_->ui_->messageLabelTop->showWarning("No access to output directories");

    owner_->ui_->reloadTb->setEnabled(true);
    owner_->show();
}

//===========================================================
// DirWidgetIdleState
//===========================================================

DirWidgetIdleState::DirWidgetIdleState(OutputDirWidget* owner) : DirWidgetState(owner)
{
    owner_->stopTimer();
    owner_->hide();
    owner_->joboutFile_.clear();
    owner_->dirModel_->clearData();
}

void DirWidgetIdleState::handleFailed(VReply* reply)
{
    owner_->transitionTo(new DirWidgetFirstFailedState(owner_, reply));
}

//===========================================================
// DirWidgetDisabledState
//===========================================================

DirWidgetDisabledState::DirWidgetDisabledState(OutputDirWidget* owner) : DirWidgetState(owner)
{
    owner_->stopTimer();
    owner_->hide();
    owner_->joboutFile_.clear();
    owner_->dirModel_->clearData();
}

void DirWidgetDisabledState::handleEnable()
{
    owner_->transitionTo(new DirWidgetIdleState(owner_));
    owner_->reload();
}

//===========================================================
//
// OutputDirWidget
//
//==========================================================

OutputDirWidget::OutputDirWidget(QWidget* parent) :
    QWidget(parent),
    ui_(new Ui::OutputDirWidget)
{
    ui_->setupUi(this);

    //--------------------------------
    // The dir contents
    //--------------------------------

    ui_->messageLabelTop->hide();
    ui_->messageLabelTop->setShowTypeTitle(false);
    ui_->messageLabelBottom->hide();
    ui_->messageLabelBottom->setShowTypeTitle(false);

    //dirLabel_->hide();
    ui_->infoLabel->setProperty("fileInfo","1");

    //The view
    auto* dirDelegate=new OutputDirListDelegate(this);
    ui_->view->setItemDelegate(dirDelegate);
    ui_->view->setRootIsDecorated(false);
    ui_->view->setAllColumnsShowFocus(true);
    ui_->view->setUniformRowHeights(true);
    ui_->view->setAlternatingRowColors(true);
    ui_->view->setSortingEnabled(true);

    //Sort by column "modifiied (ago)", latest files first
    ui_->view->sortByColumn(3, Qt::AscendingOrder);

    //The models
    dirModel_=new OutputModel(this);
    dirSortModel_=new OutputSortModel(this);
    dirSortModel_->setSourceModel(dirModel_);
    dirSortModel_->setSortRole(Qt::UserRole);
    dirSortModel_->setDynamicSortFilter(true);

    ui_->view->setModel(dirSortModel_);

    //When the selection changes in the view
    connect(ui_->view->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,SLOT(slotItemSelected(QModelIndex,QModelIndex)));

    connect(ui_->reloadTb,SIGNAL(clicked()),
            this,SLOT(reload()));

    connect(ui_->closeTb,SIGNAL(clicked()),
            this,SLOT(closeByButton()));

    //Dir contents update timer
    updateTimer_=new QTimer(this);
    updateTimer_->setInterval(updateDirTimeout_);

    connect(updateTimer_,SIGNAL(timeout()),
            this,SLOT(reload()));

    // initialise state
    transitionTo(new DirWidgetIdleState(this));
}


void OutputDirWidget::showIt(bool st)
{
    if (st) {
        state_->handleEnable();
    } else {
        state_->handleDisable();
    }
}

void OutputDirWidget::closeByButton()
{
    state_->handleDisable();
    Q_EMIT closedByButton();
}

void OutputDirWidget::clear()
{
    UI_FN_DBG
    state_->handleClear();
}

void OutputDirWidget::load(VReply* reply, const std::string& joboutFile)
{
    joboutFile_ = joboutFile;
    state_->handleLoad(reply);
}

void OutputDirWidget::failed(VReply* reply, const std::string& joboutFile)
{
    UI_FN_DBG
    joboutFile_ = joboutFile;
    state_->handleFailed(reply);
}

void OutputDirWidget::startTimer()
{
    //if (!state_->isDisabled()) {
        updateTimer_->start();
    //}
}

void OutputDirWidget::stopTimer()
{
     //if (!state_->isDisabled()) {
         updateTimer_->stop();
     //}
}

void OutputDirWidget::reload()
{
    UI_FN_DBG
    if (!state_->isDisabled()) {
        ui_->reloadTb->setEnabled(false);
        Q_EMIT updateRequested();
    }
}

bool OutputDirWidget::currentSelection(std::string& fPath, VDir::FetchMode& mode) const
{
    QModelIndex current=dirSortModel_->mapToSource(ui_->view->currentIndex());
    if(current.isValid())
    {
        dirModel_->itemDesc(current, fPath, mode);
        return true;
    }
    return false;
}

void OutputDirWidget::adjustCurrentSelection(VFile_ptr loadedFile)
{
    UI_FN_DBG
    if (!state_->isDisabled() && loadedFile) {
        adjustCurrentSelection(loadedFile->sourcePath(), loadedFile->fetchMode());
    }
}

void OutputDirWidget::adjustCurrentSelection(const std::string& fPath, VFile::FetchMode fMode)
{
    ignoreOutputSelection_ = true;
    setCurrentSelection(fPath, fMode);
    ignoreOutputSelection_ = false;
}

// set the current item in the directory list based on the contents of the browser. If no mathing
// dir items found the current index is not set. This relies on the sourcePath in the current file
// objects so it has to be propely set!!
void OutputDirWidget::setCurrentSelection(const std::string& fPath, VFile::FetchMode fMode)
{
    if(!dirModel_->isEmpty()) {

#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
        UiLog().dbg() << UI_FN_INFO;
#endif
        QModelIndex idx;

        if (!fPath.empty()) {
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
            UiLog().dbg() << " fPath=" << fPath;
#endif
            if (fPath == joboutFile_) {
                idx = dirModel_->itemToIndex(fPath);
            } else {
                VDir::FetchMode dMode=VDir::NoFetchMode;
                switch(fMode) {
                case VFile::LocalFetchMode:
                    dMode = VDir::LocalFetchMode;
                    break;
                case VFile::LogServerFetchMode:
                    dMode = VDir::LogServerFetchMode;
                    break;
                case VFile::TransferFetchMode:
                    dMode = VDir::TransferFetchMode;
                    break;
                default:
                     dMode=VDir::NoFetchMode;
                    break;
                }
                idx = dirModel_->itemToIndex(fPath, dMode);
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
                UiLog().dbg() << " idx=" << idx <<  " dMode=" << dMode;
#endif
            }
        }

        if (idx.isValid()) {
            ui_->view->setCurrentIndex(dirSortModel_->mapFromSource(idx));
        }
    }
}

void OutputDirWidget::updateContents(const std::vector<VDir_ptr>& dirs, bool restartTimer)
{
    if(restartTimer)
        updateTimer_->stop();

    bool status=false;
    for(const auto & dir : dirs)
    {
        if(dir && dir->count() > 0)
        {
            status=true;
            break;
        }
    }

    if(status)
    {
        ui_->view->selectionModel()->clearSelection();
        dirModel_->resetData(dirs,joboutFile_);

        //Adjust column width
        if(!dirColumnsAdjusted_)
        {
            dirColumnsAdjusted_=true;
            for(int i=0; i< dirModel_->columnCount()-1; i++)
                ui_->view->resizeColumnToContents(i);

            if(dirModel_->columnCount() > 1)
                ui_->view->setColumnWidth(1,ui_->view->columnWidth(0));

        }
#ifdef _UI_OUTPUTITEMWIDGET_DEBUG
        UiLog().dbg() << UI_FN_INFO << "dir item count=" << dirModel_->rowCount();
#endif
    }
    else
    {
        dirModel_->clearData();
    }

    if(restartTimer)
        updateTimer_->start(updateDirTimeout_);
}

bool OutputDirWidget::isEmpty() const
{
    return dirModel_->isEmpty();
}


//void OutputDirWidget::enableDir(bool status)
//{
//    if(status)
//    {
//        dirWidget_->show();
//        dirMessageLabel_->hide();
//        reloadTb_->setEnabled(true);
//    }
//    else
//    {
//        dirWidget_->hide();
//        dirModel_->clearData();
//        dirMessageLabel_->show();
//    }
//}

QString OutputDirWidget::formatErrors(const std::vector<std::string>& errorVec) const
{
    QString s;
    if(errorVec.size() > 0)
    {
        QColor col(70,71,72);
        s=Viewer::formatBoldText("Output directory: ",col);

        if(errorVec.size() > 1)
        {
            for(size_t i=0; i < errorVec.size(); i++)
                s+=Viewer::formatBoldText("[" + QString::number(i+1) + "] ",col) +
                    QString::fromStdString(errorVec[i]) + ". &nbsp;&nbsp;";
        }
        else if(errorVec.size() == 1)
            s+=QString::fromStdString(errorVec[0]);
    }
    return s;
}


//This slot is called when a file item is selected in the dir view
void OutputDirWidget::slotItemSelected(const QModelIndex& currentIdx,const QModelIndex& /*idx2*/)
{
    if(!ignoreOutputSelection_) {
        Q_EMIT itemSelected();
    }
}


void OutputDirWidget::transitionTo(DirWidgetState* state)
{
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
    UiLog().dbg() << UI_FN_INFO << typeid(*state).name() << "\n";
#endif
    if (state_ != nullptr) {
        delete state_;
    }
    state_ = state;
}
