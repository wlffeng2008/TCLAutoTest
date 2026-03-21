#include "MainWindow.h"
#include "qforeach.h"
#include "ui_MainWindow.h"

#include "DialogSerialportList.h"

#include "DialogTVCmd.h"
#include "DialogTestFlow.h"
#include "DialogSPISetting.h"
#include "EasyToast.h"

#include <windows.h>

#include <QThread>
#include <QTcpSocket>
#include <QDateTime>
#include <QStandardPaths>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>
#include <QDateTime>
#include <QDir>
#include <QCloseEvent>
#include <QClipboard>

#define NO_MSXML_XMLDOCUMENT
#include <windows.h>
#include "tinyxml2.h"
using namespace tinyxml2;

static QStringList TVCmds = {
    "AA 06 10 01 A7 EF",    // 进入工程菜单
    "AA 06 11 03 B4 9C",    // 退出售后服务菜单
    "AA 06 11 02 A4 BD",
    "AA 07 9F 07 01 F1 55", // Local dimming开关
    "AA 06 30 03 A1 09",    // 图效选择(图像状态)
    "AA 07 9F 3E 01 4E 58", // 峰值亮度档位设置
    "AA 07 9F 0A 01 87 09", // 环境光感应开关设置
    "AA 06 25 01 5D 8F",    // 信源选择
    "AA 06 31 01 92 38",    // 色温设置
    "AA 09 93 01 02 03 04 4A 36", // 自然光参数设置

    "AA 06 27 01 3B ED",
    "AA 06 27 00 3B ED",
    "AA 08 28 FF FF FF 0B F6",
    "AA 08 28 FF 00 FF 08 09",
    "AA 08 28 FF 00 00 16 F9",
    "AA 08 28 00 FF 00 DA 65",
    "AA 08 28 00 00 FF C7 6A",
    "AA 08 28 00 00 00 D9 9A",
    "AA 07 7C 03 00 D4 D1",
    "AA 06 3A 01 4E C2",
    "AA 06 36 64 37 AC",
    "AA 06 32 00 D7 4A"
};

uint16_t crc = 0x0000;
const uint16_t polynomial = 0xA001 ; // 0x1021;//
WORD Get_CRC16_Sum(BYTE const* CRC_Buf, WORD nLen)
{
    WORD wCrc = 0xFFFF;
    for (WORD i = 0; i < nLen; i++)
    {
        wCrc ^= CRC_Buf[i]; //异或
        for (BYTE j = 0; j < 8; j++)
        {
            if (wCrc & 0x0001)
                wCrc = (wCrc >> 1) ^ polynomial;
            else
                wCrc = wCrc >> 1;
        }
    }
    return wCrc;
}

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include<tlhelp32.h>
static void killProcess(const QString &processName)
{
    if (processName.isEmpty()) return;

    HANDLE hRootHandle = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hRootHandle) return;

    PROCESSENTRY32 pEntry = {0};
    pEntry.dwSize = sizeof(PROCESSENTRY32);

    BOOL bProcess = ::Process32First(hRootHandle, &pEntry);

    while (bProcess)
    {
        QString strProcName = QString::fromWCharArray(pEntry.szExeFile);
        if (strProcName.compare(processName,Qt::CaseInsensitive) == 0)
        {
            HANDLE handLe = ::OpenProcess(PROCESS_TERMINATE, FALSE, pEntry.th32ProcessID);
            if (handLe) ::TerminateProcess(handLe, 0);
        }
        bProcess = ::Process32Next(hRootHandle, &pEntry);
    }
}

QString getOpenFileName(QWidget *parent = nullptr,
                                   const QString &caption = "打开文件",
                                   const QString &dir = "",
                                   const QString &filter = "",
                                   const QString &defaultFileName = "") {
    QFileDialog dialog(parent, caption, dir, filter);
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (!defaultFileName.isEmpty()) {
        dialog.selectFile(defaultFileName); // 设置默认文件名
    }
    if (dialog.exec() == QDialog::Accepted) {
        return dialog.selectedFiles().first();
    }
    return "";
}

static HWND m_hVisWnd = nullptr;
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    if(!::IsWindowVisible(hWnd))
        return true;
    int length = GetWindowTextLength(hWnd);
    if (length <= 0 || length>256) {
        return TRUE;
    }

    wchar_t title[1024]={0};
    GetWindowText(hWnd,title,length+1);
    QString strTitle = QString::fromStdWString(title);

    qDebug() << hWnd  << strTitle;
    if(strTitle.contains(" - KingstVIS"))
    {
        m_hVisWnd = hWnd;
        ::SetWindowPos(hWnd,HWND_TOPMOST,0,0,800,320,SWP_NOACTIVATE);
        return false;
    }

    return true;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    QString strBuild;

#ifdef _MSC_VER

    QString strName = "MSVC2017";

#if _MSC_VER >= 1930
    strName = "MSVC2022";
#elif _MSC_VER >= 1920
    strName = "MSVC2019";
#else
    strName = "MSVC2017";
#endif

    strBuild = QString("使用 %1[%2] 编译").arg(strName).arg(_MSC_VER);

#else
    strBuild = "非 MSVC 编译(如 MinGW, GCC 等)";
