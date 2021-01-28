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
#include "esp_event.h"
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
#include "soc/rtc_wdt.h"
#include "Wifiap.h"
#include "esp_sleep.h"
#include "swing_socket_app.h"
#include "string.h"




#if USER_DEBUG
    static const char *TAG = "[MAIN]";
#endif



// public variable
Http_ota_server_t Server;
_StationConfig StConfig;
_StationData StData;
char IP_server[50];
char StartImgName[30]="Image_reset";
void Cmd_hal_task(void *pvParameter);
char *info_path 	= "/sdcard/info.txt";
char *err_path 	= "/sdcard/err.txt";
char *data_path 	= "/sdcard/data.txt";
EventGroupHandle_t s_wifi_event_group;
void Nvs_init(void);
void DNS_server_init(void);
static void System_Timer(void);
void StationCongig_Init();

uint32_t Start_Post_Time =0;
uint32_t Start_Get_Time =0;
uint8_t Cnt_Http_err = 0;
uint32_t start_get_power =0;
TaskHandle_t socket_Tag;
void GPIO_Config();
void  SD_Card_Write_Data(char *Path,char *data);
esp_err_t Esp_nvs_read_write_uint32(char *key,uint32_t *value,int mode);


uint8_t ESP_ID[6];
char ESP_ID_CHAR[20] ={0};

uint8_t cablib_salinity =0;
uint8_t cablib_salinity_val =0;
uint8_t cablib_Ph71 =0;

