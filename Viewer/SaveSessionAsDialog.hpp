#ifndef SAVESESSIONASDIALOG_HPP
#define SAVESESSIONASDIALOG_HPP

#include <QDialog>

namespace Ui {
class SaveSessionAsDialog;
}

class SaveSessionAsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveSessionAsDialog(QWidget *parent = 0);
    ~SaveSessionAsDialog();

private:
    Ui::SaveSessionAsDialog *ui;
};

#endif // SAVESESSIONASDIALOG_HPP
