unsigned char ReadSPI(void);
void WriteSPI(unsigned int);
unsigned char ReadStatus(void);
void WriteEnable(void);
void WriteDisable(void);
void SPIWIPPolling(void);
void ByteWriteSPI(unsigned int, unsigned int, unsigned int, unsigned int);
unsigned char ByteReadSPI(unsigned char, unsigned char, unsigned char);
int save2memory(void);
void read_memory(void);
void format_memory(void);

void save_freq(unsigned long freq2tune,unsigned int vfo);
