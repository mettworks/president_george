//#include "channels.h"

int init_geraet(void);
int tx(void);
int rx(void);
int tune(unsigned long freq2tune,unsigned int step2tune,unsigned int mod);
//void set_modulation(unsigned int mod);
int rogerbeep(void);
void off(void);
void off2(void);
void tune2channel(unsigned int ch,unsigned int cb_mod);
unsigned long ch2freq(unsigned int ch);
void set_ctcss(unsigned int ctcss_value);
void set_rpt(unsigned int rpt_value);
void set_echo(unsigned int echo_value);
void set_beep(unsigned int beep_value);
void setvfo(unsigned int vfo);
void tone(unsigned int tonefreq);
void init_tone(void);
void i2c_poti(unsigned char value);
