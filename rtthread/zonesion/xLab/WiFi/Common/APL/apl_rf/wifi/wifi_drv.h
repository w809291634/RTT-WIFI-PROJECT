#ifndef __WIFI_DRV_H__
#define __WIFI_DRV_H__

extern unsigned char wifi_param_error;

void wifi_reset(void);
void wifi_Get_IMEI(char *pimei);
void wifi_disconnect_TCP(void);
int8_t wifi_TCP_Connect(char *ssid, char *key, char *ip, uint16_t port);

#endif