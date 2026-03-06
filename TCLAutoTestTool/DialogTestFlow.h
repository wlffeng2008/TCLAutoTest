#ifndef DIALOGTESTFLOW_H
#define DIALOGTESTFLOW_H

#include <QDialog>
#include <QStandardItem>
#include <QStandardItemModel>

namespace Ui {
class DialogTestFlow;
}

class DialogTestFlow : public QDialog
{
    Q_OBJECT

public:
    explicit DialogTestFlow(QWidget *parent = nullptr);
    ~DialogTestFlow();

private:
    Ui::DialogTestFlow *ui;
    QStandardItemModel *m_model = nullptr;
};

#endif // DIALOGTESTFLOW_H
