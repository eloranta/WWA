#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QSqlQuery>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MyModel  *model = new MyModel;

    ui->tableView->setModel(model);
    model->setTable("my_table");


    ui->tableView->setItemDelegate(new CheckboxDelegate(ui->tableView));

    QSqlQuery query;

    QString q = R"(
    CREATE TABLE IF NOT EXISTS my_table (
        call TEXT,
        "10" TEXT,
        "12" TEXT,
        "15" TEXT,
        "17" TEXT,
        "20" TEXT,
        "30" TEXT,
        "40" TEXT,
        "80" TEXT
        )
    )";

    if (!query.exec(q))
    {
        qDebug() << "Cannot create table";
        return;
    }

    query.prepare(R"(INSERT INTO my_table (call, "10", "12") VALUES (?, ?, ?))");
    query.addBindValue("OH2LHE");
    query.addBindValue("CP84");
    query.addBindValue("CP84");

    if (!query.exec()) {
        qDebug() << "Insert failed!";
    } else {
        qDebug() << "Insert OK!";
    }

    model->select();
}

MainWindow::~MainWindow()
{
    delete ui;
}
