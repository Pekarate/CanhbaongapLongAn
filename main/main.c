#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "nvs.h"

#include "lwip/sockets.h"
#include "Ota.h"
#include "Uc_Sim.h"
#include "Ethernet.h"
#include "Kbvison_cam.h"
#include "Wifi_station.h"
#include "Sensor.h"
#include "driver/timer.h"
#include "ModbusMaster.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include <lwip/dns.h>


#include "esp_sleep.h"


#include "lwipopts.h"
#include "define.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "driver/gpio.h"
#include "Server.h"
#include "time.h"
#include "power.h"
#include "Sd_card.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"

#if USER_DEBUG
    static const char *TAG = "[MAIN]";
#endif



// public variable
Http_ota_server_t Server;
_StationConfig StConfig;
_StationData StData;
char IP_server[50];
char StartImgName[50]="Image_reset";

char *info_path 	= "/sdcard/info.txt";
char *err_path 	= "/sdcard/err.txt";
char *data_path 	= "/sdcard/data.txt";
char Sd_buff[512];
EventGroupHandle_t s_wifi_event_group;
void Nvs_init(void);
void DNS_server_init(void);
static void System_Timer(void);
void StationCongig_Init();

uint32_t Start_Post_Time =0;
uint32_t Start_Get_Time =0;
uint8_t Cnt_Http_err = 0;
uint32_t start_get_power =0;

void GPIO_Config();
void  SD_Card_Write_Data(char *Path,char *data);

