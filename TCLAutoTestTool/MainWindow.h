#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTcpSocket>

#include "gencomport.h"

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

    bool m_bLoading=true;
    GenComport *m_COM0 = nullptr;
    GenComport *m_COM1 = nullptr;

    int m_nLMRead = 0;

    QSettings *m_setting=nullptr;

    QTcpSocket *s_sock=nullptr;
    QStringList m_LmCmds;
    void sendLmCmd();

    QStringList m_TvCmds;
    void sendTvCmd();

    void InitTest();
};
#endif // MAINWINDOW_H