#endif

    QString strTitle = QString("LDM自动化调试(V1.00) -- [Build: %1] [By Qt%2] -- [%3]").arg(__TIMESTAMP__,QT_VERSION_STR,strBuild);
    setWindowTitle(strTitle);

    EnumWindows(EnumWindowsProc,9527);

    //setStyleSheet("* { font: bold 12px 'Microsoft YaHei'; }");
    //setStyleSheet("QLineEdit { font: normal 12px 'Microsoft YaHei'; }");

    ui->spinBox->hide();

    m_bLoading = true;
    m_setting = new QSettings(QCoreApplication::applicationDirPath() + "\\setting.ini", QSettings::IniFormat);

    m_pTVCmd = new DialogTVCmd(this);
    m_pTest  = new DialogTestFlow(this);
    m_pTest->m_pSet = m_setting;
    QTimer::singleShot(200,this,[=]{
        m_pSPI = new DialogSPISetting(this);
    });

    connect(ui->pushButtonTVSendX,&QPushButton::clicked,this,[=]{
        m_pTVCmd->show();
    });
    connect(ui->pushButtonDotest,&QPushButton::clicked,this,[=]{
        m_pTest->show();
        //DoDealData();
    });
    connect(ui->pushButtonSPISet,&QPushButton::clicked,this,[=]{
        m_pSPI->show();
    });

    connect(m_pTVCmd,&DialogTVCmd::onSendCmd,this,[=](const QString&strCmd){
        m_TvCmds.append(strCmd);
        sendTvCmd();
    });

    ui->comboBoxWindows->blockSignals(true);
    ui->comboBoxWindows->addItem("L0");
    for(int i=20; i<54; i++)
    {
        ui->comboBoxWindows->addItem(QString::asprintf("L%02d",i));
    }
    ui->comboBoxWindows->addItem("L100");
    ui->comboBoxWindows->blockSignals(false);
    ui->comboBoxWindows->setCurrentIndex(22);
    connect(ui->comboBoxWindows,&QComboBox::currentIndexChanged,this,[=]{
        ui->lineEditBase3->setText(ui->comboBoxWindows->currentText().trimmed());
    });

    qDebug() << QString::asprintf("%04X", Get_CRC16_Sum( (BYTE *)QByteArray::fromHex(QString("AA 08 28 FF FF FF").toLatin1()).data(),6));

    ui->pushButtonCom0->setText(m_setting->value("port0","COM0").toString());
    ui->pushButtonCom1->setText(m_setting->value("port1","COM1").toString());
    ui->comboBoxBaud0->setCurrentIndex(m_setting->value("baud0",6).toInt());
    ui->comboBoxBaud1->setCurrentIndex(m_setting->value("baud1",6).toInt());
    ui->lineEditUDisk->setText(m_setting->value("udisk","0004-6C3D").toString());

    ui->lineEditOut001->setText(m_setting->value("lumi0","1000").toString());
    ui->lineEditOut011->setText(m_setting->value("lumi1","1001").toString());
    ui->lineEditOut021->setText(m_setting->value("lumi2","1002").toString());

    QStringList tvSet = m_setting->value("TVSet","1,2,0,0,0,0,0,0,0,0,0,0,0").toStringList();

    connect(ui->pushButtonSetDefault,&QPushButton::clicked,this,[=]{
        ui->comboBoxTV0->setCurrentIndex(1);
        ui->comboBoxTV1->setCurrentIndex(2);
        ui->comboBoxTV2->setCurrentIndex(0);
        ui->comboBoxTV3->setCurrentIndex(0);
        ui->comboBoxTV4->setCurrentIndex(0);
        ui->comboBoxTV5->setCurrentIndex(0);
        ui->comboBoxTV6->setCurrentIndex(0);
        ui->comboBoxTV7->setCurrentIndex(0);
        ui->comboBoxTV8->setCurrentIndex(0);

        ui->comboBoxTV0->activated(1);
        ui->comboBoxTV1->activated(2);
        ui->comboBoxTV2->activated(0);
        ui->comboBoxTV3->activated(0);
        ui->comboBoxTV4->activated(0);
        ui->comboBoxTV5->activated(0);
        ui->comboBoxTV6->activated(0);
        ui->comboBoxTV7->activated(0);
        ui->comboBoxTV8->activated(0);
    });

    connect(ui->lineEditInfoFile,&QLineEdit::textChanged,this,[=](const QString&text){
        ReloadInfo();
    });
    connect(ui->pushButtonReload,&QPushButton::clicked,this,[=]{
        ReloadInfo();
    });

    connect(ui->comboModel,&QComboBox::currentIndexChanged,this,[=](int index){
        QString strFile = ui->lineEditInfoFile->text().trimmed();
        Document xlsx(strFile);
        if (!xlsx.load())
        {
            QMessageBox::critical(this, "提示", "文件无法加载！\n" + strFile);
        }
        else
        {
            QStringList readCols = {"B","C","D","E","F","G","H","I","J","K","L","M"};

            for (int col = 0; col < readCols.count(); col++)
            {
                QString strCell = QString::asprintf("%s%d", readCols[col].toStdString().c_str(), index+2);
                QString strValue = xlsx.read(strCell).toString().trimmed();
                if (strValue.isEmpty() && col == 0)
                    continue;

                switch (col)
                {
                case  0: break;
                case  1: ui->lineEditBase1->setText(strValue); break;
                case  2: ui->lineEditBase2->setText(strValue); break;
                case  3: ui->lineEditBase3->setText(strValue); break;
                case  4: ui->lineEditBase4->setText(strValue); break;
                case  5: ui->lineEditBase5->setText(strValue); break;
                case  6: ui->lineEditBase6->setText(strValue); break;
                case  7: ui->lineEditBase7->setText(strValue); break;
                case  8: ui->lineEditBase8->setText(strValue); break;
                case  9: ui->lineEditBase9->setText(strValue); break;
                case 10:ui->lineEditBase10->setText(strValue); break;
                case 11:ui->lineEditBase11->setText(strValue); break;
                default: break;
                }
            }
        }
    });

    QString strExcel=m_setting->value("Excel","./自动化测试导入数据.xlsx").toString().trimmed();
    ui->lineEditInfoFile->setText(strExcel);
    connect(ui->pushButtonLoadInfo,&QPushButton::clicked,this,[=]{
        QString strFile = QFileDialog::getOpenFileName(
            this, "选择配置文件", nullptr, tr("XLSX文件(*.xlsx);;所有文件 (*.*)"));
        if (strFile.isEmpty())
            return;
        ui->lineEditInfoFile->setText(strFile);
    });

    connect(ui->checkBoxOntop,&QCheckBox::clicked,this,[=](bool checked){
        HWND hWnd = (HWND)this->winId();
        ::SetWindowPos(hWnd, checked ? HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    });

    QTimer::singleShot(500,this,[=]{
       ui->checkBoxOpen0->click();
       ui->checkBoxOpen1->click();
    }) ;

    m_tmLM = new QTimer(this);
    m_tmTV = new QTimer(this);

    connect(m_tmLM,&QTimer::timeout,this,[=]{
        m_tmLM->stop();
        EasyToast::critical("亮度计应答超时！");
    });
    connect(m_tmTV,&QTimer::timeout,this,[=]{
        m_tmTV->stop();
        EasyToast::critical("电视机应答超时！");
    });

    // COM0
    connect(ui->pushButtonCom0,&QPushButton::clicked,this,[=]{
        DialogSerialportList comList;
        if(QDialog::Accepted == comList.exec())
        {
            ui->pushButtonCom0->setText(comList.selectedCOM());
            ui->checkBoxOpen0->setChecked(false);
            ui->checkBoxOpen0->click();
            m_setting->setValue("port0",comList.selectedCOM());
        }
    });
    connect(ui->checkBoxOpen0,&QCheckBox::clicked,this,[=](bool checked){

        GenComport *pComTmp = m_COM0;
        if(pComTmp)
        {
            pComTmp->closePort();
            pComTmp->deleteLater();
            pComTmp = nullptr;
        }

        if(checked)
        {
            pComTmp = new GenComport(this);
            int index = ui->comboBoxBaud0->currentIndex();
            m_setting->setValue("baud0",index);
            pComTmp->setPortParam(ui->comboBoxBaud0->currentText().toInt());
            pComTmp->setPortName(ui->pushButtonCom0->text());
        }

        m_COM0 = pComTmp;

        if(pComTmp && pComTmp->isOpen())
        {
            ui->labelStatus0->setText("已连接");
            ui->labelStatus0->setStyleSheet("QLabel{color:blue;}");
            QTimer::singleShot(300,this,[=]{ ui->pushButtonZeroCall->click(); });

            connect(pComTmp,&GenComport::onReceive,this,[=](const QByteArray &data){
                m_tmLM->stop();
                QString reply(data.data());

                // OK00,P1 0.0992179;0.1051576;0.1289090;+0.09\r

                qDebug() << reply;
                if(reply.startsWith("OK") && reply.contains(",P1"))
                {
                    reply = reply.mid(8);
                    QStringList values = reply.split(';');

                    qreal X = values[0].toFloat();
                    qreal Y = values[1].toFloat();
                    qreal Z = values[2].toFloat();

                    qreal x = X / (X+Y+Z);
                    qreal y = Y / (X+Y+Z);

                    switch(m_nLMRead)
                    {
                    case 0:
                        ui->lineEditOut000->setText(QString::asprintf("%f",Y));
                        ui->lineEditOut002->setText(QString::asprintf("%f",x));
                        ui->lineEditOut003->setText(QString::asprintf("%f",y));
                        break;

                    case 1:
                        ui->lineEditOut010->setText(QString::asprintf("%f",Y));
                        ui->lineEditOut012->setText(QString::asprintf("%f",x));
                        ui->lineEditOut013->setText(QString::asprintf("%f",y));
                        break;

                    case 2:
                        ui->lineEditOut020->setText(QString::asprintf("%f",Y));
                        ui->lineEditOut022->setText(QString::asprintf("%f",x));
                        ui->lineEditOut023->setText(QString::asprintf("%f",y));
                        break;
                    }
                    m_nLMRead++;
                }

                QTimer::singleShot(20,this,[=]{ sendLmCmd(); });
            });
        }
        else
        {
            ui->labelStatus0->setText("未连接");
            ui->labelStatus0->setStyleSheet("QLabel{color:red;}");
        }
    });

    connect(ui->pushButtonMeasure,&QPushButton::clicked,this,[=]{
        m_LmCmds.append("STR,1,23\r");
        m_LmCmds.append("STR,1,23\r");
        m_LmCmds.append("XYZ,1\r");
        sendLmCmd();
    });
    connect(ui->pushButtonZeroCall,&QPushButton::clicked,this,[=]{
        m_nLMRead = 0;
        InitTest();
        QDateTime tm = QDateTime::currentDateTime();
        QString strCmd = QString::asprintf("ZRC,1,%d,%d,%d,%d,%d,%d\r",tm.date().year(),
                tm.date().month(),tm.date().day(),tm.time().hour(),tm.time().minute(),tm.time().second());

        m_LmCmds.append(strCmd);
        sendLmCmd();
    });

    // COM1
    connect(ui->pushButtonCom1,&QPushButton::clicked,this,[=]{
        DialogSerialportList comList;
        if(QDialog::Accepted == comList.exec())
        {
            ui->pushButtonCom1->setText(comList.selectedCOM());
            ui->checkBoxOpen1->setChecked(false);
            ui->checkBoxOpen1->click();
            m_setting->setValue("port1",comList.selectedCOM());
        }
    });

    connect(ui->checkBoxOpen1,&QCheckBox::clicked,this,[=](bool checked){
        GenComport *pComTmp = m_COM1;
        if(pComTmp)
        {
            pComTmp->closePort();
            pComTmp->deleteLater();
            pComTmp = nullptr;
        }

        if(checked)
        {
            pComTmp = new GenComport(this);
            int index = ui->comboBoxBaud1->currentIndex();
            m_setting->setValue("baud1",index);
            pComTmp->setPortParam(ui->comboBoxBaud1->currentText().toInt());
            pComTmp->setPortName(ui->pushButtonCom1->text());
        }
        m_COM1 = pComTmp;

        if(pComTmp && pComTmp->isOpen())
        {
            ui->labelStatus1->setText("已连接");
            ui->labelStatus1->setStyleSheet("QLabel{color:blue;}");

            connect(pComTmp,&GenComport::onReceive,this,[=](const QByteArray &data){
                m_tmTV->stop();
                QString reply(data.data());
                //qDebug() << data.toHex(' ').toUpper();

                QTimer::singleShot(20,this,[=]{ sendTvCmd(); });
            });
        }
        else
        {
            ui->labelStatus1->setText("未连接");
            ui->labelStatus1->setStyleSheet("QLabel{color:red;}");
        }
    });

    connect(ui->comboBoxTVCmd,&QComboBox::activated,this,[=](int index){
        if(!m_bLoading)
            ui->pushButtonTVSend->click();
    });

    ui->comboBoxTVCmd->addItems(TVCmds);
    connect(ui->pushButtonTVSend,&QPushButton::clicked,this,[=]{
        m_TvCmds.append(ui->comboBoxTVCmd->currentText());
        sendTvCmd();
    });

    {
        //killProcess("KingstVIS.exe");

        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString strFile = appDataPath + QString("/AppData/Local/kingst/vis.config");
        QFile F(strFile);
        if(!F.exists())
        {
            QFile::copy(QApplication::applicationDirPath() + "/vis.config",strFile);
        }

        QTimer::singleShot(500,this,[=]{
            tinyxml2::XMLDocument doc;
            XMLError error = doc.LoadFile(strFile.toStdString().c_str());

            if (error == XMLError::XML_SUCCESS)
            {
                tinyxml2::XMLElement* settings = doc.RootElement();              // settings
                tinyxml2::XMLElement* global   = settings->FirstChildElement();  // global
                tinyxml2::XMLElement* socket = global->FirstChildElement("enaSocket");
                socket->SetText("1");

                doc.SaveFile(strFile.toStdString().c_str());
            }
            ui->checkBoxStartVIS->click();
        });

        ui->lineEditKingstKIS->setText(m_setting->value("KingstVIS",QApplication::applicationDirPath() + "/KingstVIS/KingstVIS.exe").toString());
        ui->lineEditExcelTemp->setText(m_setting->value("ExcelTemp",QApplication::applicationDirPath() + "/LDM调试和自检表模板.xlsx").toString());

        connect(ui->checkBoxStartVIS,&QCheckBox::clicked,this,[=](bool checked){
            QString strExe = ui->lineEditKingstKIS->text().trimmed();
            int index = strExe.lastIndexOf('/');
            //killProcess(strExe.mid(index+1));
            if(!checked)
            {
                if(m_hVisWnd) ::PostMessage(m_hVisWnd,WM_CLOSE,0,0);
                m_hVisWnd=NULL;
            }
            else
            {
                if(!m_hVisWnd) QProcess::startDetached(strExe, QStringList{});

                QTimer::singleShot(2000,this,[=]{
                    if(!s_sock)
                    {
                        s_sock = new QTcpSocket(this);

                        connect(s_sock,&QAbstractSocket::connected,this,[=]{
                            qDebug() << "Socket Connected ..." ;
                            QTimer::singleShot(200,this,[=]{
                                EnumWindows(EnumWindowsProc,9527);
                            });
                        });

                        connect(s_sock,&QAbstractSocket::disconnected,this,[=]{
                            s_sock->close();
                            s_sock->deleteLater();
                            s_sock = nullptr;
                            qDebug() << "Socket disconnected ...";
                        });

                        connect(s_sock,&QAbstractSocket::readyRead,this,[=]{

                            QString strAck = s_sock->readAll();
                            qDebug() << m_strRCmd << strAck;
                            QString strPath = QApplication::applicationDirPath() + QString("/save");
                            QDir D(strPath);
                            if(!D.exists()) D.mkdir(strPath);
                            if(strAck == "ACK")
                            {
                                if(m_strRCmd == "start")
                                {
                                    m_tmRd0->start(100);
                                }

                                if(m_strRCmd == "export-data")
                                {
                                    m_tmRd1->start(300);
                                }
                            }
                        });

                        s_sock->connectToHost("127.0.0.1",23367);
                    }
                });
            }
        });

        m_tmRd0 = new QTimer(this);
        m_tmRd1 = new QTimer(this);

        connect(m_tmRd0,&QTimer::timeout,this,[=]{
            m_tmRd0->stop();
            ui->pushButtonDotest1->click();
        });
        connect(m_tmRd1,&QTimer::timeout,this,[=]{
            m_tmRd1->stop();
            ui->pushButtonDotest2->click();
        });

        connect(ui->pushButtonDotest1,&QPushButton::clicked,this,[=]{

            m_strRCmd = "export-data";
            QString strPath = QApplication::applicationDirPath() + QString("/save");
            QString strFile = strPath + QString("/kisdata.csv");
            QString strCmd = QString("export-data \"%1\"").arg(strFile);
            s_sock->write(strCmd.toStdString().c_str());
        });
        connect(ui->pushButtonDotest2,&QPushButton::clicked,this,[=]{

            QString strPath = QApplication::applicationDirPath() + QString("/save");
            QString strFile = strPath + QString("/save%1.csv").arg(m_nExport);
            DoSaveFile(strFile);
        });

        connect(m_pSPI,&DialogSPISetting::onLoadConfig,this,[=](int index0,int index1){

            ui->comboBoxDepth->blockSignals(true);
            ui->comboBoxDepth->setCurrentIndex(index0);
            ui->comboBoxDepth->blockSignals(false);

            ui->comboBoxFreq->blockSignals(true);
            ui->comboBoxFreq->setCurrentIndex(index1);
            ui->comboBoxFreq->blockSignals(false);
        });

        ui->comboBoxModel->blockSignals(true);
        ui->comboBoxModel->setCurrentIndex(m_setting->value("deviceModel",2).toInt());
        ui->comboBoxModel->blockSignals(false);

        ui->comboBoxDepth->blockSignals(true);
        ui->comboBoxDepth->setCurrentIndex(m_setting->value("deviceDepth",2).toInt());
        ui->comboBoxDepth->blockSignals(false);

        ui->comboBoxFreq->blockSignals(true);
        ui->comboBoxFreq->setCurrentIndex(m_setting->value("deviceFreq",2).toInt());
        ui->comboBoxFreq->blockSignals(false);

        connect(ui->comboBoxModel,&QComboBox::activated,this,[=](int index){
            m_pSPI->setModel(ui->comboBoxModel->currentText());
            m_setting->setValue("deviceModel",index);
        });
        connect(ui->comboBoxDepth,&QComboBox::activated,this,[=](int index){
            m_pSPI->setDeepth(index);
            m_setting->setValue("deviceDepth",index);
        });
        connect(ui->comboBoxFreq,&QComboBox::activated,this,[=](int index){
            m_pSPI->setFreq(index);
            m_setting->setValue("deviceFreq",index);
        });

        connect(ui->pushButtonFindKIS,&QPushButton::clicked,this,[=](){

            QString strFile = getOpenFileName(
                this, "选择分析仪程序", "KingstVIS", tr("exe文件(*.exe);;所有文件 (*.*)"),"KingstVIS.exe");
            if (strFile.isEmpty())
                return;
            ui->lineEditKingstKIS->setText(strFile);
            m_setting->setValue("KingstVIS",strFile);
        });

        connect(ui->pushButtonFindExcel,&QPushButton::clicked,this,[=](){

            QString strFile = QFileDialog::getOpenFileName(
                this, "选择模板文件", nullptr, tr("XLSX文件(*.xlsx);;所有文件 (*.*)"));
            if (strFile.isEmpty())
                return;
            ui->lineEditExcelTemp->setText(strFile);
            m_setting->setValue("ExcelTemp",strFile);
        });

        connect(ui->pushButtonExcel,&QPushButton::clicked,this,[=](){
            QString strFile = ui->lineEditExcelTemp->text().trimmed();

            Document xlsx(strFile);
            if ( xlsx.load() && xlsx.selectSheet("LDM调试和自检清单"))
            {
                xlsx.write("F10","1");
                xlsx.write("F11","1");
                xlsx.write("F12","1");
                xlsx.write("F13","1");

                xlsx.write("F15","1");
                xlsx.write("F16","1");

                xlsx.write("F18","1");
                xlsx.write("F19","1");
                xlsx.write("F20","1");

                xlsx.write("E10","1");
                xlsx.write("E11","1");
                xlsx.write("E12","1");
                xlsx.write("E13","1");

                xlsx.write("E15","1");
                xlsx.write("E16","1");

                xlsx.write("E18","1");
                xlsx.write("E19","1");
                xlsx.write("E20","1");


                xlsx.write("D10","1");
                xlsx.write("D11","1");
                xlsx.write("D12","1");
                xlsx.write("D13","1");

                xlsx.write("D15","1");
                xlsx.write("D16","1");

                xlsx.write("D20",0.2233);
                xlsx.write("D29",0.2333);
                xlsx.write("D30",0.2533);
                xlsx.write("D31",0.2633);
                xlsx.write("D32",0.004451233455);

                QString strPath(QApplication::applicationDirPath() + QString("/output"));
                QDir D(strPath);
                if(!D.exists()) D.mkdir(strPath);
                QDateTime now = QDateTime::currentDateTime();
                strFile = strPath+QString("/LDM调试和自检表(%1).xlsx").arg(now.toString("yyyy-MM-dd_HH-mm"));
                if(xlsx.saveAs(strFile))
                {
                    QMessageBox::information(this, "提示", "数据导出成功！\n" + strFile);
                }
                else
                {
                    QMessageBox::critical(this, "提示", "数据导出失败！\n" + strFile);
                }
            }
            else
            {
                QMessageBox::critical(this, "提示", "模板文件无法打开！\n" + strFile);
            }
        });
    }

    {
        connect(ui->lineEditBase3,&QLineEdit::textChanged,this,[=](const QString&text){
            ShowImage();
        });
        connect(ui->pushButtonShowImage,&QPushButton::clicked,this,[=](){
            ShowImage();
        });
    }

    {
        connect(ui->radioButtonWhite,&QRadioButton::clicked,this,[=]{
            DoSetMode(0);
        });
        connect(ui->radioButtonRGB,&QRadioButton::clicked,this,[=]{
            DoSetMode(1);
        });
    }

    {
        connect(ui->comboBoxTV0,&QComboBox::activated,this,[=](int index){
            if(m_bLoading) return;

            QStringList cmds={
                "AA 06 25 01 5D 8F",
                "AA 06 25 02 6D EC",
                "AA 06 25 03 7D CD",
                "AA 06 25 04 0D 2A",
                "AA 06 25 05 1D 0B",
                "AA 06 25 06 2D 68",
                "AA 06 25 07 3D 49",
                "AA 06 25 08 CC A6",
                "AA 06 25 09 DC 87"
            };
            DoSendTV(cmds[index]);
        });

        connect(ui->comboBoxTV1,&QComboBox::activated,this,[=](int index){
            if(m_bLoading) return;
            QStringList cmds={
                "AA 06 30 01 A1 09",
                "AA 06 30 02 91 6A",
                "AA 06 30 03 81 4B",
                "AA 06 30 04 F1 AC",
                "AA 06 30 05 E1 8D",
                "AA 06 30 06 D1 EE",
                "AA 06 30 07 C1 CF",
                "AA 06 30 08 30 20",
                "AA 06 30 09 20 01",
                "AA 06 30 0A 10 62",
                "AA 06 30 0B 00 43",
                "AA 06 30 0C 70 A4",
                "AA 06 30 0D 60 85",
                "AA 06 30 0E 50 E6",
                "AA 06 30 0F 40 C7"
            };
            DoSendTV(cmds[index]);
        });

        connect(ui->comboBoxTV2,&QComboBox::activated,this,[=](int index){
            if(m_bLoading) return;
            QStringList cmds={
                "AA 06 31 01 92 38",
                "AA 06 31 02 A2 5B",
                "AA 06 31 03 B2 7A"
            };
            DoSendTV(cmds[index]);
        });

        connect(ui->comboBoxTV3,&QComboBox::activated,this,[=](int index){
            if(m_bLoading) return;
            QStringList cmds={
                "AA 07 9F 07 01 F1 55",
                "AA 07 9F 07 00 E1 74"
            };
            DoSendTV(cmds[index]);
        });

        connect(ui->comboBoxTV4,&QComboBox::activated,this,[=](int index){
            if(m_bLoading) return;
            QStringList cmds={
                "AA 07 9F 3E 00 5E 79",
                "AA 07 9F 3E 01 4E 58"
            };
            DoSendTV(cmds[index]);
        });

        connect(ui->comboBoxTV5,&QComboBox::activated,this,[=](int index){
            if(m_bLoading) return;
            QStringList cmds={
                "AA 07 9F 0A 00 97 28",
                "AA 07 9F 0A 01 87 09"
            };
            DoSendTV(cmds[index]);
        });

        connect(ui->comboBoxTV6,&QComboBox::activated,this,[=](int index){
            if(m_bLoading) return;
            QStringList cmds={
                "AA 06 25 01 5D 8F",
                "AA 06 25 02 6D EC",
                "AA 06 25 03 7D CD"
            };
            //DoSendTV(cmds[index]);
        });

        connect(ui->comboBoxTV7,&QComboBox::activated,this,[=](int index){
            if(m_bLoading) return;
            QStringList cmds={
                "AA 06 25 01 5D 8F",
                "AA 06 25 02 6D EC",
                "AA 06 25 03 7D CD"
            };
            DoSendTV(cmds[index]);
        });

        connect(ui->comboBoxTV8,&QComboBox::activated,this,[=](int index){
            if(m_bLoading) return;
            QStringList cmds={
                "AA 06 25 01 5D 8F",
                "AA 06 25 02 6D EC",
                "AA 06 25 03 7D CD"
            };
            //DoSendTV(cmds[index]);
        });
    }

    connect(m_pTest,&DialogTestFlow::onTestIndex,this,[=](int item){
        DoTEST(item);
    });

    QTimer::singleShot(100,this,[=]{
        InitTest();
        m_bLoading=false;
    });
}