void app_main(void)
{
	Nvs_init();
		uart_config_t uart_config = {
			.baud_rate = 115200,
			.data_bits = UART_DATA_8_BITS,
			.parity = UART_PARITY_DISABLE,
			.stop_bits = UART_STOP_BITS_1,
			.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
    if (uart_param_config(UART_NUM_0, &uart_config)) esp_restart();
    if (uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)) esp_restart();
	if (uart_driver_install(UART_NUM_0, 1024 * 2, 1024 * 2, 0, NULL, 0)) esp_restart();
	uint32_t id_tmp = 6201;
	if(Esp_nvs_read_write_uint32("IDCONFIG",&id_tmp,0)!= ESP_OK)
	{
		USER_LOGE(TAG,"ID not set");
		id_tmp = 6201;
		if(Esp_nvs_read_write_uint32("IDCONFIG",&id_tmp,1) == ESP_OK)
		{
			USER_LOGI(TAG,"set id: %d done",id_tmp);
		}
		else
		{
			vTaskDelay(100);
			USER_LOGE(TAG,"set id: %d done",id_tmp);
			esp_restart();
		}
		
	}
	USER_LOGI(TAG,"id: %d",id_tmp);
	StConfig.ID =id_tmp;
	esp_efuse_mac_get_default(ESP_ID);
	sprintf(ESP_ID_CHAR,"H_%02X%02X%02X%02X%02X%02X",ESP_ID[0],ESP_ID[1],ESP_ID[2],ESP_ID[3],ESP_ID[4],ESP_ID[5]);

	if (xTaskCreate(Cmd_hal_task, "Cmd_hal_task", 4096, NULL, 1, NULL) != pdPASS)
	{
		esp_restart();
		//return -1;
	}

	System_Timer();
	GPIO_Config();

	
	Power_Init();
	StationCongig_Init();
	if(ModbusMaster_Init() != 1)
    {
        USER_LOGE(TAG,"Modbus Init fail");
    }
    USER_LOGI(TAG,"Modbus Init done");
	DNS_server_init();

	Spi_Ethernet_Init();
	Turn_off_all_relay();
    USER_LOGI(TAG,"reset all device 30s cpu clock: %d\n",ets_get_cpu_frequency());

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
	        USER_LOGE(TAG, "Failed to initialise HTTP connection");
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
	char Image_tmp[50];
	
	if (ppposInit() < 0) {
		USER_LOGE(TAG,"ppposInit faile,module offline");
    }/*--------end init module 3G------------*/

	start_get_power =0;
	Start_Get_Time =0;
	Start_Post_Time=0;
    while (1) {
		if(Get_mili() >start_get_power)
		{
			Power_Process(&StData.Acquy);
			USER_LOGI(TAG,"acquy 0: %d mV",StData.Acquy.Vol);

			if(StData.Acquy.Volume < 15 )
			{
				USER_LOGE(TAG," -------turn off all device");

				//Turn_on_relay(RELAY_1);
			}
			if(StData.Acquy.Volume>40)
			{
				USER_LOGI(TAG," -------turn on all device");

				//Turn_off_relay(RELAY_1);
			}
			start_get_power +=300000;
		}

		if(Start_Post_Time <Get_mili())
		{
			int ppstatus;
			if((ppstatus = ppposStatus()) != GSM_STATE_CONNECTED)
			{
					if (ppposInit() < 0) {
						USER_LOGE(TAG,"ppposInit faile,module offline");
					}/*--------end init module 3G------------*/
					else
					{
						USER_LOGI(TAG,"ppposInit done");
					}

			}
			else
			{
				USER_LOGE(TAG,"ppposStatus: %d",ppstatus);
			}
			Turn_on_relay(RELAY_1);
			USER_LOGI(TAG,"Turn_on_relay: %d\n",ets_get_cpu_frequency());
			delay(35000);
			if(cablib_salinity)
			{	
				Calib_sanity(cablib_salinity_val);
				cablib_salinity = 0;
			}
			if(cablib_Ph71)
			{
				Calib_PH();
				cablib_Ph71 = 0;
			}
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
						USER_LOGE(TAG, "OTA FAIL ");
					}
				}
			}
			else
			{
				if((Cnt_Http_err++)>5)
				{

					esp_restart();
				}
			}
			Free_cur = xPortGetFreeHeapSize();
			if(Free_cur < 50000)
			{
				break;
			}
			USER_LOGI(TAG,"free heap : %d byte, %d byte",Free_cur,Free_cur-free_reg);

			free_reg = Free_cur;
			USER_LOGI(TAG,"Time system: %02d:%02d",StConfig.gio,StConfig.phut);
			if((StConfig.gio>17)||(StConfig.gio<6))
			{
				//batden
				Turn_on_relay(RELAY_2);
				USER_LOGI(TAG,"Bat den");
			}
			else 
			{
				//tatden
				Turn_off_relay(RELAY_2);
				USER_LOGI(TAG,"tat den");
			}	
			Read_all_data();
			try =3;
			do{
				sprintf(Image_tmp,"%s_%d_%llu.jpg",StartImgName,StConfig.ID,Get_mili());
				USER_LOGI(TAG,"Imagename_t: %s",Image_tmp);
				if( (res= Update_image(Imgclient,Image_tmp))<1000)
				{
					USER_LOGE(TAG,"Failed to update image,err: %d",res);
					sprintf(StData.Cam.ImageName,"err_%s",Image_tmp);
				}
				else
				{
					sprintf(StData.Cam.ImageName,"%s",Image_tmp);
					USER_LOGI(TAG,"Imagename: %s",StData.Cam.ImageName);
					break;
				}
				delay(2000);
			}while(try --);
			Network_Send_StationData(Dataclient);	
			Start_Post_Time = Get_mili() + StConfig.Delaytime *60*1000 - 30000; //khoi dong truoc 30s
			USER_LOGI(TAG,"Post data delay: %d s", StConfig.Delaytime *60);
			if(StConfig.Delaytime>=5)
			{
				Turn_off_relay(RELAY_1);
				ppposDisconnect(1,1);
				delay(StConfig.Delaytime *60*1000);
				esp_restart();
				// USER_LOGI(TAG,"ppposStatus wait idle");
				// while(ppposStatus() != GSM_STATE_IDLE);
				// USER_LOGI(TAG,"uc enter CFUN=4");
				// atCmd_waitResponse("AT+CFUN=4\r\n", "OK\r\n", NULL, -1, 1000, NULL, 0);
				//esp_sleep_enable_timer_wakeup(StConfig.Delaytime *60*1000);
			}
		}
		gpio_set_level(LED_STATUS_1, StConfig.Cnt%2);
		if((StConfig.IsReset ==1)||(Get_mili() > 400000000))
		{
			esp_restart();
		}
		else
		{
			
		}
		vTaskDelay(1);
		rtc_wdt_feed();
		
    }
	
}
void  SD_Card_Write_Data(char *Path,char *data)
{
	/*denit spi bus */
	USER_LOGW(TAG,"begin Hspi_denit()");
	Hspi_denit();
	USER_LOGW(TAG,"end Hspi_denit()");
	if(Sd_card_init() >0)
	{
		USER_LOGI(TAG, "Opening file");
		FILE* f = fopen(Path, "a");
		if (f == NULL) {
			USER_LOGE(TAG, "Failed to open file for writing");
		}
		else
		{
			fprintf(f, "%s\n",data);
			fclose(f);
			USER_LOGI(TAG, "File: %s written",Path);
		}
		//wite to sd card -----------------------
		Sd_card_denit();
	}
	USER_LOGW(TAG,"begin Hspi_init()");
	Spi_Ethernet_Init();
	USER_LOGW(TAG,"end Hspi_init()");
	
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
	USER_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
	inet_pton(AF_INET, "8.8.8.8", &dnsserver);
	dns_setserver(0, &dnsserver);
	inet_pton(AF_INET, "8.8.4.4", &dnsserver);
	dns_setserver(1, &dnsserver);
	USER_LOGI(TAG, "SET DNS SERVER DONE");
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
		rtc_wdt_feed();
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
	//sprintf(StConfig.ApiKey,"B423713AB2769F83889D9520C6794656");
	sprintf(StConfig.ApiKey,"6DF34EFB0DE43E4CE3DB02497110F4E7");
	sprintf(StConfig.UploadUrl,"http://api.canhbaongap.com/upload.php");
	//sprintf(StConfig.UploadUrl,"http://quantrackhongkhi.namlongtekgroup.com/api/Embedded/InsertImage");

	Server.ApiKey = malloc(100);
    sprintf(Server.ApiKey,"%s",StConfig.ApiKey);
    Server.Url = malloc(100);
    sprintf(Server.Url,"%s",StConfig.Ota_URL);
    Server.foder = malloc(100);
	sprintf(Server.foder,"canhbaongap/Firmware/");
    Server.filename = malloc(100);
	sprintf(Server.filename,"Firmware_%d.bin",StConfig.ID);

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
    	USER_LOGE(TAG,"Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        // Read
         // value will default to 0, if not set yet iint Set_id_inFlash(void);n NVS
        err = nvs_get_i32(my_handle, Name, &value);
        switch (err) {
            case ESP_OK:
            	USER_LOGI(TAG," Get data done:  %d", value);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
            	USER_LOGW(TAG,"The Id is not initialized yet!");
            	value = -1;
                break;
            default :
            	USER_LOGE(TAG,"Error (%s) reading!", esp_err_to_name(err));
            	value = -1;
        }
        err = nvs_commit(my_handle);
        if(err != ESP_OK)
        {
        	USER_LOGE(TAG,"nvs_commit fail");
        	value = -1;
        }
        nvs_close(my_handle);
    }
    return value;
}
esp_err_t Esp_nvs_read_write_uint32(char *key,uint32_t *value,int mode)
{   
    uint32_t tmp =0;
    esp_err_t err;
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,"Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        if(mode == 0) // read
        {
            err = nvs_get_u32(my_handle,key,&tmp);
            switch (err) {
                case ESP_OK:
                     ESP_LOGW(TAG,"The value %s was nvs_get_u32 yet!",key);
                    break;
                case ESP_ERR_NVS_NOT_FOUND:
                    ESP_LOGE(TAG,"The value %s not initialized yet!",key);  // write 0
                    break;
                default :
                    ESP_LOGE(TAG,"Error (%s) reading!", esp_err_to_name(err));
                    break;
            }
            *value = tmp;
        }
        else{
                if((err = nvs_set_u32(my_handle,key,*value)) != ESP_OK)
                {
                    ESP_LOGE(TAG,"Error (%s) when nvs_set_u32!", esp_err_to_name(err));
                }
                err = nvs_commit(my_handle);
                if(err != ESP_OK){
                    ESP_LOGE(TAG,"Error (%s) when nvs_commit!", esp_err_to_name(err));
                }
                else
                    ESP_LOGI(TAG,"nsv_commit done");
        }
        nvs_close(my_handle);
    }
    return err;
}
esp_err_t Esp_nvs_erase_all(void)
{
    esp_err_t err;
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,"Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        err = nvs_erase_all(my_handle);
        if(err != ESP_OK){
            ESP_LOGE(TAG,"Error (%s) when nvs_erase_all!", esp_err_to_name(err));
        }
        err = nvs_commit(my_handle);
        if(err != ESP_OK){
            ESP_LOGE(TAG,"Error (%s) when nvs_commit!", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGE(TAG,"nvs_erase_all commit done");
        }
        nvs_close(my_handle);
    }
    return err;
}

