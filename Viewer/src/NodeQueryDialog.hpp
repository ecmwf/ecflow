#ifndef NODEQUERYDIALOG_HPP_
#define NODEQUERYDIALOG_HPP_

#include <QDialog>

#include "ServerFilter.hpp"

#include "ui_NodeQueryDialog.h"

class ServerFilter;

class NodeQueryDialog : public QDialog, protected Ui::NodeQueryDialog
{
    Q_OBJECT

public:
    explicit NodeQueryDialog(QWidget *parent = 0);
    ~NodeQueryDialog();

    void setServerFilter(ServerFilter*);

protected Q_SLOTS:
	void accept();
    void reject();

protected:
	void closeEvent(QCloseEvent * event);

private:
    void readSettings();
    void writeSettings();
};


#endif
