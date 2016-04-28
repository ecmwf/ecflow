#include "SaveSessionAsDialog.hpp"
#include "ui_SaveSessionAsDialog.h"

SaveSessionAsDialog::SaveSessionAsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveSessionAsDialog)
{
    ui->setupUi(this);
}

SaveSessionAsDialog::~SaveSessionAsDialog()
{
    delete ui;
}