void app_main(void)
{
	// uart_config_t uart_config = {
	// 		.baud_rate = 115200,
	// 		.data_bits = UART_DATA_8_BITS,
	// 		.parity = UART_PARITY_DISABLE,
	// 		.stop_bits = UART_STOP_BITS_1,
	// 		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	// };
    // if (uart_param_config(UART_NUM_0, &uart_config)) return -3;
    // if (uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)) return -4;
	// if (uart_driver_install(UART_NUM_0, 1024 * 2, 1024 * 2, 0, NULL, 0)) return -5;

	Nvs_init();

	System_Timer();
	GPIO_Config();
	StConfig.ID =9505;
	Power_Init();
	StationCongig_Init();
	if(ModbusMaster_Init() != 1)
    {
        ESP_LOGE(TAG,"Modbus Init fail");
    }
    ESP_LOGI(TAG,"Modbus Init done");

	DNS_server_init();
	Spi_Ethernet_Init();
	
	Turn_off_all_relay();
    ESP_LOGI(TAG,"reset all device 30s cpu clock: %d\n",ets_get_cpu_frequency());
    Turn_on_relay(RELAY_1);
    vTaskDelay(3000);
    Turn_off_relay(RELAY_1);


    /*---------begin int ota server------------ */
	//Get_ip_server(StConfig.Server_Url,IP_server);
    // Server.ApiKey = malloc(100);
    // sprintf(Server.ApiKey,"B423713AB2769F83889D9520C6794656");
    // Server.Url = malloc(100);
    // sprintf(Server.Url,"http://firmware.namlongtekgroup.com/GetfileUpdate");
    // Server.foder = malloc(100);
	// sprintf(Server.foder,"canhbaongap/Firmware/");
    // Server.filename = malloc(100);
	// sprintf(Server.filename,"Firmware_%d.bin",StConfig.ID);
	
	
	
    /*---------end int ota server------------ */
    int free_reg=0,Free_cur=1;
//    Ota_socket();
    int res =0;
    esp_http_client_handle_t Dataclient;
	esp_http_client_handle_t Imgclient;
	esp_http_client_config_t config = {
	        .url = StConfig.UploadUrl,
//	        .event_handler = _http_event_handler,
	  };
	Imgclient = esp_http_client_init(&config);
	if (Imgclient == NULL) {
	        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
	}
	esp_http_client_set_method(Imgclient, HTTP_METHOD_POST);
	esp_http_client_set_header(Imgclient,"Content-Type","application/x-www-form-urlencoded");
	esp_http_client_set_header(Imgclient,"APIKEY",StConfig.ApiKey);
	char b[20];
	sprintf(b,"%d",StConfig.ID);
	esp_http_client_set_header(Imgclient,"StationID",b);

	config.url = StConfig.Data_URL;
	Dataclient = esp_http_client_init(&config);
	esp_http_client_set_header(Dataclient,"Content-Type","application/x-www-form-urlencoded");
	esp_http_client_set_method(Dataclient, HTTP_METHOD_POST);
	esp_http_client_set_header(Dataclient,"APIKEY",StConfig.ApiKey);
	uint8_t try = 3;
	char Image_tmp[30];

	if (ppposInit() < 0) {
		ESP_LOGE(TAG,"ppposInit faile,module offline\n");
		sprintf(Sd_buff,"ppposInit faile,module offline");
		//SD_Card_Write_Data(err_path,Sd_buff);
    }/*--------end init module 3G------------*/

	start_get_power =Get_mili();
	Start_Get_Time =Get_mili();
	Start_Post_Time=Get_mili();
    while (1) {
		//esp_task_wdt_reset();
		//esp_task_wdt_feed();
		if(Get_mili() >start_get_power)
		{
			Power_Process(&StData.Acquy);
			ESP_LOGI(TAG,"acquy 0: %d mV",StData.Acquy.Vol);
			if(StData.Acquy.Volume < 15 )
			{
				ESP_LOGE(TAG," -------turn off all device\n");
				Turn_on_relay(RELAY_1);
			}
			if(StData.Acquy.Volume>40)
			{
				ESP_LOGI(TAG," -------turn on all device\n");
				Turn_off_relay(RELAY_1);
			}
			start_get_power +=300000;
		}
		if(Start_Get_Time <Get_mili())
		{
			if(esp_server_get_config(Dataclient)==200)
			{
				Start_Get_Time += 300000;
				Cnt_Http_err = 0;
				StConfig.Cnt++;
				if(StConfig.IsUpdate ==1)
				{
					if( ota_update(Server)>0){

					}
					else
					{
						ESP_LOGE(TAG, "OTA FAIL ");
					}
				}
			}
			else
			{
				if((Cnt_Http_err++)>5)
				{
					sprintf(Sd_buff,"Cnt_Http_err :%d",Cnt_Http_err);
					//SD_Card_Write_Data(err_path,Sd_buff);
					esp_restart();
				}
			}
			Free_cur = xPortGetFreeHeapSize();
			if(Free_cur < 50000)
			{
				break;
			}
			ESP_LOGI(TAG,"free heap : %d byte, %d byte",Free_cur,Free_cur-free_reg);
			free_reg = Free_cur;
			ESP_LOGI(TAG,"Time system: %02d:%02d",StConfig.gio,StConfig.phut);
			if((StConfig.gio>17)||(StConfig.gio<6))
			{
				//batden
				Turn_on_relay(RELAY_3);
				ESP_LOGI(TAG,"Bat den \n");
			}
			else 
			{
				//tatden
				Turn_off_relay(RELAY_3);
				ESP_LOGI(TAG,"tat den \n");
			}	
		}
//    	if(StConfig.IsUpdateName ==0)
		if(Start_Post_Time < Get_mili())
		{
			Read_all_data();
			try =3;
			do{
				sprintf(Image_tmp,"%s_%d_%llu.jpg",StartImgName,StConfig.ID,Get_mili());
				ESP_LOGI(TAG,"Imagename_t: %s",Image_tmp);
				if( (res= Update_image(Imgclient,Image_tmp))<1000)
				{
					//sprintf(StData.Cam.ImageName,"%s_%d_%d_%llu.jpg","UpdateErr",StConfig.ID,res,Get_mili());
					ESP_LOGE(TAG,"Failed to update image,err: %d",res);

					//sprintf(Sd_buff,"Update_image err :%d",res);
					sprintf(StData.Cam.ImageName,"err_%s",Image_tmp);
					//SD_Card_Write_Data(err_path,Sd_buff);
				}
				else
				{
					sprintf(StData.Cam.ImageName,"%s",Image_tmp);
					ESP_LOGI(TAG,"Imagename: %s",StData.Cam.ImageName);
					break;
				}
				delay(2000);
			}while(try --);
			int t = Network_Send_StationData(Dataclient);	
			Start_Post_Time = Get_mili() + StConfig.Delaytime *60*1000 - 30000; //khoi dong truoc 30s
			ESP_LOGI(TAG,"Post data delay: %d", StConfig.Delaytime *60*1000);
		}
		gpio_set_level(LED_STATUS_1, StConfig.Cnt%2);
		
		if((StConfig.IsReset ==1)||(Get_mili() > 400000000))
		{
			esp_restart();
		}
		vTaskDelay(15000/portTICK_PERIOD_MS);
    }
	
}
void  SD_Card_Write_Data(char *Path,char *data)
{
	/*denit spi bus */
	ESP_LOGW(TAG,"begin Hspi_denit()");
	Hspi_denit();
	ESP_LOGW(TAG,"end Hspi_denit()");
	if(Sd_card_init() >0)
	{
		ESP_LOGI(TAG, "Opening file");
		FILE* f = fopen(Path, "a");
		if (f == NULL) {
			ESP_LOGE(TAG, "Failed to open file for writing");
		}
		else
		{
			fprintf(f, "%s\n",data);
			fclose(f);
			ESP_LOGI(TAG, "File: %s written",Path);
		}
		//wite to sd card -----------------------
		Sd_card_denit();
	}
	ESP_LOGW(TAG,"begin Hspi_init()");
	Spi_Ethernet_Init();
	ESP_LOGW(TAG,"end Hspi_init()");
	
}
void Nvs_init(void)
{
	/*--------begin init NVS flash------------*/
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK( err );
	/*--------end init NVS flash------------*/
}
void DNS_server_init(void)
{
	/*--------begin init DNS server------------*/
	ip_addr_t dnsserver;
	ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
	inet_pton(AF_INET, "8.8.8.8", &dnsserver);
	dns_setserver(0, &dnsserver);
	inet_pton(AF_INET, "8.8.4.4", &dnsserver);
	dns_setserver(1, &dnsserver);
	ESP_LOGI(TAG, "SET DNS SERVER DONE");
	/*--------end init DNS server------------*/
}
static void System_Timer(void)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = 40000;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_DIS;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = TIMER_AUTORELOAD_DIS;
    timer_init(TIMER_GROUP_0, TIMER_1, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0x00000000ULL);
    timer_start(TIMER_GROUP_0, TIMER_1);
}
uint64_t Get_mili(void)
{
    uint64_t task_counter_value;
    timer_get_counter_value(TIMER_GROUP_0,TIMER_1, &task_counter_value);
    return task_counter_value/2;
}
void delay(int dl)
{
	uint64_t dl_t = Get_mili() + dl;
	while(Get_mili() < dl_t)
	{
		vTaskDelay(1);
	}
}