void MainWindow::DoSendTV(const QString&cmd)
{
    m_TvCmds.append(cmd);
    sendTvCmd();
}

void MainWindow::ShowImage()
{
    QString strFile = QApplication::applicationDirPath() + "/platform-tools/test001.bat";
    QString strAdb  = QApplication::applicationDirPath() + "/platform-tools/adb.exe";
    QString strUDisk = ui->lineEditUDisk->text().trimmed();
    QString strImage = ui->lineEditBase3->text().trimmed();
    QString strType  = "bmp";
    QString strMedia = "image";
    QFile batFile(strFile);
    if(batFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QString strTex = QString("\"%1\" root\n\"%2\" shell am start -a android.intent.action.VIEW -d \"file:///storage/%3/boost_Pattern/%4.%5\" -t \"%6/*\" ").arg(
            strAdb,strAdb,strUDisk,strImage,strType,strMedia);

        QTextStream out(&batFile);

        out<< strTex;

        batFile.close();

        QProcess::execute(strFile,QStringList{});
    }

    m_setting->setValue("udisk",strUDisk);
}

void MainWindow::ReloadInfo()
{
    QString strFile = ui->lineEditInfoFile->text().trimmed();
    Document xlsx(strFile);
    if (!xlsx.load())
    {
        QMessageBox::critical(this, "提示", "文件无法加载！\n" + strFile);
    }
    else
    {
        ui->comboModel->clear();
        QStringList sheets = xlsx.sheetNames();
        foreach (const QString &strSheet, sheets)
        {
            if (!xlsx.selectSheet(strSheet))
                continue;
            for (int i = 2; i < 200; i++)
            {
                QString strCell = QString::asprintf("B%d", i);
                QString strValue = xlsx.read(strCell).toString().trimmed();
                if(strValue.isEmpty()) continue;
                ui->comboModel->addItem(strValue);
            }
            if(ui->comboModel->count())
            {
                ui->comboModel->setCurrentIndex(0);
                m_setting->setValue("Excel",strFile);
            }
            break;
        }
    }
}