int Get_string_json(char * des,const char * json,const char *key)
{
    char key_tmp[100];
    sprintf(key_tmp,"\"%s\":",key);
    char *start = strstr(json,key_tmp);
    if(!start)			//key add "" not found
    {
    	sprintf(key_tmp,"%s:",key);
    	if(!(start = strstr(json,key_tmp)))
        	return -1;
    }
    int sizekey = strlen(key_tmp);
    int size=0;
    start +=sizekey; //moves to the beginning of the value
    //printf("%s\n",start );
    char *end;
    if(*(start)=='[') //value is a array
    {
    	start +=1;
    	if(!(end = strstr(start,"]")))	//end of array
        	return -1;
        size = (int)end - (int)start;
    }
    else 
    {	
    	//printf("%s\n",start );
	    if(!(end = strstr(start,",")))
	    {
	    	if(!(end = strstr(start,"}")))
        		return -1;	
	    }
	    //printf("end:%s\n",end );
	    if(*(start)=='\"') //value is a strings;
	    {
	    	start +=1;
	    	size = (int)end - (int)start -1;
	    }
	    else
	    {
	    	size = (int)end - (int)start;
	    }
	}
	if(size<0)
		return -1;
    memcpy(des,start,size);
    des[size]=0;
    //printf("des: %d\n",size);
    return size;
}
int Get_int_json(int *des,const char * json,const char *key)
{
    char tmp[50];
    if(Get_string_json(tmp,json,key)>0)
    {
        *des =  atoi(tmp);
        return *des;
    }
    return -1;
}
void Calib_sanity(uint16_t sample)
{
	USER_LOGE(TAG,"start calib sanity: %d",sample);
	vTaskDelay(1);
	if(MBm_Write_signal_data(4,40001+0x23,0x03) == MB_OK)
	{
		if(MBm_Write_signal_data(4,40001+0x24,0x02) == MB_OK)
		{	if(MBm_Write_signal_data(4,40001+0x40,0x60) == MB_OK)
			{
				USER_LOGE(TAG,"Enter the calibration successful");
				if(MBm_Write_signal_data(4,40001+0x41,sample) == MB_OK)
				{
					USER_LOGE(TAG,"Wait for 2 minutes");
					for(uint8_t i =90;i>0;i--)
					{
						USER_LOGE(TAG,"time remaing: %d",i);
						delay(1000);
					}
					uint16_t result =100;
					MBm_Read_data_hoilding(4,40001+0x43,1,&result);
					USER_LOGE(TAG,"result: %d",result);
					return;
				}
			}
		}
		else
		{
			USER_LOGE(TAG,"MBm_Write_signal_data(4,40001+0x24,0x02)");
		}
	}
	USER_LOGE(TAG,"MBm_Write_signal_data(4,40001+0x23,0x03)");
}
void Calib_PH()
{
	USER_LOGE(TAG,"start calib PH");
	if(MBm_Write_signal_data(5,40001+0x40,0x60) == MB_OK)
	{
		USER_LOGE(TAG,"Enter the calibration successful");
		if(MBm_Write_signal_data(5,40001+0x41,0x04) == MB_OK)
		{
			USER_LOGE(TAG,"Wait for 10 seconds");

			delay(15000);
			{
				uint16_t result =100;
				MBm_Read_data_hoilding(5,40001+0x43,1,&result);
				USER_LOGE(TAG,"result: %d",result);
				return;
			}
		}
		else
		{
			/* code */
		}
		
	}
	USER_LOGE(TAG,"the calibration fail");
}