void StationCongig_Init()
{

	StConfig.Data_URL = malloc(50);
	StConfig.Server_Url = malloc(50);
	StConfig.Ota_URL = malloc(50);
	StConfig.ApiKey = malloc(50);
	StConfig.UploadUrl = malloc(100);
	sprintf(StConfig.Server_Url,"canhbaongap.com");
	sprintf(StConfig.Data_URL,"http://api.canhbaongap.com/quantrac.php");
	sprintf(StConfig.Ota_URL,"http://firmware.namlongtekgroup.com/GetfileUpdate");
	sprintf(StConfig.ApiKey,"B423713AB2769F83889D9520C6794656");
	sprintf(StConfig.UploadUrl,"http://api.canhbaongap.com/upload.php");
	//sprintf(StConfig.UploadUrl,"http://quantrackhongkhi.namlongtekgroup.com/api/Embedded/InsertImage");
	StConfig.HH = 0;
	StConfig.Delaytime = 5;  //Phut
	StConfig.LV = 0;
	StConfig.HL = 0;
	StConfig.IsUpdate=0;
	StConfig.gio=0;
	StConfig.phut=0;
	StConfig.Cnt=0;
	StConfig.IsUpdateName =1;
	StConfig.TurnonLed=0;


	StData.Cam.status=0;
	StData.Cam.description_len=0;

	StData.Acquy.Volume=100;

	StData.Ph.ID=5;
	StData.Ph.data=0;
	StData.Ph.reg=30001;
	StData.Ph.description[0]=0;
	StData.Ph.description_len=0;
	StData.Ph.active =1;


	StData.Rain.ID=2;
	StData.Rain.data=0;
	StData.Rain.reg=40001;
	StData.Rain.description[0]=0;
	StData.Rain.description_len=0;
	StData.Rain.active =1;

	StData.Water_level.ID=1;
	StData.Water_level.data=0;
	StData.Water_level.reg=40001;
	StData.Water_level.description[0]=0;
	StData.Water_level.description_len=0;
	StData.Water_level.active=1;

	StData.Salinity.ID=4;
	StData.Salinity.data=0;
	StData.Salinity.reg=30007;
	StData.Salinity.description[0]=0;
	StData.Salinity.description_len=0;
	StData.Salinity.active=1;

}