void MainWindow::InitTest()
{
    m_nLMRead = 0;
    m_nExport = 1;
    ui->lineEditOut000->setText("--");
    ui->lineEditOut002->setText("");
    ui->lineEditOut003->setText("");

    ui->lineEditOut010->setText("--");
    ui->lineEditOut012->setText("");
    ui->lineEditOut013->setText("");

    ui->lineEditOut020->setText("--");
    ui->lineEditOut022->setText("");
    ui->lineEditOut023->setText("");

    ui->lineEditOut10->setText("--");
    ui->lineEditOut11->setText("--");
    ui->lineEditOut12->setText("--");
    ui->lineEditOut13->setText("--");
    ui->lineEditOut14->setText("--");
    ui->lineEditOut15->setText("--");

    ui->lineEditOut20->setText("--");
    ui->lineEditOut21->setText("--");
    ui->lineEditOut22->setText("--");
    ui->lineEditOut23->setText("--");
    ui->lineEditOut24->setText("--");
    ui->lineEditOut25->setText("--");
}

void MainWindow::sendLmCmd()
{
    m_tmLM->stop();
    int count = m_LmCmds.count();
    if(count <= 0)
        return;

    QString strCmd = m_LmCmds[0];
    m_LmCmds.pop_front();

    if(m_COM0 && m_COM0->isOpen())
    {
        m_tmLM->start(1000);
        m_COM0->send(strCmd,true);
    }
    else
    {
        EasyToast::warning("亮度计串口未连接！");
    }
}

