/* Native host test tool: process interleaved stereo S32_LE frames via v0.9.4 core.
   Not included in SD card payload; for repeatable WAV tests on a development host. */
#include "../src/pcmfix_core.h"
#include <stdint.h>
#include <stdio.h>
int main(void){
    pcmfix_state s;pcmfix_reset(&s);
    int32_t in[2],out[2];
    while(fread(in,sizeof(int32_t),2,stdin)==2){
        pcmfix_step(&s,in,out);
        if(fwrite(out,sizeof(int32_t),2,stdout)!=2)return 2;
    }
    fprintf(stderr,"frames=%llu corrections=%llu phase=%d instant=%llu tracked=%llu\n",
      (unsigned long long)s.nframes,(unsigned long long)s.corrections,s.locked_phase,
      (unsigned long long)s.instant_corrections,(unsigned long long)s.tracked_corrections);
    return ferror(stdin)?3:0;
}
