#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>

#include "gencomport.h"

#include <QXlsx>
using namespace QXlsx;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class DialogTVCmd;
class DialogTestFlow;
class DialogSPISetting;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) final;

private:
    Ui::MainWindow *ui;

    bool m_bLoading = true;
    GenComport *m_COM0 = nullptr;
    GenComport *m_COM1 = nullptr;

    int m_nLMRead = 0;
    QTimer *m_tmLM = nullptr;
    QTimer *m_tmTV = nullptr;
    QTimer *m_tmRd0 = nullptr;
    QTimer *m_tmRd1 = nullptr;

    QSettings *m_setting=nullptr;

    QTcpSocket *s_sock=nullptr;
    QStringList m_LmCmds;
    void sendLmCmd();

    QStringList m_TvCmds;
    void sendTvCmd();

    void InitTest();
    void ShowImage();

    void DoTEST(int step);

    QString m_strRCmd;
    int  m_nRemote=0;
    int  m_nExport=1;
    void DoRemote();

    QString m_Boost;

    void DoSendTV(const QString&cmd);

    void DoSaveFile(const QString&file);
    void DoDealData();
    void DoCurrent(int index,const QString&format);

    void ReloadInfo();
    DialogTVCmd     *m_pTVCmd = nullptr;
    DialogTestFlow  *m_pTest = nullptr;
    DialogSPISetting*m_pSPI = nullptr;
};
#endif // MAINWINDOW_H
