#include "gencomport.h"
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

GenComport::GenComport(QObject *parent) : QObject(parent)
{

}

GenComport::~GenComport()
{
    closePort() ;
    if(m_serialPort)
        m_serialPort->deleteLater();
}

void GenComport::closePort()
{
    if(m_serialPort)
    {
        if(m_serialPort->isOpen())
        {
            m_serialPort->clear();
            m_serialPort->close();
        }
        delete m_serialPort ;
        m_serialPort = nullptr ;
    }
}

bool GenComport::isOpen()
{
    return (m_serialPort && m_serialPort->isOpen());
}

void GenComport::setPortName(const QString&strName)
{
    m_strName = strName.trimmed();
    openPort(true) ;
}

bool GenComport::openPort(bool bForce)
{
    if(m_strName.length() == 0)
        return false;

    if(bForce)
        closePort() ;

    if(!m_serialPort)
        m_serialPort = new QSerialPort();

    if(!m_serialPort->isOpen())
    {
        m_serialPort->setPortName(m_strName);
        if(!m_serialPort->open(QIODevice::ReadWrite))
        {
            qCritical() << m_strName << "  ------ Open Failed !";
            return false;
        }
        else
        {
            updateSetting() ;
            connect(m_serialPort,SIGNAL(readyRead()),this,SLOT(receive()));
            return true;
        }
    }

    return true;
}

void GenComport::updateSetting()
{
    if(!m_serialPort || !m_serialPort->isOpen())
        return ;

    m_serialPort->setBaudRate(m_nBaudRate,QSerialPort::AllDirections);//设置波特率和读写方向

    static QSerialPort::StopBits StopBtis[] =
    {
        QSerialPort::OneStop,
        QSerialPort::OneAndHalfStop,
        QSerialPort::TwoStop
    } ;

    static QSerialPort::Parity Paritys[] =
    {
        QSerialPort::NoParity,
        QSerialPort::EvenParity,
        QSerialPort::OddParity,
        QSerialPort::SpaceParity,
        QSerialPort::MarkParity
    } ;

    m_serialPort->setDataBits(QSerialPort::DataBits(m_nDataBit));
    m_serialPort->setStopBits(StopBtis[m_nStopBit]);
    m_serialPort->setParity(Paritys[m_nParity]);
    m_serialPort->setFlowControl((QSerialPort::FlowControl)m_nFlowControl);

    qDebug() << m_nBaudRate << m_nDataBit << m_nStopBit << m_nParity << m_nFlowControl ;
}

void GenComport::setPortParam(int nBaudRate,int nDataBit,int nStopBit,int nParity,int nFlowControl)
{
    m_nBaudRate=nBaudRate;
    m_nDataBit =nDataBit;
    m_nStopBit =nStopBit;
    m_nParity  =nParity;
    m_nFlowControl=nFlowControl;
    updateSetting() ;
}

void GenComport::send(const QByteArray &data)
{
    qDebug().noquote() << m_strName << "Send: ----> " << data.toHex(' ').toUpper();
    if(m_serialPort && m_serialPort->isOpen())
    {
        m_serialPort->write(data);
        if(!m_serialPort->waitForBytesWritten())
        {
        }
        waitForRelpySignal(500) ;
    }
    else
    {
        qCritical() << m_strName << " is NOT open!";
    }
}

void GenComport::send(const QString &strData,bool bRaw)
{
    QByteArray data = bRaw ? strData.toUtf8() : QByteArray::fromHex(strData.toLatin1());
    send(data);
}

void GenComport::receive()
{
    if(m_serialPort)
    {
        m_data = m_serialPort->readAll();
        qDebug().noquote()<< m_strName << "Recv: <---- "<<m_data.toHex(' ').toUpper();
        emit onReceive(m_data);
    }
}

bool GenComport::waitForRelpySignal(const unsigned int millisecond)
{
    bool result = true;

    QEventLoop loop;
    connect(m_serialPort, SIGNAL(readyRead()), &loop, SLOT(quit()));

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, [&loop, &result, &timer] { result = false; timer.stop(); loop.quit(); });
    timer.start(millisecond);

    loop.exec();
    timer.stop();
    return result;
}