void MainWindow::sendTvCmd()
{
    m_tmTV->stop();
    int count = m_TvCmds.count();
    if(count <= 0)
        return;

    QString strCmd = m_TvCmds[0];
    m_TvCmds.pop_front();

    if(m_COM1 && m_COM1->isOpen())
    {
        m_tmTV->start(1000);
        m_COM1->send(strCmd,false);
    }
    else
    {
        EasyToast::warning("电视机串口未连接！");
    }
}

void MainWindow::DoTEST(int step)
{
    if(!(m_COM0 && m_COM0->isOpen() && m_COM1 && m_COM1->isOpen()))
    {
        m_pTest->toCancel();
        QMessageBox::critical(this, "提示", "设备尚未全部连接，无法进行测试！");
        return;
    }

    if(!s_sock)
    {
        m_pTest->toCancel();
        QMessageBox::critical(this, "提示", "分析仪程序未运行，无法进行测试！");
        return;
    }

    QString strPath = QApplication::applicationDirPath() + QString("/save");
    QDir D(strPath);
    if(!D.exists()) D.mkdir(strPath);

    switch(step)
    {
    case 0:
        m_setting->value("lumi0",ui->lineEditOut001->text().trimmed());
        m_setting->value("lumi1",ui->lineEditOut011->text().trimmed());
        m_setting->value("lumi2",ui->lineEditOut021->text().trimmed());
        QFile::remove(strPath + QString("/kisdata.csv"));
        QFile::remove(strPath + QString("/save1.csv"));
        QFile::remove(strPath + QString("/save2.csv"));
        QFile::remove(strPath + QString("/save3.csv"));
        m_nLMRead = 0;
        m_nExport = 1;
        InitTest();
        DoSendTV("AA 06 10 01 A7 EF");
        DoSendTV("AA 06 27 01 3B ED");
        DoSendTV("AA 08 28 FF FF FF 0B F6");
        QTimer::singleShot(200,this,[=]{
            ui->comboBoxTV1->activated(ui->comboBoxTV1->currentIndex());
            EnumWindows(EnumWindowsProc,9527);
        });
        break;

    case 1:
        ui->pushButtonMeasure->click();
        m_nExport = 1;
        DoRemote();
        break;

    case 2:
        m_Boost = ui->lineEditBase3->text();
        ui->lineEditBase3->setText("L32");
        ui->pushButtonShowImage->click();
        QTimer::singleShot(200,this,[=]{
            ui->comboBoxTV1->activated(ui->comboBoxTV1->currentIndex());
        });
        break;

    case 3:
        ui->pushButtonMeasure->click();
        m_nExport = 2;
        DoRemote();
        break;

    case 4:
        DoSendTV("AA 06 27 01 3B ED");
        DoSendTV("AA 08 28 00 00 00 D9 9A");
        break;

    case 5:
        ui->lineEditBase3->setText(m_Boost);
        ui->pushButtonShowImage->click();
        QTimer::singleShot(200,this,[=]{
            ui->comboBoxTV1->activated(ui->comboBoxTV1->currentIndex());
        });
        break;

    case 6:
        ui->pushButtonMeasure->click();
        m_nExport = 3;
        DoRemote();
        break;

    case 7:
        m_nLMRead = 0;
        //m_nExport = 1;
        DoSendTV("AA 08 28 FF FF FF 0B F6");
        // DoSendTV("AA 06 10 01 A7 EF");
        // DoSendTV("AA 06 11 03 B4 9C");
        // DoSendTV("AA 06 11 02 A4 BD");
        {
            QFile S3(strPath + QString("/save3.csv"));
            if(!S3.exists())
            {
                ui->pushButtonDotest2->click();
            }
        }
        QTimer::singleShot(300,this,[=]{
            DoDealData();
        });
        break;
    }
}

