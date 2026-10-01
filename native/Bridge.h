#include <stdint.h>
void jw_reset(void);
int jw_encode(const char *text, int profile, float frequency, int16_t *samples, int capacity, char *normalized);
int jw_feed(const int16_t *samples, int count, float frequency, float tolerance);
int jw_pop(int64_t *identifier, float *frequency, double *seconds, int *complete, char *text);
void *jw_radio_open(int model, const char *path, const char *ptt_path, int baud, int stopbits, int ptt_method, char *error);
int jw_radio_state(void *radio, double *frequency, int *ptt, char *mode);
int jw_radio_frequency(void *radio, double frequency);
int jw_radio_mode(void *radio, int data_mode);
int jw_radio_ptt(void *radio, int on, double limit_seconds);
int jw_radio_close(void *radio);
int jw_radio_watchdog(void *radio);
const char *jw_radio_error(int code);

typedef void (*jw_capture_callback)(const float *samples, int count, void *context);
void *jw_capture_open(uint32_t device_id, const char *name, int channels, double rate, jw_capture_callback callback, void *context, char *error);
void jw_capture_close(void *capture);
