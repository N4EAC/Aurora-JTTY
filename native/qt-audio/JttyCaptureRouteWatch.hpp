// GPL-3.0-or-later. CoreAudio route evidence; defaults are not the effective Qt route.
#pragma once
#include <QTimer>
#include <QStringList>
#include <functional>
#include <atomic>
#include <array>
#include <QCoreApplication>
#include <QThread>
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
  std::atomic<unsigned> eventCount_{0},eventSelector_{0};
  struct Subscription {AudioObjectID id;AudioObjectPropertyAddress address;};
  QVector<Subscription> subscriptions_;
  static OSStatus changed(AudioObjectID,UInt32 count,AudioObjectPropertyAddress const* addresses,void* context) {
    auto self=static_cast<JttyCaptureRouteWatch*>(context);
    if(count) self->eventSelector_.store(addresses[count-1].mSelector);
    self->eventCount_.fetch_add(count);return noErr;
  }
  void subscribe(AudioObjectID id,AudioObjectPropertySelector selector,AudioObjectPropertyScope scope=kAudioObjectPropertyScopeGlobal) {
    AudioObjectPropertyAddress address{selector,scope,kAudioObjectPropertyElementMain};
    auto result=AudioObjectAddPropertyListener(id,&address,changed,this);
    LOG_INFO("JTTY HAL subscribe id=" << id << " selector=" << selector << " scope=" << scope << " status=" << result);
    if(!result) subscriptions_.append({id,address});
  }
  void snapshot(char const* reason) {
    LOG_INFO("JTTY HAL snapshot reason=" << reason << " pid=" << QCoreApplication::applicationPid());
    for(auto id:devices()) {
      AudioObjectPropertyAddress a{kAudioDevicePropertyNominalSampleRate,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};
      Float64 rate=0;UInt32 size=sizeof(rate);auto rateStatus=AudioObjectGetPropertyData(id,&a,0,nullptr,&size,&rate);
      OSStatus aliveStatus=0,runningStatus=0,bufferStatus=0;
      auto alive=integerProperty(id,kAudioDevicePropertyDeviceIsAlive,&aliveStatus);
      auto running=integerProperty(id,kAudioDevicePropertyDeviceIsRunningSomewhere,&runningStatus);
      auto buffer=integerProperty(id,kAudioDevicePropertyBufferFrameSize,&bufferStatus);
      LOG_INFO("JTTY HAL device id=" << id << " name=" << stringProperty(id,kAudioObjectPropertyName).toStdString()
        << " uid=" << stringProperty(id,kAudioDevicePropertyDeviceUID).toStdString()
        << " alive=" << alive << " alive_status=" << aliveStatus << " running=" << running << " running_status=" << runningStatus
        << " rate=" << rate << " rate_status=" << rateStatus << " buffer_frames=" << buffer << " buffer_status=" << bufferStatus
        << " transport=" << integerProperty(id,kAudioDevicePropertyTransportType));
      if(id==selected_ || stringProperty(id,kAudioDevicePropertyDeviceUID)==uid_) {
        for(auto scope:{kAudioObjectPropertyScopeInput,kAudioObjectPropertyScopeOutput}) {
          AudioStreamBasicDescription format{};size=sizeof(format);
          a={kAudioDevicePropertyStreamFormat,scope,kAudioObjectPropertyElementMain};
          auto status=AudioObjectGetPropertyData(id,&a,0,nullptr,&size,&format);
          LOG_INFO("JTTY HAL native format id=" << id << " scope=" << scope << " status=" << status << " rate=" << format.mSampleRate
            << " channels=" << format.mChannelsPerFrame << " bits=" << format.mBitsPerChannel << " flags=" << format.mFormatFlags << " bytes_per_frame=" << format.mBytesPerFrame);
        }
      }
    }
  }
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
    snapshot("capture binding");
    for(auto selector:{kAudioHardwarePropertyDevices,kAudioHardwarePropertyDefaultInputDevice,kAudioHardwarePropertyDefaultOutputDevice}) subscribe(kAudioObjectSystemObject,selector);
    if(selected_) for(auto selector:std::array<AudioObjectPropertySelector,4>{kAudioDevicePropertyDeviceIsAlive,kAudioDevicePropertyNominalSampleRate,kAudioDevicePropertyBufferFrameSize,kAudioDevicePropertyDeviceIsRunningSomewhere}) subscribe(selected_,selector);
    timer_.setInterval(500);
    connect(&timer_,&QTimer::timeout,this,[this]{sample();});
  }
  ~JttyCaptureRouteWatch() override {
    timer_.stop();
    for(auto const& entry:subscriptions_) AudioObjectRemovePropertyListener(entry.id,&entry.address,changed,this);
  }
  UInt32 inputChannels() const {
    AudioStreamBasicDescription format{};UInt32 size=sizeof(format);
    AudioObjectPropertyAddress a{kAudioDevicePropertyStreamFormat,kAudioObjectPropertyScopeInput,kAudioObjectPropertyElementMain};
    return AudioObjectGetPropertyData(selected_,&a,0,nullptr,&size,&format)==noErr?format.mChannelsPerFrame:0;
  }
  UInt32 nativeBufferFrames() const {
    OSStatus status=0;auto frames=integerProperty(selected_,kAudioDevicePropertyBufferFrameSize,&status);
    return status==noErr?frames:0;
  }
  bool valid() const {return healthy_ && selected_ && !uid_.isEmpty();}
  void begin(){sample();if(valid()) timer_.start();}
  void sample() {
    auto events=eventCount_.exchange(0);
    if(events) {LOG_INFO("JTTY HAL property changes count=" << events << " last_selector=" << eventSelector_.load());snapshot("HAL property change");}
    OSStatus status=0;
    auto alive=integerProperty(selected_,kAudioDevicePropertyDeviceIsAlive,&status);
    if(status || !alive || stringProperty(selected_,kAudioDevicePropertyDeviceUID)!=uid_) {
      healthy_=false;
      timer_.stop();
      LOG_ERROR("JTTY selected capture device lost id=" << selected_ << " status=" << status << " alive=" << alive << " expected_uid=" << uid_.toStdString() << " observed_uid=" << stringProperty(selected_,kAudioDevicePropertyDeviceUID).toStdString());
      snapshot("selected device lost; inspect same UID under replacement ID");
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