void MainWindow::DoRemote()
{
    m_strRCmd = "start";
    if(s_sock && s_sock->state() == QAbstractSocket::ConnectedState)
        s_sock->write("start");
}

void MainWindow::DoSaveFile(const QString&file)
{
    m_strRCmd = "export-decoded";
    QString strCmd = QString("export-decoded \"%1\"").arg(file);
    if(s_sock && s_sock->state() == QAbstractSocket::ConnectedState)
        s_sock->write(strCmd.toStdString().c_str());

    return;

    SetCursorPos(768,302);
    mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
    mouse_event(MOUSEEVENTF_LEFTUP  ,0,0,0,0);
    QThread::msleep(50);
    SetCursorPos(850,362);

    QThread::msleep(50);

    mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
    mouse_event(MOUSEEVENTF_LEFTUP  ,0,0,0,0);

    QThread::msleep(50);
    QString strTmp = file;
    strTmp.replace("/","\\");
    QClipboard *pClip = QApplication::clipboard();
    pClip->setText(strTmp);

    keybd_event(VK_DOWN,0,0,0);
    keybd_event(VK_DOWN,0,KEYEVENTF_KEYUP,0);
    Sleep(5);
    keybd_event(VK_DOWN,0,0,0);
    keybd_event(VK_DOWN,0,KEYEVENTF_KEYUP,0);
    Sleep(5);
    keybd_event(VK_RETURN,0,0,0);
    keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);

    QTimer::singleShot(500,this,[=]{
        keybd_event(VK_CONTROL,0,0,0);
        keybd_event('V',0,0,0);
        Sleep(5);
        keybd_event('V',0,2,0);
        keybd_event(VK_CONTROL,0,2,0);

        QThread::msleep(100);
        keybd_event(VK_RETURN,0,0,0);
        keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
    });
}