typedef enum{
	GET_ID=1,
	SET_ID,
	GET_TIME,
	SET_CABLIB_SALINITY,
	SET_CABLIB_PH,
	RESET_DEFAULT,
	RESET

}cmd_hal;
char System_handles_out[257];
int System_handles(char *cmd,char * output)
{
	USER_LOGW(TAG, "recv: %s",(char *)cmd);
	int cmd1 = -1;
	if(Get_int_json(&cmd1,cmd,"cmd")>=0)
	{
		switch (cmd1)
		{
		case GET_ID:
			USER_LOGI(TAG, "Id : %d",StConfig.ID);
			break;
		case SET_ID:
			if(Get_int_json(&cmd1,cmd,"val")>=0)
			{
				USER_LOGI(TAG, "set id : %d",cmd1);
				uint32_t id_set = cmd1;
				if(Esp_nvs_read_write_uint32("IDCONFIG",&id_set,1) == ESP_OK)
				{
					USER_LOGI(TAG,"set id: %d done",cmd1);
					USER_LOGI(TAG,"reset_esp");
					vTaskDelay(200);
						esp_restart();
				}
				else
					USER_LOGE(TAG, "store nsv fail");
			}
			else
			{
				USER_LOGE(TAG, "get val fail");
			}
			/* code */
		break;
		case SET_CABLIB_SALINITY:
			if(Get_int_json(&cmd1,cmd,"val")>=0)
			{
				USER_LOGI(TAG, "set request cablib salinity: %d done",cmd1);
				cablib_salinity_val = cmd1;
				cablib_salinity =1;
				
			}
		/* code */
		break;
		case SET_CABLIB_PH:
			USER_LOGI(TAG, "set request cablib PH done");
			cablib_Ph71 =1;
			
		/* code */
		break;
		case RESET_DEFAULT:
			Esp_nvs_erase_all();
			esp_restart();
		/* code */
		break;
		case RESET:
		/* code */
		break;
		default:
			USER_LOGE(TAG, "fail value cmd: %d",cmd1);
			break;
		}
			// if(cmd1 == 1)
			// {
			// 	if(Get_int_json(&cmd1,cmd,"val"))
			// 		//Calib_sanity(cmd1);
			// 	{
			// 		//salinityCablib = 1;
			// 		return sprintf(output,"set salinityCablib : 1\n");
			// 	}
			// }
			// else if(cmd1 == 2)
			// {
			// 	//Calib_PH();
			// }
			// else
			// {
			// 	USER_LOGE(System_handles,"fail value cmd: %d",cmd1);
			// 	// Socketcontrol_out(output,outsize);
			// }
	}
	else
	{
		USER_LOGE(TAG, "json fail");
	}
	return 0;
}

void Cmd_hal_task(void *pvParameter)
{
	char line[256] = {0};
	int count = 0;
	uint8_t cnt_tmp =0;
	for(;;)
	{
		count = 0;
		while (1) {
            int s = uart_read_bytes(UART_NUM_0,(uint8_t *)line+count,127,10);
			if(s > 0 )
			{
				count += s;
				if (strstr(line,"\r\n")) {
					line[count] = '\0';
					break;
				}
				if(count >100)
					break;
				
			}
			cnt_tmp ++;
            if(cnt_tmp ==0)
				vTaskDelay(1);			//10ms
        }
		//USER_LOGW(System_handles,"recv: %s",line);
		System_handles(line,System_handles_out);
		vTaskDelay(10);
		rtc_wdt_feed();
		
	}
}