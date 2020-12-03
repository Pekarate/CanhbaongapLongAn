#ifndef __SERVER_H
#define __SERVER_H
#include "esp_http_client.h"
int esp_server_get_config(esp_http_client_handle_t client);
int Network_Send_StationData(esp_http_client_handle_t client);
#endif
