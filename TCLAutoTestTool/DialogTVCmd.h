#ifndef DIALOGTVCMD_H
#define DIALOGTVCMD_H

#include <QDialog>
#include <QStandardItem>
#include <QStandardItemModel>

namespace Ui {
class DialogTVCmd;
}

class DialogTVCmd : public QDialog
{
    Q_OBJECT

public:
    explicit DialogTVCmd(QWidget *parent = nullptr);
    ~DialogTVCmd();

signals:
    void onSendCmd(const QString&strCmd);

private:
    Ui::DialogTVCmd *ui;

    quint32 m_nCurRow = -1;
    QStandardItemModel *m_model = nullptr;

    void SaveLoadCmdFile(bool save=true);
    void addCommandRow(const QString&strCmd,const QString&strMemo);
};

#endif // DIALOGTVCMD_H
