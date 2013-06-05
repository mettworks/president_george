unsigned char ReadSPI(void);
void WriteSPI(unsigned char);
unsigned char ReadStatus(void);
void WriteEnable(void);
void WriteDisable(void);
void SPIWIPPolling(void);
void ByteWriteSPI(unsigned char, unsigned char, unsigned char, unsigned char);
unsigned char ByteReadSPI(unsigned char, unsigned char, unsigned char);