#ifndef __NETWORK_H
#define __NETWORK_H

#include "stdint.h"
#include "define.h"


#define NET_RECV_TIMEOUT 4      //10s
#define NET_SEND_TIMEOUT 30    //120s
#define NET_CONN_TIMEOUT 10     //10s


int Network_Connect(char *DesIP,uint16_t DesPort);
int Updata_ImageName(char *Name);    //socketnamlong
//void Network_process(void *pvParameters);
int Networl_Send_StationData(_StationData Data);
int Network_Get_Congig(_StationConfig *cf);
int Network_Send_StationData(_StationData Dt,_StationConfig cf);
void StationData_Init(_StationData *Dt );
int Network_ProcessData_FromServer(char *Data,_StationData *Dt);
int Network_ProcessData_ToServer(char *Data,_StationData Dt);

#endif