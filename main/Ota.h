#ifndef __OTA_H
#define __OTA_H

typedef struct
{
	char *Url;
	char *foder;
	char *filename;
	char *ApiKey;
}
Http_ota_server_t;

void Ota_Init(void);
int Ota_start(Http_ota_server_t server);
int ota_update(Http_ota_server_t server);
void ota_example_task(void *pvParameter);
void Ota_socket(void);
#endif
