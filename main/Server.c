#include "Server.h"
#include "esp_log.h"
#include <lwip/dns.h>

#include "esp_http_client.h"
#include "define.h"
#include "string.h"

#include "esp_err.h"
static const char *TAG = "[SERVER]";

extern _StationData StData;
extern _StationConfig StConfig;
extern char StartImgName[50];
extern char *info_path ;
extern char *err_path  ;
extern char *data_path ;



static void Process_Data_recever(char *data)
{
    int res =0;
    int change = 0;
    USER_LOGI(TAG, "Data processing: %s",data);

    if(Get_int_json(&res,data,"TM")>0)
    {
        if(res> 0)    {
            if(StConfig.Delaytime!= res)
                change = 1;
            StConfig.Delaytime= res;
        }
        else          USER_LOGE(TAG, "Delaytime value wrong:%d",res);
    }
    else
        USER_LOGE(TAG, " Delaytime can't be found");

    if(Get_int_json(&res,data,"HL")>=0)
    {
        if(res> 0)    {
            if(StConfig.HL!= res)
                change = 1;
            StConfig.HL= res;
        }
        else{
                USER_LOGE(TAG, "HL value wrong:%d",res);
        }
    }
    else
        USER_LOGE(TAG, " HL can't be found");

    if(Get_int_json(&res,data,"LV")>=0)
    {
        if(res> 0)    {
            if(StConfig.LV!= res)
                change = 1;
            StConfig.LV= res;
        }
        else       	USER_LOGE(TAG, "LV value wrong:%d",res);
    }
    else
        USER_LOGE(TAG, " LV can't be found");

    if(Get_int_json(&res,data,"HH")>=0)
    {
        if(res>= 0)   {
            if(StConfig.HH!= res)
                change = 1;
            StConfig.HH= res;
        }
        else        USER_LOGE(TAG, "HH value wrong:%d",res);

    }
    else	USER_LOGE(TAG, " HH can't be found");
    if(Get_int_json(&res,data,"hh")>=0)
	{
		if(res>= 0)    StConfig.gio = res;
		else		USER_LOGE(TAG, "cf-> hh fail");
	}
	else	USER_LOGE(TAG, "hh can't be found");
	if(Get_int_json(&res,data,"mm")>=0)
	{
		if(res>= 0)    StConfig.phut = res;
		else{
			 USER_LOGE(TAG, "cf-> mm fail");
		}
	}
	else	USER_LOGE(TAG, "mm can't be found");
	if(Get_int_json(&res,data,"RST")>-1)
	{
		if((res == 0) ||(res == 1))
			StConfig.IsReset =res;
		else	USER_LOGE(TAG, "IsReset value wrong:%d",res);
	}
    if(Get_int_json(&res,data,"UD")>=0)
	{
		if((res == 0) ||(res == 1))
		{	
            StConfig.IsUpdate =res;
            if(res == 1)
            {     
                USER_LOGW(TAG, "new firmware is readly");
            }
            else
            {
                USER_LOGW(TAG, "No update request");
            }
            
        }
		else	USER_LOGE(TAG, "IsUpdate value wrong:%d",res);
	}
	if(StConfig.IsUpdateName ==1)
	{
		if(Get_string_json(StartImgName,data,"ImgName")>0)
		{
			StConfig.IsUpdateName =0;
			USER_LOGI(TAG, "StartImgName : %s",StartImgName);
		}
		else USER_LOGE(TAG, "ImgName can't be found");
	}
    if(change ==1)
    {
        char tmp[1024];
        sprintf(tmp,"%s,",data);
        //SD_Card_Write_Data(info_path,tmp);
    }
//    Station_ShowConfig();
}
int esp_server_get_config(esp_http_client_handle_t client)
{
    char BodyData[2048];
    int Status_code = -1;

    esp_err_t err =ESP_OK;

    int len = sprintf(BodyData,"DSI=%d&PS=1&WS=%d&RST=0&CNT=%d&UD=0",StConfig.ID,StConfig.TurnonLed,StConfig.Cnt);
    if(esp_http_client_open(client,len) != ESP_OK)
    {
        return -1;
    }
    esp_http_client_write(client,BodyData,len);
    esp_http_client_fetch_headers(client);
	if((Status_code =esp_http_client_get_status_code(client)) ==200){
		int ss = esp_http_client_read(client,BodyData,2048);
		if(ss>50)
		{
			USER_LOGI(TAG, "HTTP get config : %d byte", ss);
			BodyData[ss]=0;
			Process_Data_recever(BodyData);
            
		}
	}
	else
	{
		USER_LOGE(TAG, "HTTP POST request failed: %s,code : %d", esp_err_to_name(err),Status_code);
	}
//    esp_http_client_set_post_field(client, BodyData, len);
//    err = esp_http_client_perform(client);
//    if (err == ESP_OK) {
//        if((Status_code =esp_http_client_get_status_code(client)) ==200){
//            int ss = esp_http_client_read(client,BodyData,2048);
//            if(ss>50)
//            {
//            	USER_LOGI(TAG, "HTTP POST request failed: %d", ss);
//            	BodyData[ss]=0;
//            	Process_Data_recever(BodyData);
//            }
//        }
//    } else {
//        USER_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
//    }
////    esp_http_client_cleanup(client);
    esp_http_client_close(client);
    return Status_code;
}
int Network_Send_StationData(esp_http_client_handle_t client)
{
    char BodyData[5000];
    int Status_code = -1;

    esp_err_t err = ESP_OK;
    int len = sprintf(BodyData,"DSI=%d&"
                            "ED=%d&D_ED=%s&"
                            "SD=%d&SS=%d&D_SS=%s&"
                            "CD=%d/%s&CS=%d&D_CS=%s&"
                            "DTR=%d&SRS=%d&SRD=%s&"
                            "DTP=%d&PSS=%d&PSD=%s&"
                            "DTS=%d&SSS=%d&SSD=%s&"
                            "ET1=%d mv",
							StConfig.ID,
							StData.Acquy.Volume,StData.Acquy.description,\
							StData.Water_level.data,StData.Water_level.status,StData.Water_level.description,\
							StConfig.ID,StData.Cam.ImageName,StData.Cam.status,StData.Cam.description,\
							StData.Rain.data,StData.Rain.status,StData.Rain.description,\
							StData.Ph.data,StData.Ph.status,StData.Ph.description,\
							StData.Salinity.data,StData.Salinity.status,StData.Salinity.description,\
							StData.Acquy.Vol);
	//SD_Card_Write_Data(data_path,BodyData);                        
    esp_http_client_open(client,len);
    esp_http_client_write(client,BodyData,len);
    esp_http_client_fetch_headers(client);
	if((Status_code =esp_http_client_get_status_code(client)) ==200){
		int ss = esp_http_client_read(client,BodyData,2048);
		if(ss>50)
		{
			BodyData[ss]=0;
			USER_LOGI(TAG, "HTTP POST data received : %d byte %s", ss,BodyData);
			//Process_Data_recever(BodyData);
		}
	}
	else
	{
		USER_LOGE(TAG, "HTTP POST request failed: %s,Status_code: %d", esp_err_to_name(err),Status_code);
	}
    esp_http_client_close(client);
    
    return Status_code;
}
