#include "camprotocol.h"


// 回调函数的参数，时间和有无流的判断
typedef struct {
    time_t lasttime;
    bool connected;
} Runner;

// 回调函数
int interrupt_callback(void *p) {
    Runner *r = (Runner *)p;
    if (r->lasttime > 0) {
        if (time(NULL) - r->lasttime > 2 && !r->connected) {
            // 等待超过1s则中断
            return 1;
        }
    }
    return 0;
}

void rtsp::DrawDetectCount(int detectCount){
    this->detectCount = detectCount;
}

void rtsp::DrawMissDistance(int missDistanceX,int missDistanceY){
    this->missDistanceX = missDistanceX;
    this->missDistanceY = missDistanceY;
}

void rtsp::DrawYawPitch(int Yaw,int Pitch){
    this->yaw = Yaw;
    this->pitch = Pitch;
}
void rtsp::DrawRazerDistance(float razerDistance){
    this->razerDistance = razerDistance;
    qDebug()<<razerDistance;
}

void rtsp::run()
    {
    //定义FFMPEG参数指针
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    const AVCodec *pCodec = NULL;
    AVFrame *pFrame;
    AVPacket *packet;
    struct SwsContext *img_convert_ctx_;
    R = 255;
    G = 0;
    B = 0;

    unsigned char *out_buffer;
    int i,videoIndex;
    int ret;
    char errors[1024] = "";

    //绘制设置
    int thickness = 2;
    int lineType = 8;
    char * x_coor[] = {"-170","-90","0","90","170"};
    char * y_coor[] = {"","0","15","30","45","60"};


    //rstp地址设置
    char url[] = "rtsp://192.168.1.112:8554/live";
    QString url1 = "rtsp://"+m_ip+":"+m_RTSPport+"/"+m_stream;
    qDebug()<<url1;
    QByteArray byte = url1.toLocal8Bit();
    strcpy_s(url,byte.size()+1,byte.data());

    //初始化FFMPEG  调用了这个才能正常适用编码器和解码器
    pFormatCtx = avformat_alloc_context();  //init FormatContext

    //初始化FFmpeg网络模块
    avformat_network_init();    //init FFmpeg network

    //判断RTSP超时断开
    Runner input_runner = { 0 };
    pFormatCtx->interrupt_callback.callback = interrupt_callback;
    pFormatCtx->interrupt_callback.opaque = &input_runner;
    input_runner.lasttime = time(NULL);
    input_runner.connected = false;
    AVDictionary* options = nullptr;
    //实时播放使用udp，减小带宽并防止断线
    av_dict_set(&options, "rtsp_transport", "udp", 0);





    //open Media File
     qDebug()<<"+++++++++++++++++++++++++++++++++++++";
    ret = avformat_open_input(&pFormatCtx,byte,NULL,&options);
    if(ret != 0){
        av_strerror(ret,errors,sizeof(errors));
        qDebug() <<"Failed to open video: ["<< ret << "]"<< errors << endl;
        emit open_rtsp_fail("Failed to open video");
        return;
    }
    else{
        input_runner.connected = true;
    }

    //Get audio information
    ret = avformat_find_stream_info(pFormatCtx,NULL);
    if(ret != 0){
        av_strerror(ret,errors,sizeof(errors));
        //cout <<"Failed to get audio info: ["<< ret << "]"<< errors << endl;
        return;
    }

    videoIndex = -1;

    ///循环查找视频中包含的流信息，直到找到视频类型的流
    ///便将其记录下来 videoIndex
    ///这里我们现在只处理视频流  音频流先不管他
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
        }
    }

    ///videoIndex-1 说明没有找到视频流
    if (videoIndex == -1) {
        printf("Didn't find a video stream.\n");
        return;
    }

    //配置编码上下文，AVCodecContext内容
    //1.查找解码器
    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoIndex]->codecpar->codec_id);
    //2.初始化上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    //3.配置上下文相关参数
    avcodec_parameters_to_context(pCodecCtx,pFormatCtx->streams[videoIndex]->codecpar);
    //4.打开解码器
    ret = avcodec_open2(pCodecCtx, pCodec, NULL);
    if(ret != 0){
        av_strerror(ret,errors,sizeof(errors));
        //cout <<"Failed to open Codec Context: ["<< ret << "]"<< errors << endl;
        return;
    }

    //初始化视频帧
    pFrame = av_frame_alloc();
