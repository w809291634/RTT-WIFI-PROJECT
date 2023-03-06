#ifndef __RFID900M_H__
#define __RFID900M_H__

#define BLK_RFU     0
#define BLK_EPC     1
#define BLK_TID     2
#define BLK_USER    3

void RFID900MInit(void);
int RFID900MWrite(char* ap, char* epc, int mb, int sa, int dl, char *in);
int RFID900MRead(char* ap, char* epc, int mb, int sa, int dl, char *out);
int RFID900MCheckCard(char *cid);
int RFID900GetTxPower(void);
int RFID900SetTxPower(int pow);

void RFID900MCheckCardReq(void);
int RFID900MCheckCardRsp(char *cid);

void RFID900MReadReq(char* ap, char* epc, int mb, int sa, int dl);
int RFID900MReadRsp(char *out);

void RFID900MWriteReq(char* ap, char* epc, int mb, int sa, int dl, char *in);
int RFID900MWriteRsp(void);
#endif
