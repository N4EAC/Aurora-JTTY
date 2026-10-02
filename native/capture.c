// Capture directly from the selected CoreAudio device. GPL-3.0-or-later.
#include "Bridge.h"
#include <CoreAudio/CoreAudio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    AudioDeviceID device;
    AudioDeviceIOProcID proc;
    jw_capture_callback callback;
    void *context;
    int channels;
    float *samples;
    unsigned int capacity;
    atomic_int active;
} capture_state;

static OSStatus receive_audio(AudioDeviceID device, const AudioTimeStamp *now,
 const AudioBufferList *input, const AudioTimeStamp *input_time,
 AudioBufferList *output, const AudioTimeStamp *output_time, void *context) {
    (void)now; (void)input_time; (void)output_time;
    capture_state *state = context;
    // Device IO can include output buffers; never supply modulation from capture.
    if (output) for (UInt32 b=0;b<output->mNumberBuffers;b++)
        if(output->mBuffers[b].mData) memset(output->mBuffers[b].mData,0,output->mBuffers[b].mDataByteSize);
    if (!atomic_load(&state->active) || device != state->device || !input) return noErr;
    unsigned int frames=0, channels=0;
    for(UInt32 b=0;b<input->mNumberBuffers;b++) {
        const AudioBuffer *buffer=&input->mBuffers[b];
        if (!buffer->mData || !buffer->mNumberChannels) continue;
        unsigned int n=buffer->mDataByteSize/(sizeof(float)*buffer->mNumberChannels);
        if(!frames || n<frames) frames=n;
        channels+=buffer->mNumberChannels;
    }
    if(channels!=(unsigned)state->channels || !frames || frames>state->capacity) return noErr;
    unsigned int offset=0;
    for(UInt32 b=0;b<input->mNumberBuffers;b++) {
        const AudioBuffer *buffer=&input->mBuffers[b];
        if(!buffer->mData) continue;
        const float *values=buffer->mData;
        for(unsigned int f=0;f<frames;f++) for(unsigned int c=0;c<buffer->mNumberChannels;c++)
            state->samples[f*channels+offset+c]=values[f*buffer->mNumberChannels+c];
        offset+=buffer->mNumberChannels;
    }
    state->callback(state->samples,(int)(frames*channels),state->context);
    return noErr;
}

void *jw_capture_open(uint32_t device_id,const char *name,int channels,double rate,
 jw_capture_callback callback,void *context,char *error) {
    error[0]=0;
    AudioObjectPropertyAddress address={kAudioDevicePropertyStreamFormat,kAudioDevicePropertyScopeInput,kAudioObjectPropertyElementMain};
    AudioStreamBasicDescription format={0};UInt32 bytes=sizeof(format);
    OSStatus result=AudioObjectGetPropertyData(device_id,&address,0,NULL,&bytes,&format);
    if(result || format.mFormatID!=kAudioFormatLinearPCM || !(format.mFormatFlags&kAudioFormatFlagIsFloat) || format.mBitsPerChannel!=32 || format.mSampleRate!=rate) {
        snprintf(error,512,"Input %s has an unsupported or changed hardware format (%d). Refresh devices.",name,result);return NULL;
    }
    capture_state *state=calloc(1,sizeof(*state));
    if(!state){snprintf(error,512,"Cannot allocate capture");return NULL;}
    state->device=device_id;state->channels=channels;state->callback=callback;state->context=context;state->capacity=65536;
    state->samples=calloc(state->capacity*(unsigned)channels,sizeof(float));
    if(!state->samples){free(state);snprintf(error,512,"Cannot allocate input buffers");return NULL;}
    atomic_init(&state->active,1);
    result=AudioDeviceCreateIOProcID(device_id,receive_audio,state,&state->proc);
    if(!result) result=AudioDeviceStart(device_id,state->proc);
    if(result){
        snprintf(error,512,"Start direct USB capture on %s failed (%d)",name,result);
        atomic_store(&state->active,0);
        if(state->proc) AudioDeviceDestroyIOProcID(device_id,state->proc);
        free(state->samples);free(state);return NULL;
    }
    return state;
}

void jw_capture_close(void *context) {
    if(!context)return;
    capture_state *state=context;
    atomic_store(&state->active,0);
    AudioDeviceStop(state->device,state->proc);
    AudioDeviceDestroyIOProcID(state->device,state->proc);
    free(state->samples);free(state);
}