//    pFrameRGB = av_frame_alloc();
    //为out_buffer申请一段存储图像的内存空间
    out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32,pCodecCtx->width,pCodecCtx->height,1));
    //为AVPacket申请内存
    packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    //打印媒体信息
    av_dump_format(pFormatCtx,0,url,0);
    //初始化一个SwsContext
    img_convert_ctx_ = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                AVPixelFormat::AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    cv::Mat frame(pCodecCtx->height, pCodecCtx->width, CV_8UC3);

    //读取帧数据，并通过av_read_frame的返回值确认是不是还有视频帧
    QImage img;
    while(!Run_stopped && av_read_frame(pFormatCtx,packet) >=0){
        //判断视频帧
        if(packet->stream_index == videoIndex){
            //解码视频帧
            ret = avcodec_send_packet(pCodecCtx, packet);
            ret = avcodec_receive_frame(pCodecCtx, pFrame);
            if(ret != 0){
                av_strerror(ret,errors,sizeof(errors));
                //cout <<"Failed to decode video frame: ["<< ret << "]"<< errors << endl;
            }
            if (ret == 0) {
                //处理图像数据
                int cvLinesizes[1];
                cvLinesizes[0] = frame.step1();
                sws_scale(img_convert_ctx_,
                                        (const unsigned char* const*) pFrame->data,
                                        pFrame->linesize, 0, pCodecCtx->height, &frame.data,
                                        cvLinesizes);
                //绘制OSD
                //刻度
                cv::line(frame, cv::Point(450, 80), cv::Point(830, 80),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(450, 70), cv::Point(450, 80),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(540, 70), cv::Point(540, 80),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(640, 70), cv::Point(640, 80),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(740, 70), cv::Point(740, 80),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(830, 70), cv::Point(830, 80),cv::Scalar(R, G, B),thickness,lineType);

                cv::line(frame, cv::Point(80, 160), cv::Point(80, 560),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(70, 160), cv::Point(80, 160),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(70, 260), cv::Point(80, 260),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(70, 360), cv::Point(80, 360),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(70, 460), cv::Point(80, 460),cv::Scalar(R, G, B),thickness,lineType);
                cv::line(frame, cv::Point(70, 560), cv::Point(80, 560),cv::Scalar(R, G, B),thickness,lineType);
                cv::arrowedLine(frame,cv::Point(640+int(float(yaw)/170*190), 85), cv::Point(640+int(float(yaw)/170*190), 84), cv::Scalar(R, G, B), thickness, 4, 0, 10);
                cv::arrowedLine(frame,cv::Point(85, 160+int(float(pitch)/60*400)), cv::Point(84, 160+int(float(pitch)/60*400)), cv::Scalar(R, G, B), thickness, 4, 0, 10);
                for(int i=0;i<5;i++){
                    cv::Size x_text_size = cv::getTextSize(x_coor[i], cv::FONT_HERSHEY_COMPLEX, 0.5, 1,cvLinesizes);
                    cv::Size y_text_size = cv::getTextSize(y_coor[5-i], cv::FONT_HERSHEY_COMPLEX, 0.5, 1,cvLinesizes);
                    if(i==0){
                        cv::putText(frame,x_coor[i],cv::Point(450+i*100-x_text_size.width/2, 60),cv::FONT_HERSHEY_COMPLEX,0.5, cv::Scalar(R, G, B), thickness, cv::LINE_8);
                    }
                    else if(i==4){
                        cv::putText(frame,x_coor[i],cv::Point(430+i*100-x_text_size.width/2, 60),cv::FONT_HERSHEY_COMPLEX,0.5, cv::Scalar(R, G, B), thickness, cv::LINE_8);
                    }
                    else{
                    cv::putText(frame,x_coor[i],cv::Point(440+i*100-x_text_size.width/2, 60),cv::FONT_HERSHEY_COMPLEX,0.5, cv::Scalar(R, G, B), thickness, cv::LINE_8);
                    }
                    cv::putText(frame,y_coor[5-i],cv::Point(70-y_text_size.width, 560-i*100+5),cv::FONT_HERSHEY_COMPLEX,0.5, cv::Scalar(R, G, B), thickness, cv::LINE_8);

                }
                //准星
                cv::line(frame, cv::Point(640, 340), cv::Point(640, 380),cv::Scalar(255, 255, 255),thickness,lineType);
                cv::line(frame, cv::Point(620, 360), cv::Point(660, 360),cv::Scalar(255, 255, 255),thickness,lineType);
                //系统时间
                QDateTime current_date_time =QDateTime::currentDateTime();
                QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss");
                QString detec="detect Count:"+QString::number(detectCount);
                QString missDistance="tracking missDistance:("+QString::number(missDistanceX)+" "+QString::number(missDistanceY)+")";
                QString SrazerDistance="Razer distance:"+ QString::number(razerDistance,'f',2)+"m";
                //QString SrazerDistance="Laser distance:"+ QString::number(20.4,'f',2)+"m";
                //cv::putText(frame,detec.toStdString(),cv::Point(850, 100),cv::FONT_HERSHEY_COMPLEX,0.7, cv::Scalar(R, G, B), thickness, cv::LINE_8);
                cv::putText(frame,missDistance.toStdString(),cv::Point(850, 150),cv::FONT_HERSHEY_COMPLEX,0.7, cv::Scalar(R, G, B), thickness, cv::LINE_8);
                cv::putText(frame,SrazerDistance.toStdString(),cv::Point(850, 200),cv::FONT_HERSHEY_COMPLEX,0.7, cv::Scalar(R, G, B), thickness, cv::LINE_8);

                cv::putText(frame,current_date.toStdString(),cv::Point(50, 650),cv::FONT_HERSHEY_COMPLEX,0.7, cv::Scalar(R, G, B), thickness, cv::LINE_8);
                img=QImage((const uchar*)frame.data,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB888);
                emit sendimg(img,pCodecCtx->width, pCodecCtx->height);
                //释放前需要一个延时
                msleep(1);
            }
        }
        //释放packet空间

        av_packet_unref(packet);
    }

