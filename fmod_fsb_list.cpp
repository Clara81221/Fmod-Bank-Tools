#include "fmod_fsb_list.h"
#include "ui_fmod_fsb_list.h"

Fmod_FSB_List::Fmod_FSB_List(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Fmod_FSB_List)
{
    ui->setupUi(this);

    // Populate the model with some data
    QStandardItemModel* model = new QStandardItemModel(1, 4);
    model->setHorizontalHeaderLabels({"FSB Name:", "FSB Version:", "FSB Format", "FSB Size:"});

    model->setItem(0, 0, new QStandardItem("test0.fsb"));
    model->setItem(0, 1, new QStandardItem("FSB5"));
    model->setItem(0, 2, new QStandardItem("Vorbis"));
    model->setItem(0, 3, new QStandardItem("2.5MB"));

    model->setItem(1, 0, new QStandardItem("test1.fsb"));
    model->setItem(1, 1, new QStandardItem("FSB5"));
    model->setItem(1, 2, new QStandardItem("Vorbis"));
    model->setItem(1, 3, new QStandardItem("2.8MB"));

    model->setItem(2, 0, new QStandardItem("test2.fsb"));
    model->setItem(2, 1, new QStandardItem("FSB5"));
    model->setItem(2, 2, new QStandardItem("Vorbis"));
    model->setItem(2, 3, new QStandardItem("4.8MB"));

    ui->tableView->setModel(model);
    ui->tableView->setWindowTitle("Fmod FSB List");
}

Fmod_FSB_List::~Fmod_FSB_List()
{
    delete ui;
}
