#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    openDatabase();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::openDatabase()
{
    const QString dbPath =
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../WWA.db");
    if (!QFile::exists(dbPath)) {
        QFile file(dbPath);
        if (!file.open(QIODevice::WriteOnly)) {
            statusBar()->showMessage("Failed to create WWA.db");
            return false;
        }
        file.close();
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        statusBar()->showMessage("DB open failed: " + db.lastError().text());
        return false;
    }

    statusBar()->showMessage("Opened WWA.db");
    return true;
}
