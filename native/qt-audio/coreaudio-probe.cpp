// GPL-3.0-or-later. Receive-only direct HAL probe; no serial, PTT or format changes.
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <atomic>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>
#include <unistd.h>
static std::atomic<unsigned long> calls{0};
static std::atomic<double> peak{0};
static AudioStreamBasicDescription format{};
static OSStatus capture(AudioDeviceID,const AudioTimeStamp*,const AudioBufferList* input,const AudioTimeStamp*,AudioBufferList* output,const AudioTimeStamp*,void*) {
  if(output) for(UInt32 i=0;i<output->mNumberBuffers;++i)
    if(output->mBuffers[i].mData) std::memset(output->mBuffers[i].mData,0,output->mBuffers[i].mDataByteSize);
  double value=0;
  if(input) for(UInt32 i=0;i<input->mNumberBuffers;++i) {
    auto const& b=input->mBuffers[i];if(!b.mData)continue;
    if((format.mFormatFlags&kAudioFormatFlagIsFloat) && format.mBitsPerChannel==32) {
      auto data=static_cast<float const*>(b.mData);
      for(UInt32 j=0;j<b.mDataByteSize/sizeof(float);++j)value=std::fmax(value,std::fabs(data[j]));
    } else if((format.mFormatFlags&kAudioFormatFlagIsSignedInteger)&&format.mBitsPerChannel==16) {
      auto data=static_cast<short const*>(b.mData);
      for(UInt32 j=0;j<b.mDataByteSize/sizeof(short);++j)value=std::fmax(value,std::fabs(double(data[j])/32768.));
    }
  }
  if(value>peak.load())peak.store(value);++calls;return noErr;
}
static bool property(AudioDeviceID id,AudioObjectPropertySelector selector,AudioObjectPropertyScope scope,void*data,UInt32& size) {
  AudioObjectPropertyAddress a{selector,scope,kAudioObjectPropertyElementMain};
  return AudioObjectGetPropertyData(id,&a,0,nullptr,&size,data)==noErr;
}
static bool named(AudioDeviceID id,char const* expected) {
  CFStringRef value=nullptr;UInt32 size=sizeof(value);
  if(!property(id,kAudioObjectPropertyName,kAudioObjectPropertyScopeGlobal,&value,size)||!value)return false;
  char name[1024]={};CFStringGetCString(value,name,sizeof(name),kCFStringEncodingUTF8);CFRelease(value);
  return std::strcmp(name,expected)==0;
}
int main(int argc,char**argv) {
  if(argc!=2){std::fprintf(stderr,"Specify exact input device name. Does not open CAT or transmit.\n");return 2;}
  AudioObjectPropertyAddress a{kAudioHardwarePropertyDevices,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};
  UInt32 size=0;if(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,&a,0,nullptr,&size))return 3;
  std::vector<AudioDeviceID> ids(size/sizeof(AudioDeviceID));
  if(AudioObjectGetPropertyData(kAudioObjectSystemObject,&a,0,nullptr,&size,ids.data()))return 3;
  AudioDeviceID selected=0;int matches=0;
  for(auto id:ids)if(named(id,argv[1])){selected=id;++matches;}
  if(matches!=1){std::fprintf(stderr,"Device missing or ambiguous; no default fallback.\n");return 4;}
  size=sizeof(format);a={kAudioDevicePropertyStreamFormat,kAudioObjectPropertyScopeInput,0};
  auto result=AudioObjectGetPropertyData(selected,&a,0,nullptr,&size,&format);
  if(result){std::fprintf(stderr,"Input format read failed: %d\n",int(result));return 5;}
  std::fprintf(stderr,"Direct HAL input id=%u rate=%.0f channels=%u bits=%u flags=%u\n",selected,format.mSampleRate,format.mChannelsPerFrame,format.mBitsPerChannel,format.mFormatFlags);
  AudioDeviceIOProcID proc=nullptr;
  result=AudioDeviceCreateIOProcID(selected,capture,nullptr,&proc);
  if(result){std::fprintf(stderr,"Create IOProc failed: %d\n",int(result));return 6;}
  result=AudioDeviceStart(selected,proc);
  int exitCode=result?7:0;
  if(result)std::fprintf(stderr,"Start failed: %d\n",int(result));
  else for(int second=1;second<=10;++second) {
    sleep(1);UInt32 alive=0;size=sizeof(alive);
    bool ok=property(selected,kAudioDevicePropertyDeviceIsAlive,kAudioObjectPropertyScopeGlobal,&alive,size);
    std::fprintf(stderr,"second=%d alive=%u callbacks=%lu peak=%.6f\n",second,alive,calls.load(),peak.exchange(0));
    if(!ok||!alive||!named(selected,argv[1])){exitCode=8;break;}
  }
  AudioDeviceStop(selected,proc);AudioDeviceDestroyIOProcID(selected,proc);
  if(!exitCode&&!calls.load())exitCode=9;
  return exitCode;
}
