#include "pcmfix_core.h"
static int64_t absi64(int64_t x) { return x < 0 ? -x : x; }
static int phase_distance(int a, int b) {
    int d=a-b; if(d<0)d=-d; return d>(int)PCM_PERIOD/2 ? (int)PCM_PERIOD-d : d;
}
void pcmfix_reset(pcmfix_state *s) {
    unsigned char *p=(unsigned char*)s;
    for(unsigned long i=0;i<sizeof(*s);i++)p[i]=0;
    s->locked_phase=-1;
}
int pcmfix_step(pcmfix_state *s, const int32_t in[2], int32_t out[2]) {
    uint64_t k=s->nframes++;
    if(k==0){out[0]=out[1]=0;s->prev1[0]=in[0];s->prev1[1]=in[1];return 0;}
    if(k==1){out[0]=s->prev1[0];out[1]=s->prev1[1];s->prev2[0]=s->prev1[0];s->prev2[1]=s->prev1[1];s->prev1[0]=in[0];s->prev1[1]=in[1];return 0;}
    uint64_t mid=k-1;
    int phase=(int)(mid % PCM_PERIOD);
    int64_t both=INT64_MAX,around=0;
    int64_t delta[2]={0,0};
    int32_t fixed[2];
    for(unsigned ch=0;ch<2;ch++){
        int64_t a=s->prev2[ch],b=s->prev1[ch],c=in[ch];
        fixed[ch]=(int32_t)((a+c)/2);
        delta[ch]=b-(int64_t)fixed[ch];
        int64_t d=absi64(delta[ch]);
        int64_t slope=absi64(a-c);
        if(d<both) both=d;
        if(slope>around) around=slope;
    }
    int strong=(both>PCM_CALIB_THRESHOLD && both*10>around*7);
    /* Fix obvious dual-channel single-frame spikes immediately, even before
       acquiring phase or while the USB clock is drifting. Stereo same-sign
       requirement limits risk of treating unrelated musical transients as errors. */
    int impulse=(both>PCM_INSTANT_THRESHOLD && both*10>around*7 &&
                 ((delta[0]>0 && delta[1]>0)||(delta[0]<0 && delta[1]<0)));
    int just_locked=0;
    if(strong) {
        s->total_events++;
        if(s->last_hit_frame>0 && mid>s->last_hit_frame){
            uint64_t dt=mid-s->last_hit_frame;
            uint64_t periods=(dt+PCM_PERIOD/2)/PCM_PERIOD;
            int64_t offset=(int64_t)dt-(int64_t)periods*PCM_PERIOD;
            /* A few dropped/quiet blocks are okay; sub-1-frame drift per 10 ms. */
            if(periods>=1 && periods<=20 && absi64(offset)<=2+(int64_t)periods/8){
                if(s->hit_streak<UINT32_MAX)s->hit_streak++;
            }else s->hit_streak=1;
        }else s->hit_streak=1;
        s->last_hit_frame=mid;
        if(s->locked_phase<0 && s->hit_streak>=5){
            s->locked_phase=phase;
            s->last_lock_frame=mid;
            s->last_tracking_frame=mid;
            just_locked=1;
        }else if(s->locked_phase>=0 && phase_distance(phase,s->locked_phase)<=3){
            if(s->locked_phase!=phase)s->phase_moves++;
            s->locked_phase=phase;
            s->last_tracking_frame=mid;
        }
    }
    /* Avoid interpolating on stale phase after long silence or disconnected capture. */
    if(s->locked_phase>=0 && mid>s->last_tracking_frame+PCM_PERIOD*100){
        s->locked_phase=-1;
        s->hit_streak=0;
        s->relocks++;
    }
    int tracked=(s->locked_phase==phase);
    int direct=impulse && !tracked;
    int correct=tracked || direct;
    out[0]=correct?fixed[0]:s->prev1[0];
    out[1]=correct?fixed[1]:s->prev1[1];
    if(correct){
        s->corrections++;
        if(direct)s->instant_corrections++;
        else s->tracked_corrections++;
    }
    s->prev2[0]=s->prev1[0];s->prev2[1]=s->prev1[1];
    s->prev1[0]=in[0];s->prev1[1]=in[1];
    return just_locked;
}
