#ifndef GENCOMPORT_H
#define GENCOMPORT_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

class GenComport : public QObject
{
    Q_OBJECT
public:
    explicit GenComport(QObject *parent = nullptr);
    ~GenComport();

    bool openPort(bool bForce=false) ;
    void closePort();
    bool isOpen();
    void setPortName(const QString&strName);
    QString getPortName(){return m_strName;};
    void setPortParam(int nBaudRate=115200,int nDataBits=8,int nStopBits=1,int nParity=0,int nFlowControl=0);

    void send(const QByteArray &data) ;
    void send(const QString &strData, bool bRaw=false) ;
    QByteArray getData() {return m_data; };
    void clearData() { m_data.clear(); };

    bool waitForRelpySignal(const unsigned int millisecond);

private slots:
    void receive();

signals:
    void onReceive(const QByteArray &data) ;

private:
    QString m_strName ;
    int m_nBaudRate=115200;  //波特率
    int m_nDataBit=8;        //数据位
    int m_nStopBit=0;        //停止位
    int m_nParity=0;         //校验位
    int m_nFlowControl=0;    //流控制

    int m_nType = 0 ;

    QSerialPort* m_serialPort = nullptr;
    QByteArray m_data ;
    void updateSetting() ;
};

#endif // GENCOMPORT_H
