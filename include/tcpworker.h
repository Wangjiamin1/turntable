#ifndef TCPWORKER_H
#define TCPWORKER_H
#include <QtCore>
#include <QTcpSocket>
#include <opencv2/opencv.hpp>


class TCPWorker : public QObject
{
    Q_OBJECT
public:
    TCPWorker();
    ~TCPWorker();
    void foo();
    void set_tcp_on_off(bool);
    uchar *data = new uchar[24];
    void decode(char *);
signals:
    void sendSelfCheck(char,char,char);
    void sendOnceDistance(char,char,char);
    void GetTcpData(int ,int,int,float,int,int);



private:
    QTcpSocket *tcp;
    bool tcp_On_Off;
    int detectCount;
//    cv::Rect rect;
    short tb_x,tb_y;
    float razer_distance;
    short turntableHor;
    short turntableVer;





public slots:
    void Connect(QString ip,qint16 port);
    void SendTcpData(uchar *pdata);
};

#endif // TCPWORKER_H