void MainWindow::DoDealData()
{
    {
        QString strPath = QApplication::applicationDirPath() + QString("/save");
        QString strFile = strPath + QString("/kisdata.csv");

        QFile DFile(strFile);
        if (DFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&DFile);
            qDebug() << QString(in.readLine());

            QList<double> Time;
            QList<quint8> col0;
            QList<quint8> col1;
            QList<quint8> col2;
            QList<quint8> col3;
            while (!in.atEnd())
            {
                QString strLine = in.readLine().trimmed();
                QStringList strVals = strLine.split(',');

                Time.push_back(strVals[0].toDouble());
                col0.push_back(strVals[1].toInt());
                col1.push_back(strVals[2].toInt());
                col2.push_back(strVals[3].toInt());
                col3.push_back(strVals[4].toInt());
            }
            DFile.close();

            //T1：有效帧开始后，通道2第3个1对应的时间轴减去第二个1对应的时间轴，就是T1；从理论上看，也就是通道3的1到0，实测数据是对的(导出有效时间轴的第3个数据减去第2个数据:T1=0.00175-0.001742)；
            //T2：有效帧开始后，通道2第1个0对应的时间轴减去通道3第2个0对应的时间轴，就是T2；导出有效时间轴的第4个数据减去第3个数据（T2=0.001953-0.00175）；
            //T3：有效帧开始后，通道0变为第1个1对应的时间轴减去通道2变为0对应的时间轴，就是T3；也可以说是导出有效时间轴后的第5个数据减去第4个数据（T3=0.001955-0.001953）                                                                                                                                                        0.0019549,1,0,0,0,T3,1.520 ,T3：有效帧开始后，通道0变为第1个1对应的时间轴减去通道2变为0对应的时间轴，就是T2；也可以说是导出有效时间轴后的第5个数据减去第4个数据（T3=0.001955-0.001953）

            int count = Time.size();
            int pos = -1 ;
            for(int i=200; i<count-3; i++)
            {
                if(col2[i] == 1 && col2[i+1] == 1 && col2[i+2] == 1)
                {
                    pos = i;
                    break;
                }
            }
            qDebug() << "有效帧开始:" << pos;
            double T1 = (Time[pos+2] - Time[pos+1]) * 1000000;
            qDebug() << "T1 = " << Time[pos+2] << "-" << Time[pos+1] << "=" << T1;

            int zero2 = pos+2;
            for(int i=pos+3; i < count; i++)
            {
                if(col2[i] == 0)
                {
                    zero2 = i;
                    break;
                }
            }

            int zero3 = pos+2;
            for(int i=pos; i<count; i++)
            {
                if(col1[i] == 0 && col1[i+1] == 0)
                {
                    zero3 = i+1;
                    break;
                }
            }
            double T2 = (Time[zero2] - Time[zero3]) * 1000000;
            qDebug() << "T2 = " << Time[zero2] << "-" << Time[zero3] << "=" << T2;

            int zero0 = 0;
            for(int i=pos+2; i<count; i++)
            {
                if(col0[i] == 1)
                {
                    zero0 = i;
                    break;
                }
            }
            double T3 = (Time[zero0] - Time[zero2]) * 1000000;
            qDebug() << "T3 = " << Time[zero0] << "-" << Time[zero2] << "=" << T3;

            int zero4 = pos;
            for(int i=pos+10; i<count-4; i++)
            {
                if(col0[i] == 0 && col0[i+1] == 0 && col0[i+2] == 0 && col0[i+3] == 0)
                {
                    zero4 = i;
                    break;
                }
            }

            int zero5 = pos;
            for(int i=zero4; i<count; i++)
            {
                if(col2[i] == 1)
                {
                    zero5 = i;
                    break;
                }
            }
            double T4 = (Time[zero5] - Time[zero4]) * 1000000;
            qDebug() << "T4 = " << Time[zero5] << "-" << Time[zero4] << "=" << T4;

            for(int i=pos+5; i<count-3; i++)
            {
                if(col2[i] == 1 && col2[i+1] == 1 && col2[i+2] == 1)
                {
                    zero4 = i;
                    break;
                }
            }


            double T5 = (Time[zero4] - Time[pos+4]) * 1000000;
            qDebug() << "T5 = " << Time[zero4] << "-" << Time[pos+4] << "=" << T5;

            int nData = 0;
            for(int i=pos; i<zero4; i++)
                if(col0[i] == 1) nData++;
            qDebug() << "数据量 = " << nData;

            ui->lineEditOut10->setText(QString::asprintf("%d",nData));
            ui->lineEditOut20->setText(QString::asprintf("%f",T5));

            ui->lineEditOut14->setText(QString::asprintf("%f",T1));
            ui->lineEditOut24->setText(QString::asprintf("%f",T2));
            ui->lineEditOut15->setText(QString::asprintf("%f",T3));
            ui->lineEditOut25->setText(QString::asprintf("%f",T4));
        }
    }

    {
        m_nData = 0;
        QString format=ui->lineEditBase7->text().trimmed();
        DoCurrent(1,format);
        DoCurrent(2,format);
        DoCurrent(3,format);
    }
}

