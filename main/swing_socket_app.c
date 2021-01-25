
#include "define.h"
#include "swing_socket_app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"


#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "stdint.h"
#include "driver/uart.h"
static const char *TAG = "SOCKET";

SemaphoreHandle_t Mutex_Sclient_cnt = NULL;
uint8_t Sclient_cnt= 0;
extern int System_handles(char *cmd,char * output);
int socketrunning = -1;

char SocketBuf[1025];
int SocketBufSize = 0;

//#define Printf_socket(format, ... )  SocketBufSize = snprintf(SocketBuf,1024,format, ##__VA_ARGS__); Socketcontrol_out(Socketcontrol_out,SocketBufSize)

int Socketcontrol_out(char *buf,int size)
{
	if(socketrunning!= -1)
	{
		*(buf + size) = '\n';
		*(buf + size+1) = 0;
		return send(socketrunning , (uint8_t *)buf, size+1 , 0 );
	}
	return -1;

}
static void client_socket_thread(void *arg)
{

	int accepted_socket = *(int*)arg;
	ESP_LOGI(TAG,"socket %d connected",accepted_socket); 
	send(accepted_socket , (uint8_t *)"hello form 54321", 17 , 0 );
	int ret,tmp;
	uint8_t pbuffer[1024];
	char out[1024];	
	int size;
	uint64_t st = Get_mili() +TIME_CONNET_MAX;
	socketrunning = accepted_socket;
	while(Get_mili() < st)
	{
		tmp =recv(accepted_socket, pbuffer, 1024, MSG_PEEK | MSG_DONTWAIT);
		if(tmp ==0)
		{
			break; // disconnect or network error;s
		}
		else
		{
			ret = recv(accepted_socket,pbuffer,1024,MSG_DONTWAIT);
			if(ret >0)
			{
				pbuffer[ret] =0;
				
				ESP_LOGW(TAG,"recv %d byte from socket: %d => %s",ret,accepted_socket,pbuffer); 
				size = System_handles((char *)pbuffer,out);
				send(accepted_socket , (uint8_t *)out, size , 0 );
				st = Get_mili() +TIME_CONNET_MAX;
			}
		}
		vTaskDelay(10);
	}
	if( xSemaphoreTake( Mutex_Sclient_cnt, portMAX_DELAY ) == pdTRUE )
	{
		Sclient_cnt--;
		xSemaphoreGive( Mutex_Sclient_cnt);
	}
	ESP_LOGW(TAG,"socket % disconnect or timeout ",accepted_socket); 
	socketrunning = -1;
	close(accepted_socket);
	vTaskDelete(NULL);
}

void socket_server_close()
{
	ESP_LOGE(TAG,"socket_server_close: %d",close(socketrunning)); 
}
void Soket_task(void *pvParameter)
{
	Mutex_Sclient_cnt = xSemaphoreCreateMutex();
	if( Mutex_Sclient_cnt == NULL )
    {
		ESP_LOGE(TAG,"xSemaphoreCreateMutex fail=> reset affter 5s"); 
		vTaskDelay(5000/portTICK_PERIOD_MS);
		esp_restart();
	}
	// while(!Wifi_is_connect())
	// {
	// 	vTaskDelay(1000/portTICK_PERIOD_MS);
	// 	ESP_LOGI(TAG,"------------"); 
	// }
	ESP_LOGI(TAG,"-------------SOCKET STARTED---------"); 
	int new_socket; 
    struct sockaddr_in address; 
    //int opt = 1; 
    int addrlen = sizeof(address); 
	int server_fd;
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        ESP_LOGE(TAG,"socket failed"); 
		vTaskDelete(NULL);
    }
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
       
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        ESP_LOGE(TAG,"bind failed"); 
		vTaskDelete(NULL);
    } 
    if (listen(server_fd, 3) < 0) 
    { 
        ESP_LOGE(TAG,"listen fail"); 
		vTaskDelete(NULL);
    } 
	ESP_LOGI(TAG,"-------------SOCKET STARTED---------"); 
	while (1)
	{
		    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0) 
			{ 
				ESP_LOGE(TAG,"accept fail"); 
				// perror("accept"); 
				// exit(EXIT_FAILURE); 
				break;
			} 
			else{
				if( xSemaphoreTake( Mutex_Sclient_cnt, portMAX_DELAY ) == pdTRUE )
				{
					
					if(Sclient_cnt < SOCKET_CLIENT_MAX)
					{
						Sclient_cnt ++;
						xTaskCreatePinnedToCore(client_socket_thread,"client_socket_thread",8192,(void*)&new_socket, 1,NULL,1 );
					}
					else
					{
						send(new_socket , "max client\r\n" , strlen("max client\r\n") , 0 );
						
						ESP_LOGW(TAG,"max client"); 
						vTaskDelay(200);
						closesocket(new_socket);
					}
					xSemaphoreGive( Mutex_Sclient_cnt);
				}
			}

			// valread = read( new_socket , buffer, 512); 
			// printf("%s\n",buffer ); 
			// if(strstr(buffer,"AT+ID\r\n"))
			// {
			// 	send(new_socket , ESP_ID_CHAR , strlen(ESP_ID_CHAR) , 0 ); 
			// 	send(new_socket , "\r\n" , 2 , 0 ); 
			// }
			// else
			// {
				
			// }
			
	}
	vTaskDelete(NULL);

}
// static void client_socket_thread(void *arg)
// {