// void Get_ip_server(char *Url,char* ip)
// {
//     const struct addrinfo hints = {
//         .ai_family = AF_INET,
//         .ai_socktype = SOCK_STREAM,
//     };
//     struct addrinfo *res;
//     struct in_addr *addr;

//     int err = getaddrinfo(Url, "80", &hints, &res);
//     if(err != 0 || res == NULL) {
//         ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
//     /* Code to print the resolved IP.
//        Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
//     addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
//     ESP_LOGI(TAG, "DNS lookup successfull server: %s IP=%s",Url, inet_ntoa(*addr));
//     sprintf(ip,"%s", inet_ntoa(*addr));
//     freeaddrinfo(res);
// }
void GPIO_Config()
{
	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = ((1ULL << ETH_CS_PIN));
    //set as input mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //enable pull-up mode
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&io_conf);
    io_conf.pin_bit_mask = ((1ULL << ETH_RESET_PIN));
    gpio_config(&io_conf);

    gpio_set_level(ETH_CS_PIN,1);
    gpio_set_level(ETH_RESET_PIN,1);

	io_conf.pin_bit_mask = ((1ULL << LED_STATUS_1));
    gpio_config(&io_conf);

	io_conf.mode         = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = (1ULL << 0);
	gpio_config(&io_conf);
}
int Get_data_inFlash(char *Name)
{
	esp_err_t err;
    nvs_handle my_handle;
    int32_t value = 0;

    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
    	ESP_LOGE(TAG,"Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        // Read
         // value will default to 0, if not set yet iint Set_id_inFlash(void);n NVS
        err = nvs_get_i32(my_handle, Name, &value);
        switch (err) {
            case ESP_OK:
            	ESP_LOGI(TAG," Get data done:  %d\n", value);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
            	ESP_LOGW(TAG,"The Id is not initialized yet!\n");
            	value = -1;
                break;
            default :
            	ESP_LOGE(TAG,"Error (%s) reading!\n", esp_err_to_name(err));
            	value = -1;
        }
        err = nvs_commit(my_handle);
        if(err != ESP_OK)
        {
        	ESP_LOGE(TAG,"nvs_commit fail\n");
        	value = -1;
        }
        nvs_close(my_handle);
    }
    return value;
}

//printf(" Hello world version 1.1!\n");
//
//sprintf(Server.foder,"canhbaongap/Station_Config");
//sprintf(Server.filename,"Station_%d_config.txt",37);
//if(Ota_start(Server)<0)
//{
//	vTaskDelay(5000);
//}
//Free_cur = xPortGetFreeHeapSize();
//if(Free_cur < 50000)
//{
//	break;
//}
//ESP_LOGI(TAG,"free heap : %d byte, %d byte",Free_cur,Free_cur-free_reg);
//free_reg = Free_cur;
//sprintf(Server.foder,"canhbaongap/Firmware");
//sprintf(Server.filename,"Firmware_%d.bin",37);
//ota_update(Server);
//vTaskDelay(20000);
