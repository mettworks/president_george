// TODO, extern?? Bullshit!
void display_convert_number(unsigned char number,unsigned char segmente[6]);
void display_convert_letter(char letter,unsigned char segmente[6]);
void display_write_modus(unsigned char modus);
void display_write_mod(unsigned char mod);
void display_write_frequenz(unsigned long freq2write);
void display_write_channel(unsigned char channel);
void display_init(void);
void display_send(void);
void display_write_meter(uint32_t value);
void display_clear(void);
void display_write_function(void);
void display_memory_swap(void);
void display_ctcss(unsigned int ctcss_disp);
void display_rpt(unsigned int rpt_disp);
void display_beep(unsigned int beep_disp);
void display_echo(unsigned int echo_disp);
void display_write_vfo(char display_vfo);
void display_write_step(unsigned int step2);
