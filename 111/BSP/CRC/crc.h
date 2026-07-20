#ifndef _CRC_H
#define _CRC_H

unsigned short CRC16(unsigned char* puchMSG, unsigned char usDataLen);
int  SlaveRecDataCheck(unsigned char *revbuf,int count);




extern unsigned int  	CRC_DATA;															//CRC校验完成数据
extern unsigned char  CRC_DATA_H;														//CRC校验完成数据，高八位
extern unsigned char  CRC_DATA_L;														//CRC校验完成数据，低八位

#endif
