// PortAudio receive bridge. GPL-3.0-or-later, 2026-10-01.
#include "Bridge.h"
#include <portaudio.h>
#include <pa_mac_core.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    PaStream *stream;
    jw_capture_callback callback;
    void *context;
    int channels;
    atomic_int active;
} capture_state;

static int receive_audio(const void *input, void *output, unsigned long frames,
                         const PaStreamCallbackTimeInfo *time, PaStreamCallbackFlags flags,
                         void *context) {
    (void)output; (void)time; (void)flags;
    capture_state *state = context;
    if (!atomic_load(&state->active)) return paComplete;
    if (input) state->callback(input, (int)(frames * state->channels), state->context);
    return paContinue;
}

static void report_error(char *error, const char *operation, PaError code) {
    const PaHostErrorInfo *host = Pa_GetLastHostErrorInfo();
    snprintf(error, 512, "%s: %s (%d)%s%s", operation, Pa_GetErrorText(code), code,
             code == paUnanticipatedHostError && host && host->errorText ? ": " : "",
             code == paUnanticipatedHostError && host && host->errorText ? host->errorText : "");
}

void *jw_capture_open(uint32_t device_id, const char *name, int channels, double rate,
                      jw_capture_callback callback, void *context, char *error) {
    error[0] = 0;
    PaError result = Pa_Initialize();
    if (result != paNoError) { report_error(error, "Initialize PortAudio", result); return NULL; }
    PaDeviceIndex selected = paNoDevice;
    int matches = 0;
    int count = Pa_GetDeviceCount();
    if (count < 0) { report_error(error, "List audio devices", count); Pa_Terminate(); return NULL; }
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels >= channels && !strcmp(info->name, name)) {
            const PaHostApiInfo *host = Pa_GetHostApiInfo(info->hostApi);
            if (host && host->type == paCoreAudio) { selected = i; ++matches; }
        }
    }
    if (matches != 1) {
        snprintf(error, 512, "Selected input device is %s. Refresh audio devices and select it again.", matches ? "ambiguous" : "unavailable");
        Pa_Terminate(); return NULL;
    }
    capture_state *state = calloc(1, sizeof(*state));
    if (!state) { snprintf(error, 512, "Cannot allocate audio capture"); Pa_Terminate(); return NULL; }
    state->callback = callback; state->context = context; state->channels = channels;
    atomic_init(&state->active, 1);
    const PaDeviceInfo *info = Pa_GetDeviceInfo(selected);
    PaStreamParameters parameters = {selected, channels, paFloat32, info->defaultHighInputLatency, NULL};
    result = Pa_OpenStream(&state->stream, &parameters, NULL, rate, 512, paNoFlag, receive_audio, state);
    if (result == paNoError && PaMacCore_GetStreamInputDevice(state->stream) != device_id) {
        snprintf(error, 512, "Input device changed. Refresh audio devices and select it again.");
        Pa_CloseStream(state->stream); free(state); Pa_Terminate(); return NULL;
    }
    if (result == paNoError) result = Pa_StartStream(state->stream);
    if (result == paNoError && PaMacCore_GetStreamInputDevice(state->stream) != device_id) {
        snprintf(error, 512, "Capture started on an unexpected device; monitoring stopped. Select the USB input again.");
        atomic_store(&state->active, 0);
        Pa_AbortStream(state->stream); Pa_CloseStream(state->stream);
        free(state); Pa_Terminate(); return NULL;
    }
    if (result != paNoError) {
        report_error(error, "Start PortAudio input", result);
        atomic_store(&state->active, 0);
        if (state->stream) Pa_CloseStream(state->stream);
        free(state); Pa_Terminate(); return NULL;
    }
    return state;
}

void jw_capture_close(void *context) {
    if (!context) return;
    capture_state *state = context;
    atomic_store(&state->active, 0);
    Pa_AbortStream(state->stream);
    // Closing waits for callbacks before their context can be released.
    Pa_CloseStream(state->stream);
    free(state);
    Pa_Terminate();
}
