#include "Ota.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "errno.h"
#include "define.h"

#include "lwip/err.h"
#include "lwip/sockets.h"

#define BUFFSIZE 2048


#if USER_DEBUG
    static const char *TAG = "[OTA]";
#endif

#define HASH_LEN 32 /* SHA-256 digest length */

extern _StationConfig StConfig;
/*an ota data write buffer ready to write to the flash*/

void Ota_Init(void)
{
	printf("printf from Ota.c\n");
}
int Ota_start(Http_ota_server_t server)
{
	int res=0;
	esp_http_client_config_t config = {
	        .url = server.Url,
//	        .event_handler = _http_event_handler,
	    };
	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == NULL) {
	        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
	        return -1;
	}
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client,"APIKEY",(const char *)server.ApiKey);
	esp_http_client_set_header(client,"Content-Type","application/json-patch+json");
	char body[BUFFSIZE+1];
	int len=0;
	len = sprintf(body,"{\r\n\"FolderName\": \"%s\",\r\n\"FileName\": \"%s\"\r\n}",server.foder,server.filename);

	if(esp_http_client_open(client,len)== ESP_OK)
	{
		if(esp_http_client_write(client,(const char *)body,len)== len)
		{
			if(esp_http_client_fetch_headers(client))
			{
				int status_code = esp_http_client_get_status_code(client);
				if(status_code==200)
				{
					uint32_t tot =0;
					uint32_t content_length =esp_http_client_get_content_length(client);
					#if USER_DEBUG
						ESP_LOGI(TAG, "http ok,content-length:%d",content_length);
					#endif
					do{
						len = esp_http_client_read(client,body,2*BUFFSIZE);
						if(len >0)
						{
							tot+=len;
							#if USER_DEBUG
								ESP_LOGI(TAG, "recv: %d byte,total: %d byte",len,tot);
							#endif
						}
					}while(len>0);
					//body[tot]=0;
					if(tot == content_length)
					{
						res= content_length;
					}
				}
				else
				{
					#if USER_DEBUG
						ESP_LOGE(TAG, "http error:%d",status_code);
					#endif
				}

			}
			else
			{
				#if USER_DEBUG
					ESP_LOGE(TAG, "fetch headers error");
				#endif
				res=-1;
			}

		}
		else
		{
			#if USER_DEBUG
				ESP_LOGE(TAG, "Write error");
			#endif
			res=-2;
		}
	}
	else
	{
		#if USER_DEBUG
			ESP_LOGE(TAG, "can't open connection");
		#endif
		res=-3;
	}
	esp_http_client_close(client);
	esp_http_client_cleanup(client);
	return res;
}
int ota_update(Http_ota_server_t server)
{
    esp_err_t err= ESP_OK;
    char Buff_t[BUFFSIZE+1] = {0};
    int len=0;
    int res =0;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    ESP_LOGI(TAG, "Starting OTA example...");
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);
    esp_http_client_config_t config = {
        .url = server.Url,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        return -1;
    }
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client,"APIKEY",(const char *)server.ApiKey);
    esp_http_client_set_header(client,"Content-Type","application/json-patch+json");

    len = sprintf(Buff_t,"{\r\n\"FolderName\": \"%s\",\r\n\"FileName\": \"%s\"\r\n}",server.foder,server.filename);

    if(esp_http_client_open(client, len)==ESP_OK)
    {
    	if(esp_http_client_write(client,(const char *)Buff_t,len)== len)
    	{
    	    if(esp_http_client_fetch_headers(client))
    	    {
				int status_code = esp_http_client_get_status_code(client);
				int Content_length = esp_http_client_get_content_length(client);
				if((status_code ==200)&&(Content_length>0))
				{
					#if OTA_DEBUG
						ESP_LOGI(TAG, "http ok,content-length:%d",Content_length);
					#endif
					update_partition = esp_ota_get_next_update_partition(NULL);
					ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
							 update_partition->subtype, update_partition->address);
					assert(update_partition != NULL);

					int binary_file_length = 0;
					/*deal with all receive packet*/
					bool image_header_was_checked = false;
					int data_read=0;
					while (1) {
						data_read = esp_http_client_read(client, Buff_t, BUFFSIZE);
						if (data_read < 0) {
							ESP_LOGE(TAG, "Error: SSL data read error");
							res=-1;
							break;
						} else if (data_read > 0) {
							if (image_header_was_checked == false) {
								esp_app_desc_t new_app_info;
								if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
									// check current version with downloading
									memcpy(&new_app_info, &Buff_t[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
									ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

									esp_app_desc_t running_app_info;
									if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
										ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
									}

									const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
									esp_app_desc_t invalid_app_info;
									if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
										ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
									}

									// check current version with last invalid partition
									if (last_invalid_app != NULL) {
										if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
											ESP_LOGW(TAG, "New version is the same as invalid version.");
											ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
											ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
											res=-2;
											break;
										}
									}
									if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
										ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
										res=-3;
										break;
									}

									image_header_was_checked = true;

									err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
									if (err != ESP_OK) {
										ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
										res=-4;
										break;
									}
									ESP_LOGI(TAG, "esp_ota_begin succeeded");
								} else {
									ESP_LOGE(TAG, "received package is not fit len");
									res=-5;
									break;
								}
							}
							err = esp_ota_write( update_handle, (const void *)Buff_t, data_read);
							if (err != ESP_OK) {
								res=-6;
								break;
							}
							binary_file_length += data_read;
							ESP_LOGI(TAG, "Updating : %d %c",binary_file_length*100/Content_length,'%');
//							ESP_LOGI(TAG, "Written image length %d", binary_file_length);
						} else if (data_read == 0) {
							ESP_LOGI(TAG, "recv total %d", binary_file_length);
							ESP_LOGI(TAG, "Connection closed,all data received");
							res= binary_file_length;
							break;
						}
					}
					if(res>0)
					{
						ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);
						if ((err = esp_ota_end(update_handle)) == ESP_OK) {
							if (esp_ota_set_boot_partition(update_partition)==ESP_OK)
							{
								ESP_LOGI(TAG, "Prepare to restart system!");
								vTaskDelay(2000);
								esp_restart();
							}
							else {
								ESP_LOGE(TAG, "esp_ota_set_boreadot_partition failed (%s)!", esp_err_to_name(err));
								res = -7;
							}
						}
						else
						{
							ESP_LOGE(TAG, "esp_ota_end failed (%s)!",esp_err_to_name(err));
							res = -8;
						}
					}
				}
				else
				{
					#if OTA_DEBUG
						ESP_LOGE(TAG, "http error:%d",status_code);
					#endif
					res = -9;
				}
    	    } //end fetch header
    	    else
    	    {
					#if OTA_DEBUG
						ESP_LOGE(TAG, "http error fetch header");
					#endif
					res = -9;
    	    }
    	}
    	else
    	{
			#if OTA_DEBUG
				ESP_LOGE(TAG, "Write error");
			#endif
			res = -10;
    	}
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        res = -11;
    }
    esp_http_client_cleanup(client);
    return res;
}

