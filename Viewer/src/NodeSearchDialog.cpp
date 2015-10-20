
#include "NodeSearchDialog.hpp"

#include <QtGlobal>
#include <QVBoxLayout>

#include "ComboMulti.hpp"

NodeSearchDialog::NodeSearchDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    searchLe_->setPlaceholderText(tr("Search"));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    searchLe_->setClearButtonEnabled(true);
    rootLe_->setClearButtonEnabled(true);
#endif

    filtersPanel_->hide();

    //Node type
    typeCb_->addItem("Server");
    typeCb_->addItem("Suite");
    typeCb_->addItem("Family");
    typeCb_->addItem("Task");
    typeCb_->addItem("Alias");


    //Node state
    stateCb_->addItem("Aborted");
    stateCb_->addItem("Active");
    stateCb_->addItem("Complete");
    stateCb_->addItem("Queued");
    stateCb_->addItem("Submitted");
    stateCb_->addItem("Suspended");
    stateCb_->addItem("Unknown");

    //Node attributes
    attrCb_->addItem("Name");
    attrCb_->addItem("Path");
    attrCb_->addItem("Meter");
    attrCb_->addItem("Label name");
    attrCb_->addItem("Label value");
    attrCb_->addItem("Event");
    attrCb_->addItem("Repeat");
    attrCb_->addItem("Date");
    attrCb_->addItem("Time");
    attrCb_->addItem("Limit");
    attrCb_->addItem("Limiter");
    attrCb_->addItem("Late");
    attrCb_->addItem("Trigger");
    attrCb_->addItem("Variable");
    attrCb_->addItem("Generated variable");

    //Flags
    flagCb_->addItem("Late");
    flagCb_->addItem("Date");
    flagCb_->addItem("Message");
    flagCb_->addItem("Time");
    flagCb_->addItem("Rerun");
    flagCb_->addItem("Waiting");
    flagCb_->addItem("Zombie");

    //connect(typeCb_,SIGNAL())

}

NodeSearchDialog::~NodeSearchDialog()
{

}
