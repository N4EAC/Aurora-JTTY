// GPL-3.0-or-later. CoreAudio route evidence; defaults are not the effective Qt route.
#pragma once
#include <QTimer>
#include <QStringList>
#include <functional>
#ifdef Q_OS_MAC
#include <CoreAudio/CoreAudio.h>
class JttyCaptureRouteWatch final : public QObject {
  AudioDeviceID selected_=0;
  QString name_, uid_, last_;
  QTimer timer_;
  std::function<void(QString)> failed_;
  std::function<QString()> streamEvidence_;
  unsigned ticks_=0;
  bool healthy_=true;
  static QString stringProperty(AudioDeviceID id, AudioObjectPropertySelector key) {
    AudioObjectPropertyAddress a={key,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};
    CFStringRef value=nullptr; UInt32 bytes=sizeof(value);
    if(AudioObjectGetPropertyData(id,&a,0,nullptr,&bytes,&value)||!value) return {};
    char text[2048]={}; CFStringGetCString(value,text,sizeof(text),kCFStringEncodingUTF8); CFRelease(value);
    return QString::fromUtf8(text);
  }
  static UInt32 integerProperty(AudioDeviceID id, AudioObjectPropertySelector key, OSStatus *status=nullptr) {
    AudioObjectPropertyAddress a={key,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};
    UInt32 value=0,bytes=sizeof(value); auto result=AudioObjectGetPropertyData(id,&a,0,nullptr,&bytes,&value);
    if(status) *status=result; return value;
  }
  static QVector<AudioDeviceID> devices() {
    AudioObjectPropertyAddress a={kAudioHardwarePropertyDevices,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};
    UInt32 bytes=0; if(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,&a,0,nullptr,&bytes)) return {};
    QVector<AudioDeviceID> result(int(bytes/sizeof(AudioDeviceID)));
    if(AudioObjectGetPropertyData(kAudioObjectSystemObject,&a,0,nullptr,&bytes,result.data())) return {};
    return result;
  }
public:
  JttyCaptureRouteWatch(QString name,std::function<void(QString)> failed,std::function<QString()> streamEvidence={}):name_(name),failed_(failed),streamEvidence_(streamEvidence) {
    int matches=0;
    for(auto id:devices()) if(stringProperty(id,kAudioObjectPropertyName)==name){selected_=id;++matches;}
    if(matches!=1) selected_=0;
    uid_=stringProperty(selected_,kAudioDevicePropertyDeviceUID);
    LOG_INFO("JTTY capture binding selected=" << name_.toStdString() << " id=" << selected_ << " uid=" << uid_.toStdString() << " matches=" << matches);
    timer_.setInterval(500);
    connect(&timer_,&QTimer::timeout,this,[this]{sample();});
  }
  bool valid() const {return healthy_ && selected_ && !uid_.isEmpty();}
  void begin(){sample();if(valid()) timer_.start();}
  void sample() {
    OSStatus status=0;
    auto alive=integerProperty(selected_,kAudioDevicePropertyDeviceIsAlive,&status);
    if(status || !alive || stringProperty(selected_,kAudioDevicePropertyDeviceUID)!=uid_) {
      healthy_=false;
      timer_.stop();
      LOG_ERROR("JTTY selected capture device lost id=" << selected_ << " status=" << status);
      failed_(QString("Selected input %1 disconnected or changed identity. Capture stopped; refresh Radio / Audio settings. No microphone fallback is allowed.").arg(name_));
      return;
    }
    auto defaultId=integerProperty(kAudioObjectSystemObject,kAudioHardwarePropertyDefaultInputDevice);
    QStringList running;
    for(auto id:devices()) if(integerProperty(id,kAudioDevicePropertyDeviceIsRunningSomewhere)) running << QString("%1[%2]").arg(stringProperty(id,kAudioObjectPropertyName)).arg(id);
    auto evidence=QString("selected=%1[%2] alive=%3 running_any_client=%4 default=%5[%6] running_devices=%7")
      .arg(name_).arg(selected_).arg(alive).arg(integerProperty(selected_,kAudioDevicePropertyDeviceIsRunningSomewhere))
      .arg(stringProperty(defaultId,kAudioObjectPropertyName)).arg(defaultId).arg(running.join(","));
    if(streamEvidence_) evidence += " " + streamEvidence_();
    if(evidence!=last_ || (++ticks_%10)==0){ LOG_INFO("JTTY capture route evidence " << evidence.toStdString());last_=evidence; }
  }
};
#endif
