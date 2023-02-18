#include "tcpworker.h"
#include <QThread>

TCPWorker::TCPWorker()
{
    tcp_On_Off = true;
    tcp = new QTcpSocket;


}

TCPWorker::~TCPWorker()
{
    if(tcp!=nullptr)
    {
        delete tcp;
    }
}

void TCPWorker::foo()
    {
        qDebug()<<"Worker::Test get called from?: "<<QThread::currentThreadId();
    }

void TCPWorker::Connect(QString ip,qint16 port)
    {

        tcp->connectToHost(ip,port);
        tcp->waitForConnected();
        qDebug()<<"tcpworker connect";
        char *Rec_Temp;
        int Rec_len = 0;
        QByteArray data;
        while(tcp_On_Off){
            if(tcp->waitForReadyRead(1000)){

                //Rec_len = tcp->read(Rec_Temp, 15);
                data = tcp->readAll();

                Rec_Temp = data.data();

                    decode(Rec_Temp);
//                QThread::msleep(1000);


//                qDebug()<<"rec_len:"<<Rec_len;
//                for (int i = 0;i<Rec_len;i++) {
//                   qDebug()<<int(Rec_Temp[i]);
//                }


                //emit sendOnceDistance(Rec_Temp[12],Rec_Temp[13],Rec_Temp[14]);


        }
        }
            qDebug()<<"连接失败";
            tcp->close();
            qDebug()<<"disconnect tcp";

        }

void TCPWorker::decode(char * rec){
    detectCount = int(rec[3]);
    tb_x = short(rec[5])+short(rec[4]<<8);
    tb_y = short(rec[7])+short(rec[6]<<8);
    razer_distance = short(rec[8]<<8)+short(rec[9])+short(rec[10])/256.0;
    turntableHor = (short(rec[12]<<8)+short(rec[11]));
    turntableVer = (short(rec[13])+short(rec[14]<<8));
    if(tb_x<0){
        tb_x = tb_x+256;
    }
    if(tb_y<0){
        tb_y = tb_y+256;
    }
    qDebug()<<"________________"<<tb_y<<short(rec[7])+tb_y<<endl;
    qDebug()<<"________________"<<razer_distance;
    emit GetTcpData(detectCount,tb_x,tb_y,razer_distance,turntableHor/100,turntableVer/100);



}

void TCPWorker::set_tcp_on_off(bool flag){
    tcp_On_Off=flag;
}


void TCPWorker::SendTcpData(uchar *pdata)
{
    for (int i=0;i<24;i++) {
        data[i] = pdata[i];
    }
    tcp->write((char*)data,24);
    //tcp->flush();
    QThread::msleep(100);
}

