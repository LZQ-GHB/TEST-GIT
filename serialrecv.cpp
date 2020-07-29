#include "serialrecv.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <QDebug>

SerialRecv::SerialRecv(QWidget *parent) : QWidget(parent)
{
    // 串口初始化
    if( -1 == this->serialInit())
        return ;

    // 绑定串口有数信号和串口接收函数
    connect(this->com, &QSerialPort::readyRead, this, &SerialRecv::serialRx);
}

void SerialRecv::serialRx()
{
    //qDebug()<<"rx:"<<com->readAll();

    // 循环读全部数据
    while (!com->atEnd())
    {
        QByteArray data = this->com->read(1);
        //数据处理
        this->rxDataHandle(data[0]);
    }
}

void SerialRecv::rxDataHandle(unsigned char data)
{
    char tmpData;
    // 数据包格式：ID + 数据头 + 数据 + 校验
    // 状态0：空闲，判断进来的数据是不是ID，如果是进入下一步
    // 状态1：接收数据头，进入下一步
    // 状态2： 接收数据，并还原数据，达到指定数据长度时进入下一步
    // 状态3： 数据校验，如果校验正确，下一步做什么?
    // 状态4： 数据

    // 如果接收到最高位为0的数据复位状态机
    if (data < 0x80)
        this->status = 0;

    // 状态机数据处理
    switch (this->status)
    {
    case 0 :
    {
        if( 0x08 == data )
        {
            this->status = 1;
            this->pkgDataCnt = 0; // 有效数据计数
        }else if ( 0x09 == data)
        {
            this->status = 4;
            this->pkgDataCnt = 8;
        }else if ( 0x0a == data)
        {
            this->status = 7;
            this->pkgDataCnt = 10;
        }else qDebug("unknown ID %2x", data);

    }break;
    case 1 :
    {
        this->pkgDataHead = data;
        this->status = 2;
    }break;
    case 2: // 接收有效数据
    {
        tmpData = data&0x7F;
        tmpData = tmpData | (((this->pkgDataHead >> (this->pkgDataCnt))&0x01) << 7);
        this->pkgData[this->pkgDataCnt] = tmpData;// 数据放入缓存区
        // 接收有效数据加1
        this->pkgDataCnt += 1;
        // 判断接收数据长度
        if(this->pkgDataCnt >=7)
            this->status = 3;
    }break;
    case 3: // 接收校验位
    {
        // 接收有效数据加1
        ecg1 = this->pkgData[0];
        ecg1 = ecg1<<8;
        ecg1 = ecg1 + this->pkgData[1];

        ecg2 = this->pkgData[2];
        ecg2 = ecg2<<8;
        ecg2 = ecg2 + this->pkgData[3];

        ecg3 = this->pkgData[4];
        ecg3 = ecg3<<8;
        ecg3 = ecg3 + this->pkgData[5];

       // qDebug()<<"ecgRx: ecg1"<<ecg1<<"ecg2"<<ecg2<<"ecg3"<<ecg3;
        emit RecvData(ecg1);

//        this->pkgData[this->pkgDataCnt] = data;
//        this->pkgDataCnt += 1;
//        if(this->pkgDataCnt >=8)
//
//        if(this->pkgDataCnt >=9)
//

        // 发出数据用于显示
        // todo
    }break;
    case 4:
    {
        this->pkgDataHead = data;
        this->status = 5;
    }break;
    case 5:
    {
        tmpData = data&0x7F;
        tmpData = tmpData | ((this->pkgDataHead&0x01) << 7);
        this->pkgData[this->pkgDataCnt] = tmpData;// 数据放入缓存区
        this->pkgDataCnt += 1;
        // 判断接收数据长度
        if(this->pkgDataCnt >=9)
            this->status = 6;
    }break;
    case 6:
    {
        spo = this->pkgData[8];
        //qDebug()<<"SpoRx:"<<spo;
        emit RecvData(spo);
    }break;
    case 7:
    {
        this->pkgDataHead = data;
        this->status = 8;
    }break;
    case 8:
    {
        tmpData = data&0x7F;
        tmpData = tmpData | ((this->pkgDataHead&0x01) << 7);
        this->pkgData[this->pkgDataCnt] = tmpData;// 数据放入缓存区
        this->pkgDataCnt += 1;
        // 判断接收数据长度
        if(this->pkgDataCnt >=11)
            this->status = 9;
    }break;
    case 9:
    {
        ibp = this->pkgData[10];
       // qDebug()<<"IbpRx:"<<ibp;
        emit RecvData(ibp);
    }
    }
}

int SerialRecv::serialInit()
{
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        qDebug()<<"Name: "<<info.portName();
        qDebug()<<"Description: "<<info.description();
        qDebug()<<"Manufacturer: "<<info.manufacturer();
        qDebug()<<"Serial number: "<<info.serialNumber();
    }
    // 创建串口对象
    this->com = new QSerialPort();
    // 打开串口
    this->com->setPortName("COM10");
    // 尝试打开
    if(!this->com->open(QIODevice::ReadWrite))
    {
        qDebug()<<"open serial error"<<this->com->portName();
        return -1;
    }
    else
        qDebug()<<"open serial success";

    // 串口参数配置
    this->com->setBaudRate(QSerialPort::Baud115200);
    this->com->setDataBits(QSerialPort::Data8);
    this->com->setFlowControl(QSerialPort::NoFlowControl);
    this->com->setParity(QSerialPort::NoParity);
    this->com->setStopBits(QSerialPort::OneStop);


    return 0;
}
