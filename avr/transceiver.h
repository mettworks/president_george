//#include "channels.h"

int init_geraet(void);
int tx(void);
int rx(void);
int tune(unsigned long freq2tune,unsigned int step2tune);
void set_modulation(unsigned int mod);
int rogerbeep(void);
void off(void);
void channel(unsigned int ch);
int ch2freq(unsigned int ch);
void set_ctcss(unsigned int ctcss_value);
void set_rpt(unsigned int rpt_value);
void set_echo(unsigned int echo_value);
void set_beep(unsigned int beep_value);
void setvfo(unsigned int vfo);
