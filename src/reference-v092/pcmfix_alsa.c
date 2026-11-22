/* Akai MPC Sample 1.3.0 — ALSA PCM capture repair, experimental.
   Own work, does not redistribute or link against proprietary mod code.
   Capture-only, 44.1kHz stereo, one-sample lookahead, phase tracking. */
#include "pcmfix_core.h"
#include <stdint.h>
#include <stddef.h>

/* Use libc/alsa interfaces from the device runtime, no cross-sysroot needed. */
extern void *dlsym(void *,const char *);
extern int open(const char *,int,...);
extern long write(int,const void*,unsigned long);
extern int close(int);
extern char *getenv(const char *);
extern long time(long *);
extern int snprintf(char*,unsigned long,const char*,...);
#define NEXT ((void*)(intptr_t)-1)
#define O_WRONLY 1
#define O_CREAT  64
#define O_APPEND 1024
#define LOG_PATH "/run/mpc-pcmfix-v092-fallback.log"
#define MAX_STREAMS 16
#define ERR_FMT -1
/* SNDRV_PCM_FORMAT constants from Linux uapi sound/asound.h */
#define F_S16 2
#define F_S24 6
#define F_S32 10
#define F_FLOAT 14
#define F_S24_3 32

typedef long (*rdi_fn)(void *,void *,unsigned long);
typedef long (*rdn_fn)(void *,void **,unsigned long);
typedef int (*open_fn)(void **,const char *,int,int);
typedef int (*params_fn)(void*,void*);
typedef int (*close_fn)(void*);
typedef int (*prepare_fn)(void*);
typedef int (*getfmt_fn)(const void*,int*);
typedef int (*getch_fn)(const void*,unsigned int*);
typedef int (*getrate_fn)(const void*,unsigned int*,int*);

static rdi_fn old_readi;
static rdn_fn old_readn;
static open_fn old_open;
static params_fn old_params;
static close_fn old_close;
static prepare_fn old_prepare;
static getfmt_fn old_getfmt;
static getch_fn old_getch;
static getrate_fn old_getrate;

struct capture {
  void *pcm;
  int is_capture;
  int usable;
  int fmt;
  int bytes;
  unsigned channels;
  unsigned rate;
  int started;
  int lock_logged;
  int usb_target;
  uint64_t last_status_frames;
  pcmfix_state dsp;
};
static struct capture cap[MAX_STREAMS];

static void logmsg(const char *msg){
  unsigned long sz=0;while(msg[sz]&&sz<1020)sz++;
  /* stderr captured by launcher as independent diagnostic path */
  (void)write(2,msg,sz);(void)write(2,"\n",1);
  const char *path=getenv("MPC_PCMFIX_LOG");
  if(!path || !path[0])path=LOG_PATH;
  int fd=open(path,O_WRONLY|O_CREAT|O_APPEND,0644);
  if(fd<0){(void)write(2,"PCM_FIX_LOG_OPEN_FAILED\n",24);return;}
  (void)write(fd,msg,sz);(void)write(fd,"\n",1);(void)close(fd);
}

