#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QSqlQuery>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    MyModel  *model = new MyModel;

    model->setRowCount(3);
    model->setColumnCount(3);
    ui->tableView->setModel(model);
 //   model->setTable("my_table");
    model->setData(model->index(0, 1), 1, Qt::UserRole);  // All bits off
    model->setData(model->index(1, 1), 1, Qt::UserRole);  // All bits off

    CheckboxDelegate *delegate = new CheckboxDelegate(ui->tableView);
    ui->tableView->setItemDelegateForColumn(1, delegate);
    ui->tableView->setItemDelegateForColumn(2, delegate);
 //   model->select();

    //ui->tableView->setColumnWidth(0, 200);

    // QString q = R"(
    // CREATE TABLE IF NOT EXISTS my_table (
    //     call TEXT,
    //     "10" INTEGER,
    //     "12" INTEGER,
    //     "15" INTEGER,
    //     "17" INTEGER,
    //     "20" INTEGER,
    //     "30" INTEGER,
    //     "40" INTEGER,
    //     "80" INTEGER
    //     )
    // )";

    // if (!query.exec(q))
    // {
    //     qDebug() << "Cannot create table";
    //     return;
    // }

    // query.prepare(R"(INSERT INTO my_table (call, "10", "12") VALUES (?, ?, ?))");
    // query.addBindValue("OH2LHE");
    // query.addBindValue(1);
    // query.addBindValue(15);

    // if (!query.exec()) {
    //     qDebug() << "Insert failed!";
    // } else {
    //     qDebug() << "Insert OK!";
    // }

}

MainWindow::~MainWindow()
{
    delete ui;
}
