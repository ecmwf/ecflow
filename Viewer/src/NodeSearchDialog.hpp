#ifndef NODESEARCHDIALOG_HPP_
#define NODESEARCHDIALOG_HPP_

#include <QDialog>

#include "ServerFilter.hpp"

#include "ui_NodeSearchDialog.h"

class NodeSearchDialog : public QDialog, protected Ui::NodeSearchDialog, public ServerFilterObserver
{
    Q_OBJECT

public:
    explicit NodeSearchDialog(QWidget *parent = 0);
    ~NodeSearchDialog();

    void setServerFilter(ServerFilter*);

    void notifyServerFilterAdded(ServerItem*);
    void notifyServerFilterRemoved(ServerItem*);
    void notifyServerFilterChanged(ServerItem*);
    void notifyServerFilterDelete();

protected Q_SLOTS:
	void buildQuery();
	void slotInCbChanged();
	void slotSearchTermEdited(QString);
	void slotRootNodeEdited(QString);
	void slotExactMatch(bool);
	void slotFind();

private:
	void updateServers();
	void check();

	ServerFilter* serverFilter_;
};

#endif