static void logcap(const char *what,const struct capture *c){
  char b[360];
  (void)snprintf(b,sizeof b,"%s epoch=%ld pcm=%p usb=%d rate=%u ch=%u fmt=%d ok=%d frames=%llu phase=%d phase_fixed=%llu instant_fixed=%llu moves=%u relocks=%u events=%u",
    what,time(0),c->pcm,c->usb_target,c->rate,c->channels,c->fmt,c->usable,
    (unsigned long long)c->dsp.nframes,c->dsp.locked_phase,
    (unsigned long long)c->dsp.tracked_corrections,(unsigned long long)c->dsp.instant_corrections,
    c->dsp.phase_moves,c->dsp.relocks,c->dsp.total_events);
  logmsg(b);
}
static struct capture *findcap(void *pcm){
  for(int i=0;i<MAX_STREAMS;i++) if(cap[i].pcm==pcm && pcm)return &cap[i];
  return 0;
}
static struct capture *newcap(void *pcm,int is_capture){
  struct capture *slot=findcap(pcm);
  if(!slot){for(int i=0;i<MAX_STREAMS;i++)if(!cap[i].pcm){slot=&cap[i];break;}}
  if(!slot)return 0;
  slot->pcm=pcm;slot->is_capture=is_capture;slot->usable=0;
  slot->fmt=ERR_FMT;slot->rate=0;slot->channels=0;slot->bytes=0;
  slot->started=0;slot->lock_logged=0;slot->usb_target=0;slot->last_status_frames=0;pcmfix_reset(&slot->dsp);
  return slot;
}
static void resolve_open(void){if(!old_open)old_open=(open_fn)dlsym(NEXT,"snd_pcm_open");}
static void resolve_params(void){
  if(!old_params)old_params=(params_fn)dlsym(NEXT,"snd_pcm_hw_params");
  if(!old_getfmt)old_getfmt=(getfmt_fn)dlsym(NEXT,"snd_pcm_hw_params_get_format");
  if(!old_getch)old_getch=(getch_fn)dlsym(NEXT,"snd_pcm_hw_params_get_channels");
  if(!old_getrate)old_getrate=(getrate_fn)dlsym(NEXT,"snd_pcm_hw_params_get_rate");
}
int snd_pcm_open(void **pcm,const char *name,int stream,int mode){
  resolve_open();if(!old_open)return -38;
  int rc=old_open(pcm,name,stream,mode);
  if(rc==0 && pcm && *pcm && stream==1){
    struct capture *s=newcap(*pcm,1);
    if(s){
      /* Only patch the named USB gadget capture, not analog/other inputs. */
      const char *needle="UAC2Gadget";
      if(name){for(const char *q=name;*q;q++){
        const char *a=q,*b=needle;
        while(*a&&*b&&*a==*b){a++;b++;}
        if(!*b){s->usb_target=1;break;}
      }}
      char b[240];(void)snprintf(b,sizeof b,"ALSA_CAPTURE_OPEN name=%.150s pcm=%p usb=%d",name?name:"(null)",*pcm,s->usb_target);logmsg(b);
    }
  }
  return rc;
}
int snd_pcm_hw_params(void *pcm,void *params){
  resolve_params();if(!old_params)return -38;
  int rc=old_params(pcm,params);
  struct capture *s=findcap(pcm);
  if(rc==0 && s && s->is_capture){
    int fmt=-1,direction=0;unsigned rate=0,ch=0;
    if(old_getfmt)old_getfmt(params,&fmt);
    if(old_getrate)old_getrate(params,&rate,&direction);
    if(old_getch)old_getch(params,&ch);
    s->fmt=fmt;s->rate=rate;s->channels=ch;
    s->bytes=(fmt==F_S16)?2:(fmt==F_S24_3)?3:(fmt==F_S24||fmt==F_S32||fmt==F_FLOAT)?4:0;
    s->usable=s->usb_target && rate==44100 && ch==2 && s->bytes>0;
    s->started=0;s->lock_logged=0;s->last_status_frames=0;pcmfix_reset(&s->dsp);
    logcap("ALSA_CAPTURE_PARAMS",s);
  }
  return rc;
}
/* Read native PCM sample into signed Q31, then safely restore native encoding. */
static int32_t unpack(const unsigned char *p,int fmt){
  if(fmt==F_S16){int16_t x=(int16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));return (int32_t)x*65536;}
  if(fmt==F_S24_3){int32_t x=(int32_t)((uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16));if(x&0x800000)x|=(int32_t)0xff000000;return x*256;}
  if(fmt==F_S24){int32_t x=(int32_t)((uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16));if(x&0x800000)x|=(int32_t)0xff000000;return x*256;}
  if(fmt==F_S32){uint32_t v=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return (int32_t)v;}
  if(fmt==F_FLOAT){union {uint32_t bits;float f;}u;
    u.bits=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
    if(!(u.f==u.f))return 0;
    if(u.f>=1.)return INT32_MAX;
    if(u.f<=-1.)return INT32_MIN;
    return (int32_t)(u.f*2147483647.f);
  }
  return 0;
}
static void pack(unsigned char *p,int fmt,int32_t x){
  if(fmt==F_FLOAT){union {uint32_t bits;float f;}u;u.f=(float)((double)x/2147483647.0);
    p[0]=u.bits&255;p[1]=(u.bits>>8)&255;p[2]=(u.bits>>16)&255;p[3]=(u.bits>>24)&255;return;}
  uint32_t v=(uint32_t)x;
  if(fmt==F_S16)v=(uint16_t)(x/65536);
  if(fmt==F_S24||fmt==F_S24_3)v=(uint32_t)(x/256);
  p[0]=v&255;p[1]=(v>>8)&255;
  if(fmt!=F_S16){p[2]=(v>>16)&255;if(fmt!=F_S24_3)p[3]=(v>>24)&255;}
}
static void process(struct capture *s,void *buffer,void **buffers,unsigned long frames,int planar){
  if(!s||!s->usable||frames>2000000)return;
  for(unsigned long i=0;i<frames;i++){
    unsigned char *p0,*p1;
    if(planar){if(!buffers||!buffers[0]||!buffers[1])return;
      p0=(unsigned char*)buffers[0]+i*s->bytes;
      p1=(unsigned char*)buffers[1]+i*s->bytes;
    } else {if(!buffer)return;
      p0=(unsigned char*)buffer+i*(s->bytes*2);
      p1=p0+s->bytes;
    }
    int32_t in[2]={unpack(p0,s->fmt),unpack(p1,s->fmt)},out[2];
    int just_locked=pcmfix_step(&s->dsp,in,out);
    pack(p0,s->fmt,out[0]);pack(p1,s->fmt,out[1]);
    if(just_locked){s->lock_logged=1;logcap("PCM_FIX_PHASE_LOCKED",s);}
  }
  if(s->dsp.nframes>=s->last_status_frames+44100*5){
     s->last_status_frames=s->dsp.nframes;
     logcap("PCM_FIX_STATS",s);
  }
}
long snd_pcm_readi(void *pcm,void *buffer,unsigned long frames){
  if(!old_readi)old_readi=(rdi_fn)dlsym(NEXT,"snd_pcm_readi");
  if(!old_readi)return -38;
  long n=old_readi(pcm,buffer,frames);
  if(n>0){struct capture *s=findcap(pcm);
    if(s&&s->usable){if(!s->started){s->started=1;logcap("PCM_FIX_READI_STARTED",s);}process(s,buffer,0,(unsigned long)n,0);}
  }
  return n;
}
long snd_pcm_readn(void *pcm,void **buffers,unsigned long frames){
  if(!old_readn)old_readn=(rdn_fn)dlsym(NEXT,"snd_pcm_readn");
  if(!old_readn)return -38;
  long n=old_readn(pcm,buffers,frames);
  if(n>0){struct capture *s=findcap(pcm);
    if(s&&s->usable){if(!s->started){s->started=1;logcap("PCM_FIX_READN_STARTED",s);}process(s,0,buffers,(unsigned long)n,1);}
  }
  return n;
}
int snd_pcm_prepare(void *pcm){
  if(!old_prepare)old_prepare=(prepare_fn)dlsym(NEXT,"snd_pcm_prepare");
  if(!old_prepare)return -38;
  int rc=old_prepare(pcm);
  struct capture *s=findcap(pcm);
  if(!rc && s && s->usable){logcap("PCM_FIX_PREPARE_RESET",s);pcmfix_reset(&s->dsp);s->lock_logged=0;s->last_status_frames=0;}
  return rc;
}
int snd_pcm_close(void *pcm){
  if(!old_close)old_close=(close_fn)dlsym(NEXT,"snd_pcm_close");
  if(!old_close)return -38;
  struct capture *s=findcap(pcm);
  if(s){logcap("PCM_FIX_CLOSE",s);s->pcm=0;}
  return old_close(pcm);
}
__attribute__((constructor))static void module_loaded(void){logmsg("PCM_FIX_V092_PRELOAD_LOADED");}
