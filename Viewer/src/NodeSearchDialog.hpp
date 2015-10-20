#ifndef NODESEARCHDIALOG_HPP_
#define NODESEARCHDIALOG_HPP_

#include <QDialog>

#include "ui_NodeSearchDialog.h"


class NodeSearchDialog : public QDialog, protected Ui::NodeSearchDialog
{
    Q_OBJECT

public:
    explicit NodeSearchDialog(QWidget *parent = 0);
    ~NodeSearchDialog();

};

#endif
