#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QMouseEvent>
#include <QSqlTableModel>

class MyModel : public QSqlTableModel
{
public:
    MyModel()
    {
        cellData = false;
     }
    QVariant data(const QModelIndex &index, int role) const
    {
        if (role == Qt::UserRole) {
            return QVariant::fromValue(cellData);  //.at(index.row()).at(index.column()));
        }
        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (role == Qt::UserRole) {
            cellData = value.toBool();
            emit dataChanged(index, index);
            qDebug() << "there";

            return true;
        }
        return false;
    }
    bool cellData;
};



class CheckboxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        bool bools = index.data(Qt::UserRole).value<bool>();

        int boxWidth = option.rect.width() / 4;

        for (int i = 0; i < 4; ++i) {
            QStyleOptionButton checkBoxOption;
            checkBoxOption.state = bools ? QStyle::State_On : QStyle::State_Off;
            checkBoxOption.rect = QRect(
                option.rect.left() + i * boxWidth,
                option.rect.top(),
                boxWidth,
                option.rect.height()
                );
            QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkBoxOption, painter);
        }
    }

    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override
    {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

            //int boxWidth = option.rect.width() / 4;
            //int clickedIndex = (mouseEvent->pos().x() - option.rect.x()) / boxWidth;

            bool bools = index.data(Qt::UserRole).value<bool>();
            bools = !bools;
            model->setData(index, QVariant::fromValue(bools), Qt::UserRole);
            qDebug() << "here" << bools;
         }

        return true;
    }
};




QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
