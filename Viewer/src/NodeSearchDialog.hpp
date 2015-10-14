#ifndef NODESEARCHDIALOG_HPP_
#define NODESEARCHDIALOG_HPP_

#include <QDialog>

namespace Ui {
class NodeSearchDialog;
}

class NodeSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NodeSearchDialog(QWidget *parent = 0);
    ~NodeSearchDialog();

private:
    Ui::NodeSearchDialog *ui;
};

#endif
