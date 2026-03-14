#include "DialogTestFlow.h"
#include "ui_DialogTestFlow.h"

#include <QTimer>

DialogTestFlow::DialogTestFlow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogTestFlow)
{
    ui->setupUi(this);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(QString("测试内容,使能,等待时间(ms),状态").split(','));
    ui->tableView->setModel(m_model);

    QHeaderView *pHeader = ui->tableView->horizontalHeader() ;
    pHeader->setSectionResizeMode(QHeaderView::Stretch) ;
    pHeader->setSectionResizeMode(0,QHeaderView::Fixed) ;
    pHeader->setSectionResizeMode(1,QHeaderView::Fixed) ;
    pHeader->setSectionResizeMode(2,QHeaderView::Fixed) ;
    pHeader->resizeSection(0,150);
    pHeader->resizeSection(1,32);
    pHeader->resizeSection(2,100);
    ui->tableView->setVerticalHeader(new ZeroBasedHeader(ui->tableView));
    //ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QTimer::singleShot(500,this,[=]{
        QStringList Times = m_pSet->value("WaitTimes").toStringList();
        if(Times.isEmpty())
            Times = QString("3000,2000,3000,2000,15000,4000,2000,2000,2000,1000").split(',');

        addTestItem("全白",Times[0],true);
        addTestItem("读取数据",Times[1],true);
        addTestItem("L32",Times[2],true);
        addTestItem("读取数据",Times[3],true);
        addTestItem("全黑",Times[4],true);
        addTestItem("Boost",Times[5],true);
        addTestItem("读取数据",Times[6],true);
        addTestItem("全白",Times[7],true);
    });

    connect(&m_TMTest,&QTimer::timeout,this,[=]{
        m_TMTest.stop();

        int index = m_nTestIndex;

        if(index >= 0)
            m_model->item(index,3)->setText("Test OK!");

        if(index == m_model->rowCount() - 1)
        {
            toTheEnd();
            return;
        }

        index++;
        m_nTestIndex = index;
        int wait = m_model->item(index,2)->text().trimmed().toInt();

        bool toTest = true;
        if(m_model->item(index,1)->isCheckable())
        {
            toTest = (m_model->item(index,1)->checkState() == Qt::Checked);
            if(!toTest)
                wait = 0;
        }

        if(toTest)
        {
            emit onTestIndex(index);
        }
        else
        {
            m_model->item(index,3)->setText("Pass by ......");
        }

        int waitX = 100;

        if(!toTest) waitX = 100;

        m_TCount = 0;
        m_TMCount.start(100);
        m_TMTest.start(wait + waitX);
    });


    connect(&m_TMCount,&QTimer::timeout,this,[=]{
        m_TCount += 100;
        if(m_nTestIndex != -1)
        {
            QString strInfo = QString::asprintf("Testing(%.1f) ......",m_TCount/1000.0);
            m_model->item(m_nTestIndex,3)->setText(strInfo);
        }
    });

    connect(ui->pushButtonAction,&QPushButton::clicked,this,[=]{
        if(m_nTestIndex == -1)
        {
            startTest();
        }
        else
        {
            toCancel();
        }
    });

    connect(ui->pushButtonSave,&QPushButton::clicked,this,[=]{
        QStringList Values;
        for(int i=0; i<m_model->rowCount(); i++)
            Values.append(m_model->item(i,2)->text().trimmed());
        m_pSet->setValue("WaitTimes",Values);
    });
}

DialogTestFlow::~DialogTestFlow()
{
    delete ui;
}

void DialogTestFlow::toCancel()
{
    m_TMCount.stop();
    m_TMTest.stop();
    m_nTestIndex = -1;
    ui->pushButtonAction->setText("开始测试");
}

void DialogTestFlow::toTheEnd()
{
    toCancel();
    emit onTestToEnd();
}

void DialogTestFlow::startTest()
{
    if(m_nTestIndex != -1)
        return;
    m_nTestIndex = -1 ;
    for(int i=0; i<m_model->rowCount(); i++)
        m_model->item(i,3)->setText("");
    m_TMTest.start(10);
    ui->pushButtonAction->setText("停止测试");
}

void DialogTestFlow::addTestItem(const QString&text, const QString &time, bool option)
{
    QStandardItem *item0 = new QStandardItem(text);
    QStandardItem *item1 = new QStandardItem("");
    QStandardItem *item2 = new QStandardItem(time);
    QStandardItem *item3 = new QStandardItem("");

    item0->setEditable(false);
    item1->setEditable(false);
    item2->setEditable(true);
    item3->setEditable(false);
    item1->setCheckable(option);
    item1->setCheckState(Qt::Checked);

    m_model->appendRow({item0, item1, item2, item3});
    ui->tableView->setRowHeight(m_model->rowCount(),18);
}
