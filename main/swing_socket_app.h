#ifndef __SWING_SOCKET_APP_H
#define __SWING_SOCKET_APP_H

#define SOCKET_CLIENT_MAX 1

#define PORT 54321 
#define TIME_CONNET_MAX 600000


void Soket_task(void *pvParameter);
void socket_server_close();
void Soket_monitor_task(void *pvParameter);

int Socketcontrol_out(char *buf,int size);

extern char SocketBuf[1025];
extern int SocketBufSize;

#define printf2socket(format, ... )  SocketBufSize = snprintf(SocketBuf,1024,format, ##__VA_ARGS__); Socketcontrol_out(Socketcontrol_out,SocketBufSize)
#endif