//    //close and release resource
    av_free(out_buffer);
    sws_freeContext(img_convert_ctx_);
    avcodec_close(pCodecCtx);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);

    emit s_set_videolabel_clear();
}

camprotocol::camprotocol()
{
    Rec_Buffer_ptr = new uchar[BUFFER_SIZE];
    freeBytes = new QSemaphore(BUFFER_SIZE);
    usedBytes = new QSemaphore(0);

    shower = new rtsp();
}

camprotocol::~camprotocol()
{
    delete shower;

    delete freeBytes;
    delete usedBytes;
    delete [] Rec_Buffer_ptr;
}

void camprotocol::open_rtsp(QObject *qMain, const QString rtsp_ip,QString port,QString stream,qint16 flag)
{
    if(flag){
        shower->m_ip.clear();
        shower->m_ip = rtsp_ip;
        shower->m_RTSPport.clear();
        shower->m_RTSPport = port;
        shower->m_stream.clear();
        shower->m_stream = stream;
        QObject::connect(shower,SIGNAL(sendimg(QImage,int, int)),qMain,SLOT(Showpic(QImage ,int ,int ))
                         ,Qt::QueuedConnection);
        QObject::connect(shower,SIGNAL(s_set_videolabel_clear()),qMain,SLOT(t_set_videolabel_clear())
                         ,Qt::QueuedConnection);
        shower->Run_stopped = false;
        shower->start(QThread::TimeCriticalPriority);
    }
    else{
        QObject::disconnect(shower,SIGNAL(sendimg(QImage,int,int)),qMain,SLOT(Showpic(QImage ,int ,int )));
        shower->Run_stopped = true;
    }
}

