#include "mainwindow.h"

#include <QApplication>
#include <QTableView>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

bool setupDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("WWA.db");

    if (!db.open()) {
        qFatal("Cannot open database!");
        return false;
    }

    QSqlQuery query;

    // ✅ Create the table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS modes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            callsign TEXT,
            "10" INTEGER,
            "12" INTEGER,
            "15" INTEGER,
            "17" INTEGER,
            "20" INTEGER,
            "30" INTEGER,
            "40" INTEGER,
            "80" INTEGER
        )
    )");

    // ✅ Only insert if table is empty
    query.exec("SELECT COUNT(*) FROM modes");
    if (query.next() && query.value(0).toInt() == 0) {
        QStringList calls = {
            "3B8WWA","3Z6I","4M5A","4M5DX","4U1A","8A1A","9M2WWA","9M8WWA","A43WWA","AT2WWA","AT3WWA","AT4WWA","AT6WWA","AT7WWA","BA3RA","BA7CK","BG0DXC","BH9CA","BI4SSB","BY1RX",
            "BY2WL","BY5HB","BY6SX","BY8MA","CQ7WWA","CR2WWA","CR5WWA","CR6WWA","D4W","DA0WWA","DL0WWA","DU0WWA","E2WWA","E7W","EG1WWA","EG2WWA","EG3WWA","EG4WWA","EG5WWA","EG6WWA",
            "EG7WWA","EG9WWA","EM0WWA","GB0WWA","GB1WWA","GB2WWA","GB4WWA","GB5WWA","GB6WWA","GB8WWA","GB9WWA","HB9WWA","HI3WWA","HI6WWA","HI7WWA","HI8WWA","HZ1WWA","II0WWA","II1WWA",
            "II2WWA","II3WWA","II4WWA","II5WWA","II6WWA","II7WWA","II8WWA","II9WWA","IR0WWA","IR1WWA","LR1WWA","N0W","N1W","N4W","N6W","N8W","N9W","OL6WWA","OP0WWA","PA26WWA","PC26WWA",
            "PD26WWA","PE26WWA","PF26WWA","RW1F","S53WWA","SB9WWA","SC9WWA","SD9WWA","SN0WWA","SN1WWA","SN2WWA","SN3WWA","SN4WWA","SN6WWA","SO3WWA","SX0W","TK4TH","TM18WWA","TM1WWA",
            "TM29WWA","TM7WWA","TM9WWA","UP7WWA","VB2WWA","VC1WWA","VE9WWA","W4I","YI1RN","YL73R","YO0WWA","YU45MJA","Z30WWA"
        };
        for (const QString &call : calls) {
            query.prepare(R"(
                INSERT INTO modes (callsign, "10", "12", "15", "17", "20", "30", "40", "80")
                VALUES (?, 0, 0, 0, 0, 0, 0, 0, 0)
            )");
            query.addBindValue(call);
            if (!query.exec()) {
                qWarning() << "Insert failed for call:" << call; // << query.lastError();
            }
        }

        qDebug() << "Inserted" << calls.size() << "calls";
    } else {
        qDebug() << "Data already exists, skipping insert.";
    }
    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (!setupDatabase())
        return -1;
    MainWindow window;
    window.show();

    return app.exec();
}
