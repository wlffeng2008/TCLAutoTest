#ifndef DIALOGSERIALPORTLIST_H
#define DIALOGSERIALPORTLIST_H

#include <QDialog>
#include <QStandardItemModel>

namespace Ui {
class DialogSerialportList;
}

class DialogSerialportList : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSerialportList(QWidget *parent = nullptr);
    ~DialogSerialportList();

private slots:
    void on_pushButtonRefresh_clicked();

private:
    Ui::DialogSerialportList *ui;

    QStandardItemModel *model = nullptr ;
};

#endif // DIALOGSERIALPORTLIST_H
