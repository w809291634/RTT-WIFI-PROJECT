#ifndef __GPS_H
#define __GPS_H 

extern unsigned char start_cmd[];       //开始测量命令
extern unsigned char gps_flag[];                                         //停止操作指令
extern unsigned char GPS_cmd[200];       //复位指令

extern unsigned int hh = 0;
extern unsigned int mm = 0;
extern unsigned int ss = 0;
extern double Lat_v,Lon_v;
extern int Lat_deg,Lon_deg;
extern double Lat_mi,Lon_mi;
extern unsigned char sn;
extern unsigned char ew;

int gps_get(char *lat, char *lng);
void gps_init(void);

#endif
