#include "DialogTVCmd.h"
#include "ui_DialogTVCmd.h"
#include <QFile>

DialogTVCmd::DialogTVCmd(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogTVCmd)
{
    ui->setupUi(this);

    m_model = new QStandardItemModel();
    m_model->setHorizontalHeaderLabels({"串口命令", "备注"});
    ui->tableView->setModel(m_model);

    QHeaderView *header = ui->tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setSectionResizeMode(1,QHeaderView::Fixed);
    header->resizeSection(1,200);

    //ui->tableView->setShowGrid(false);

    connect(ui->tableView,&QAbstractItemView::clicked,this,[=](const QModelIndex&index){
        m_nCurRow = index.row();
        QStandardItem *item = m_model->item(index.row());
        ui->lineEditTVCommand->setText(item->text().trimmed());
        if(ui->checkBoxAutoSend->isChecked())
            ui->pushButtonSend->click();
    });

    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui->pushButtonHide,&QPushButton::clicked,this,[=]{
        this->hide();
    });
    connect(ui->pushButtonSend,&QPushButton::clicked,this,[=]{
        QString strCmd = ui->lineEditTVCommand->text().trimmed();
        emit onSendCmd(strCmd);
    });

    connect(ui->pushButtonAdd,&QPushButton::clicked,this,[=]{

        QString strCmd = ui->lineEditTVCommand->text().trimmed();
        if(strCmd.isEmpty()) return;

        addCommandRow(strCmd,"备注说明");
        SaveLoadCmdFile();
    });
    connect(ui->pushButtonDel,&QPushButton::clicked,this,[=]{
        int nDel = m_nCurRow;
        m_nCurRow = -1;
        qDebug() << nDel;
        if(nDel != -1)
        {
            m_model->removeRow(nDel);
        }
    });

    SaveLoadCmdFile(false);
}

DialogTVCmd::~DialogTVCmd()
{
    delete ui;
}

void DialogTVCmd::addCommandRow(const QString&strCmd,const QString&strMemo)
{
    QStandardItem *item0 = new QStandardItem(strCmd);
    QStandardItem *item1 = new QStandardItem(strMemo);

    m_model->appendRow({item0,item1});
}

void DialogTVCmd::SaveLoadCmdFile(bool save)
{
    QFile fileCmd("./TVCommand.txt");
    if(save)
    {
        if (fileCmd.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&fileCmd);

            QString strLine;
            int count = m_model->rowCount();
            for(int i=0; i<count; i++)
            {
                QStandardItem *item0 = m_model->item(i,0);
                QStandardItem *item1 = m_model->item(i,1);
                strLine = QString("%1;%2\n").arg(item0->text().trimmed(),item1->text().trimmed());
                out<<strLine;
            }
            fileCmd.close();
        }
    }
    else
    {
        if (fileCmd.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&fileCmd);
            while (!in.atEnd())
            {
                QStringList strVals = in.readLine().trimmed().split(';');
                addCommandRow(strVals[0],strVals[1]);
            }
            fileCmd.close();
        }

        if(m_model->rowCount() == 0)
        {
            addCommandRow("AA 06 10 01 A7 EF",    "进入工程菜单");
            addCommandRow("AA 06 11 03 B4 9C",    "退出售后服务菜单");
            addCommandRow("AA 06 11 02 A4 BD",    "备注说明");
            addCommandRow("AA 07 9F 07 01 F1 55", "Local dimming开关");
            addCommandRow("AA 06 30 03 A1 09",    "图效选择(图像状态)");
            addCommandRow("AA 07 9F 3E 01 4E 58",       "峰值亮度档位设置");
            addCommandRow("AA 07 9F 0A 01 87 09",       "环境光感应开关设置");
            addCommandRow("AA 06 25 01 5D 8F",          "信源选择");
            addCommandRow("AA 06 31 01 92 38",          "色温设置");
            addCommandRow("AA 09 93 01 02 03 04 4A 36", "自然光参数设置");

            addCommandRow("AA 06 27 01 3B ED",      "备注说明");
            addCommandRow("AA 06 27 00 3B ED",      "备注说明");
            addCommandRow("AA 08 28 FF FF FF 0B F6","备注说明");
            addCommandRow("AA 08 28 FF 00 FF 08 09","备注说明");
            addCommandRow("AA 08 28 FF 00 00 16 F9","备注说明");
            addCommandRow("AA 08 28 00 FF 00 DA 65","备注说明");
            addCommandRow("AA 08 28 00 00 FF C7 6A","备注说明");
            addCommandRow("AA 08 28 00 00 00 D9 9A","备注说明");
            addCommandRow("AA 07 7C 03 00 D4 D1",   "备注说明");
            addCommandRow("AA 06 3A 01 4E C2",      "备注说明");
            addCommandRow("AA 06 36 64 37 AC",      "备注说明");
            addCommandRow("AA 06 32 00 D7 4A",      "备注说明");
            SaveLoadCmdFile();
        }
    }
}