// 	int accepted_socket = *(int*)arg;
// 	ESP_LOGI(TAG,"socket %d connected",accepted_socket); 
// 	int ret,tmp;
// 	uint8_t pbuffer[1024];
// 	char out[1024];
// 	int size;
// 	uint64_t st = Get_mili() +TIME_CONNET_MAX;
// 	while(Get_mili() < st)
// 	{
// 		tmp =recv(accepted_socket, pbuffer, 1024, MSG_PEEK | MSG_DONTWAIT);
// 		if(tmp ==0)
// 		{
// 			break; // disconnect or network error;s
// 		}
// 		else
// 		{
// 			ret = recv(accepted_socket,pbuffer,1024,MSG_DONTWAIT);
// 			if(ret >0)
// 			{
// 				pbuffer[ret] =0;
				
// 				ESP_LOGW(TAG,"recv %d byte from socket: %d => %s",ret,accepted_socket,pbuffer); 
// 				size = System_handles((char *)pbuffer,out);
// 				send(accepted_socket , (uint8_t *)out, size , 0 );
// 				st = Get_mili() +TIME_CONNET_MAX;
// 			}
// 		}
		

// 		vTaskDelay(1);
// 	}
// 	if( xSemaphoreTake( Mutex_Sclient_cnt, portMAX_DELAY ) == pdTRUE )
// 	{
// 		Sclient_cnt--;
// 		xSemaphoreGive( Mutex_Sclient_cnt);
// 	}
// 	ESP_LOGW(TAG,"socket % disconnect or timeout ",accepted_socket); 
// 	close(accepted_socket);
// 	vTaskDelete(NULL);
// }
void Soket_monitor_task(void *pvParameter)
{
	Mutex_Sclient_cnt = xSemaphoreCreateMutex();
	if( Mutex_Sclient_cnt == NULL )
    {
		ESP_LOGE(TAG,"xSemaphoreCreateMutex fail=> reset affter 5s"); 
		vTaskDelay(5000/portTICK_PERIOD_MS);
		esp_restart();
	}
	// while(!Wifi_is_connect())
	// {
	// 	vTaskDelay(1000/portTICK_PERIOD_MS);
	// 	ESP_LOGI(TAG,"------------"); 
	// }
	ESP_LOGI(TAG,"-------------SOCKET MONITOR STARTED---------"); 
	int Monitor_socket; 
    struct sockaddr_in address; 
    //int opt = 1; 
    int addrlen = sizeof(address); 
	int server_fdmonitor;
    // Creating socket file descriptor 
    if ((server_fdmonitor = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        ESP_LOGE(TAG,"socket failed"); 
		vTaskDelete(NULL);
    }
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(12345);
       
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fdmonitor, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        ESP_LOGE(TAG,"bind failed"); 
		vTaskDelete(NULL);
    } 
    if (listen(server_fdmonitor, 3) < 0) 
    { 
        ESP_LOGE(TAG,"listen fail"); 
		vTaskDelete(NULL);
    } 
	ESP_LOGI(TAG,"-------------SOCKET MONITOR STARTED---------"); 
	while (1)
	{
		    if ((Monitor_socket = accept(server_fdmonitor, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0) 
			{ 
				ESP_LOGE(TAG,"accept fail"); 
				// perror("accept"); 
				// exit(EXIT_FAILURE); 
				break;
			} 
			else{
				int recved;
				char buf[1024];
				send(Monitor_socket , (uint8_t *)"connect to port 12345", 22 , 0 );
				while(1)
				{
					int tmp =recv(Monitor_socket, buf, 1024, MSG_PEEK | MSG_DONTWAIT);
					if(tmp ==0)
					{
						break; // disconnect or network error;s
					}
					else
					{
						int ret = recv(Monitor_socket,buf,1024,MSG_DONTWAIT);
						if(ret >0)
						{
							ESP_LOGW(TAG,"monitor socket recv : %d byte ",ret);
						}
					}
					
					recved = uart_read_bytes(UART_NUM_0,(uint8_t *)buf,1024,100/portTICK_PERIOD_MS);
					if(recved >0)
					{
							send(Monitor_socket , (uint8_t *)buf, recved , 0 );
					}
					vTaskDelay(100);
				}
				ESP_LOGW(TAG,"monitor socket % disconnect or timeout ",Monitor_socket); 
				close(Monitor_socket);
			}


			// valread = read( new_socket , buffer, 512); 
			// printf("%s\n",buffer ); 
			// if(strstr(buffer,"AT+ID\r\n"))
			// {
			// 	send(new_socket , ESP_ID_CHAR , strlen(ESP_ID_CHAR) , 0 ); 
			// 	send(new_socket , "\r\n" , 2 , 0 ); 
			// }
			// else
			// {
				
			// }
			
	}
	vTaskDelete(NULL);

}