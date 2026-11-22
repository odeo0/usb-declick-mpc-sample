#include "../src/pcmfix_core.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int test_clean(void){
    pcmfix_state s; pcmfix_reset(&s);
    int32_t in[2],out[2];int32_t last[2]={0,0};
    for(int i=0;i<44100*3;i++){
        in[0]=(int32_t)((i%2000)*12345-12345000);
        in[1]=(int32_t)((i%1700)*11000-9250000);
        pcmfix_step(&s,in,out);
        if(i>0 && (out[0]!=last[0]||out[1]!=last[1]))return 1;
        last[0]=in[0];last[1]=in[1];
    }
    if(s.corrections!=0||s.instant_corrections!=0)return 2;
    printf("PASS clean: 0 changes other than 1-frame time shift\n");return 0;
}
static int test_click(void){
    pcmfix_state s; pcmfix_reset(&s);
    int32_t in[2],out[2];int corrected=0,missed=0;
    for(int i=0;i<44100*3;i++){
        int hit=(i%441==137);
        in[0]=hit?1600000000:0;in[1]=hit?1600000000:0;
        pcmfix_step(&s,in,out);
        if(i>1 && (i-1)%441==137){
            if(out[0]==0&&out[1]==0)corrected++;
            else missed++;
        }
    }
    if(corrected<250||missed>10)return 4;
    printf("PASS periodic impulses: repaired=%d missed=%d phase=%d\n",corrected,missed,s.locked_phase);return 0;
}
int main(void){int x=test_clean(),y=test_click();return x?x:y;}
