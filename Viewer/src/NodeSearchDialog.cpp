
#include "NodeSearchDialog.hpp"
#include "ui_NodeSearchDialog.h"

#include <QVBoxLayout>

#include "ComboMulti.hpp"

NodeSearchDialog::NodeSearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NodeSearchDialog)
{
    ui->setupUi(this);

    #
    ui->nodeStateCb_->addItem("Aborted");
    ui->nodeStateCb_->addItem("Complete");
    ui->nodeStateCb_->addItem("Suspended");
    ui->nodeStateCb_->addItem("Queued");



}

NodeSearchDialog::~NodeSearchDialog()
{
    delete ui;
}