void MainWindow::DoCurrent(int index,const QString&format)
{
    QString strPath = QApplication::applicationDirPath() + QString("/save");
    QString strFile = strPath + QString("/save%1.csv").arg(index);

    QFile DFile(strFile);
    if (DFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&DFile);
        qDebug() << QString(in.readLine());

        QList<double> Time;
        QList<quint16> col0;

        bool bFound = false;
        while (!in.atEnd())
        {
            QString strLine = in.readLine().trimmed();
            QStringList strVals = strLine.split(',');
            if(strVals[2].contains("0x55AA"))
            {
                if(bFound) break;

                bFound = true;
            }
            if(!bFound) continue;

            Time.push_back(strVals[0].toDouble());
            QString strCur = strVals[2].replace("0x","");

            col0.push_back(strCur.toInt(nullptr,16));
        }
        DFile.close();

        int count = col0.size();
        if(count < 10)
            return;
        if(m_nData == 0)
        {
            m_nData = (count) * 16;
            ui->lineEditOut10->setText(QString::asprintf("%d",m_nData));
            ui->lineEditOut11->setText(QString::asprintf("%04X %04X %04X",col0[0],col0[1],col0[2]));
            ui->lineEditOut21->setText(QString::asprintf("%04X %04X %04X",col0[count-3],col0[count-2],col0[count-1]));
        }

        int H = ui->lineEditBase1->text().toInt();
        int V = ui->lineEditBase2->text().toInt();
        int pos = (H*V*0.5+0.5*H) + 3;

        if(pos >= count)
            return;
        quint16 current = col0[pos]; // 0x2BFF

        double curBase = ui->lineEditBase5->text().toDouble();
        double curStep = ui->lineEditBase6->text().toDouble();

        //QList<double> CurTable={1.25,2.50,3.75,5.00,6.25,7.50,8.75,10.00,11.25,12.50,13.25};
        //C0 = CurTable[bit0];
        double C0 = 1;
        double C1 = 1;
        quint16 bit0 = 1;
        quint16 bit1 = 1;
        if(format == "6+10")
        {
            bit0 = (current >> 10);
            C0 = curBase + curStep * bit0;

            bit1 = (current&0x3FF);
            C1 = C0 * bit1 / 0x3FF;
        }

        if(format == "4+12")
        {
            bit0 = (current >> 12);
            C0 = curBase + curStep * bit0;

            bit1 = (current&0xFFF);
            C1 = C0 * bit1 / 0xFFF;
        }

        if(format == "8+12")
        {

        }

        qDebug() << "中心点:" << pos << QString::asprintf("0x%04X",current) << bit0;
        qDebug() << "C0 =" << C0  << "C1 =" <<  C1;
        switch(index)
        {
        case 1:
            ui->lineEditOut12->setText(QString::asprintf("%.2f",C1));
            ui->lineEditOut22->setText(QString::asprintf("%.2f",C0));
            break;

        case 2:
            ui->lineEditOut13->setText(QString::asprintf("%.2f",C1));
            break;

        case 3:
            ui->lineEditOut23->setText(QString::asprintf("%.2f",C1));
            break;
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::DoSetMode(int mode)
{
    static QList<QLabel*> Labels={ui->label_3,ui->label_4,ui->label_9,ui->label_10, ui->label_47,ui->label_44,ui->label_53, ui->label_51};
    static QStringList NewTexts={"RGB全白电流(mA)","RGB峰值电流(mA)","RGB拐点电流(mA)","RGB过驱电流(mA)","RGB全白电流(mA)","RGB峰值电流(mA)","RGB拐点电流(mA)","RGB过驱电流(mA)"};
    static QStringList OldTexts;
    if(OldTexts.size() == 0)
        for (QLabel *label: Labels)
            OldTexts.push_back(label->text().trimmed());

    for (int i=0; i<8; i++) {
        QLabel *label = Labels[i];
        QString strText = label->text().trimmed();
        // strText.replace("\n(RGB)","");
        // if(mode == 1)
        //     strText += "\n(RGB)";
        strText = mode == 1 ? NewTexts[i] + QString(":"):OldTexts[i];
        label->setText(strText);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(QMessageBox::question(this, "提示", "确定要退出吗？") != QMessageBox::Yes)
    {
        event->ignore();
        return;
    }

    if(ui->checkBoxStartVIS->isChecked())
        ui->checkBoxStartVIS->click();

    QMainWindow::closeEvent(event);
}