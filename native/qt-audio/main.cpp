// GPL-3.0-or-later. Host the unmodified WSJT-X SoundInput on its own audio thread.
#include <QCoreApplication>
#include <QAudioDeviceInfo>
#include <QThread>
#include <QTimer>
#include <QSocketNotifier>
#include <QDebug>
#include <QFile>
#include <atomic>
#include <cstdio>
#include <unistd.h>
#include "Audio/soundin.h"

class PipeSink final : public AudioDevice {
    QByteArray pending;
    bool fixture;
public:
    explicit PipeSink(bool test):fixture(test){}
    qint64 readData(char*,qint64) override {return -1;}
    qint64 writeData(const char *data,qint64 length) override {
        pending.append(data,int(length));
        int frames=pending.size()/int(bytesPerFrame());
        if(!frames)return length;
        QVector<qint16> mono(frames); store(pending.constData(),size_t(frames),mono.data());
        if(!fixture) {
            const char *p=reinterpret_cast<const char*>(mono.constData());
            size_t left=size_t(frames)*sizeof(qint16);
            while(left){ssize_t n=::write(STDOUT_FILENO,p,left);if(n<=0)return -1;p+=n;left-=size_t(n);}
        }
        pending.remove(0,frames*int(bytesPerFrame()));
        return length;
    }
    bool checkChannels() {
        if(!initialize(QIODevice::WriteOnly,Right))return false;
        qint16 source[]={100,1000,200,2000,300,3000}, out[3]={};
        store(reinterpret_cast<char*>(source),3,out);
        return out[0]==1000&&out[1]==2000&&out[2]==3000;
    }
};

int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    auto args=app.arguments();
    if(args.contains("--self-test")) {
        PipeSink sink(true);
        if(!sink.checkChannels())return 1;
        QAudioFormat format;format.setCodec("audio/pcm");format.setSampleRate(48000);
        format.setChannelCount(1);format.setSampleSize(16);format.setSampleType(QAudioFormat::SignedInt);
        format.setByteOrder(QAudioFormat::LittleEndian);
        if(!audioStreamDescriptorFromQAudioFormat(format).isValid())return 2;
        SoundInput input;input.stop();input.suspend();input.resume();
        fprintf(stderr,"Upstream SoundInput lifecycle, PCM descriptor and AudioDevice channel selection passed\n");return 0;
    }
    if(args.size()==3 && args[1]=="--fixture-file") {
        QFile file(args[2]);if(!file.open(QIODevice::ReadOnly))return 6;
        PipeSink sink(false);if(!sink.initialize(QIODevice::WriteOnly,AudioDevice::Mono))return 6;
        QByteArray data=file.readAll();int offset=0;
        for(int chunk: {1,7,333,4095}) {
            int n=qMin(chunk,data.size()-offset);
            if(n>0 && sink.write(data.constData()+offset,n)!=n)return 7;
            offset+=n;
        }
        if(offset<data.size() && sink.write(data.constData()+offset,data.size()-offset)!=data.size()-offset)return 7;
        return data.size()%2?8:0;
    }
    auto devices=QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    if(args.contains("--list")){for(auto device:devices)fprintf(stderr,"%s\n",device.deviceName().toUtf8().constData());return 0;}
    if(args.size()!=4){fprintf(stderr,"ERROR expected device name, channel and downsample factor\n");return 2;}
    QAudioDeviceInfo selected;int matches=0;
    for(auto device:devices)if(device.deviceName()==args[1]){selected=device;++matches;}
    if(matches!=1){fprintf(stderr,"ERROR selected audio device unavailable or ambiguous\n");return 3;}
    bool valid=false;int channel=args[2].toInt(&valid);if(!valid||channel<0||channel>1)return 4;
    unsigned factor=args[3].toUInt(&valid);if(!valid||factor<1||factor>8)return 4;
    QThread audioThread;
    auto *input=new SoundInput;auto *sink=new PipeSink(false);
    input->moveToThread(&audioThread);sink->moveToThread(&audioThread);
    QObject::connect(&audioThread,&QThread::finished,input,&QObject::deleteLater);
    QObject::connect(&audioThread,&QThread::finished,sink,&QObject::deleteLater);
    QObject::connect(input,&AudioInputSource::error,&app,[&](QString message){fprintf(stderr,"ERROR %s\n",message.toUtf8().constData());app.exit(5);});
    QObject::connect(input,&AudioInputSource::status,&app,[](QString status){fprintf(stderr,"STATUS %s\n",status.toUtf8().constData());});
    QObject::connect(input,&AudioInputSource::streamDescriptorChanged,&app,[](AudioStreamDescriptor d){if(d.isValid())fprintf(stderr,"FORMAT %d %d %d\n",d.sample_rate_hz,d.sample_size_bits,d.channel_count);});
    qRegisterMetaType<AudioStreamDescriptor>();
    audioThread.start();
    auto mode=selected.preferredFormat().channelCount()==1?AudioDevice::Mono:(channel==1?AudioDevice::Right:AudioDevice::Left);
    QMetaObject::invokeMethod(input,[=]{input->start(selected,2048,sink,factor,mode);},Qt::QueuedConnection);
    QSocketNotifier stop(STDIN_FILENO,QSocketNotifier::Read);
    QObject::connect(&stop,&QSocketNotifier::activated,&app,[&]{char buffer[32];::read(STDIN_FILENO,buffer,sizeof(buffer));app.quit();});
    int result=app.exec();
    QMetaObject::invokeMethod(input,[=]{input->stop();},Qt::BlockingQueuedConnection);
    audioThread.quit();audioThread.wait();return result;
}
