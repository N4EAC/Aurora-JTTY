/* JTTY Workbench Hamlib adapter. GPL-3.0-or-later, 2026-10-01. */
#include "Bridge.h"
#include <hamlib/rig.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
typedef struct {
    RIG *rig;
    pthread_mutex_t lock;
    pthread_t watchdog;
    int stop, keyed, expired;
    double deadline;
} Radio;
static double now(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
static void *guard(void *pointer) {
    Radio *r = pointer;
    for (;;) {
        usleep(100000);
        pthread_mutex_lock(&r->lock);
        if (r->stop) { pthread_mutex_unlock(&r->lock); break; }
        if (r->keyed && now() >= r->deadline) {
            r->expired = 1;
            if (rig_set_ptt(r->rig, RIG_VFO_CURR, RIG_PTT_OFF) == RIG_OK) r->keyed = 0;
        }
        pthread_mutex_unlock(&r->lock);
    }
    return NULL;
}
static int conf(RIG *r, const char *name, const char *value) {
    token_t token = rig_token_lookup(r, name);
    if (token == RIG_CONF_END) return -RIG_ECONF;
    return rig_set_conf(r, token, value);
}
void *jw_radio_open(int model, const char *path, const char *ptt_path, int baud, int stopbits, int ptt_method, char *error) {
    rig_set_debug(RIG_DEBUG_NONE);
    Radio *r = calloc(1, sizeof(*r));
    if (!r) { strcpy(error, "Out of memory"); return NULL; }
    r->rig = rig_init(model);
    if (!r->rig) { strcpy(error, "Unsupported Hamlib model"); free(r); return NULL; }
    int code=0; char value[32];
    if (model != 1) code=conf(r->rig, "rig_pathname", path);
    if (!code && model != 1 && model != 2) {
        snprintf(value,sizeof(value),"%d",baud); code=conf(r->rig,"serial_speed",value);
        if (!code) { snprintf(value,sizeof(value),"%d",stopbits); code=conf(r->rig,"stop_bits",value); }
        if (!code) code=conf(r->rig,"serial_handshake","None");
    }
    // Explicit PTT choice: CAT, RTS, DTR, or none (receive-only).
    r->rig->state.pttport.type.ptt = ptt_method == 1 ? RIG_PTT_SERIAL_RTS :
        ptt_method == 2 ? RIG_PTT_SERIAL_DTR : ptt_method == 3 ? RIG_PTT_NONE : RIG_PTT_RIG;
    if (model != 1 && model != 2 && (ptt_method == 1 || ptt_method == 2)) {
        snprintf(r->rig->state.pttport.pathname, sizeof(r->rig->state.pttport.pathname), "%s",ptt_path);
    }
    r->rig->state.rigport.timeout=1000;
    r->rig->state.rigport.retry=1;
    if (!code) code=rig_open(r->rig);
    if (code) { snprintf(error,256,"%s",rigerror(code)); rig_cleanup(r->rig); free(r); return NULL; }
    pthread_mutex_init(&r->lock,NULL);
    if (pthread_create(&r->watchdog,NULL,guard,r)) {
        strcpy(error,"Cannot start PTT watchdog"); rig_close(r->rig); rig_cleanup(r->rig);
        pthread_mutex_destroy(&r->lock); free(r); return NULL;
    }
    return r;
}
int jw_radio_state(void *pointer, double *frequency, int *ptt, char *mode) {
    Radio *r=pointer; if(!r) return -RIG_EINVAL;
    pthread_mutex_lock(&r->lock);
    freq_t f; ptt_t p; rmode_t m; pbwidth_t w;
    int code=rig_get_freq(r->rig,RIG_VFO_CURR,&f);
    if(!code) code=rig_get_mode(r->rig,RIG_VFO_CURR,&m,&w);
    if(!code) { *frequency=f; snprintf(mode,32,"%s",rig_strrmode(m)); }
    int pcode=rig_get_ptt(r->rig,RIG_VFO_CURR,&p);
    *ptt=pcode ? r->keyed : p != RIG_PTT_OFF;
    pthread_mutex_unlock(&r->lock); return code;
}
int jw_radio_frequency(void *pointer,double frequency) {
    Radio *r=pointer; if(!r || frequency < 100000 || frequency > 60000000) return -RIG_EINVAL;
    pthread_mutex_lock(&r->lock);
    int code=r->keyed ? -RIG_BUSBUSY : rig_set_freq(r->rig,RIG_VFO_CURR,frequency);
    pthread_mutex_unlock(&r->lock); return code;
}
int jw_radio_mode(void *pointer,int data_mode) {
    Radio *r=pointer; if(!r) return -RIG_EINVAL;
    pthread_mutex_lock(&r->lock);
    int code=r->keyed ? -RIG_BUSBUSY : rig_set_mode(r->rig,RIG_VFO_CURR,data_mode ? RIG_MODE_PKTUSB : RIG_MODE_USB,RIG_PASSBAND_NOCHANGE);
    pthread_mutex_unlock(&r->lock); return code;
}
int jw_radio_ptt(void *pointer,int on,double limit_seconds) {
    Radio *r=pointer; if(!r) return -RIG_EINVAL;
    pthread_mutex_lock(&r->lock);
    if(on && (limit_seconds <= 0 || limit_seconds > 45)) { pthread_mutex_unlock(&r->lock); return -RIG_EINVAL; }
    int code=rig_set_ptt(r->rig,RIG_VFO_CURR,on ? RIG_PTT_ON_DATA : RIG_PTT_OFF);
    if(on) {
        // Treat a failed key command as ambiguous; immediately attempt RX.
        r->keyed=1; r->deadline=now()+(code ? 0 : limit_seconds); r->expired=0;
        if(code && rig_set_ptt(r->rig,RIG_VFO_CURR,RIG_PTT_OFF)==RIG_OK) r->keyed=0;
    } else if(!code) r->keyed=0;
    pthread_mutex_unlock(&r->lock); return code;
}
int jw_radio_watchdog(void *pointer) {
    Radio *r=pointer; if(!r) return 0;
    pthread_mutex_lock(&r->lock); int expired=r->expired; pthread_mutex_unlock(&r->lock); return expired;
}
int jw_radio_close(void *pointer) {
    Radio *r=pointer; if(!r) return 0;
    pthread_mutex_lock(&r->lock);
    int code=0;
    if(r->keyed) code=rig_set_ptt(r->rig,RIG_VFO_CURR,RIG_PTT_OFF);
    if(code) { pthread_mutex_unlock(&r->lock); return code; }
    r->stop=1;
    pthread_mutex_unlock(&r->lock);
    pthread_join(r->watchdog,NULL);
    rig_close(r->rig); rig_cleanup(r->rig); pthread_mutex_destroy(&r->lock); free(r);
    return 0;
}
const char *jw_radio_error(int code) { return rigerror(code); }
