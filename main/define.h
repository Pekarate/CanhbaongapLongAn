
#ifndef __DEFINE_H
#define __DEFINE_H


#define GET_IMAGE_ERR "Image_err_"
#define FTP_IMAGE_ERR "Image_err_ftp_"

#define USER_DEBUG 1

#include "time.h"
#include "stdint.h"
#include "string.h"
#define LORA_NSS    GPIO_NUM_15


#define ETH_RESET_PIN   GPIO_NUM_18
#define ETH_CS_PIN     GPIO_NUM_19

#define SIM_RESET   GPIO_NUM_27
#define SIM_TX      GPIO_NUM_16
#define SIM_RX      GPIO_NUM_17

#define PIN_NUM_MISO    GPIO_NUM_12
#define PIN_NUM_MOSI    GPIO_NUM_13
#define PIN_NUM_CLK    GPIO_NUM_14

#define RS485_E     GPIO_NUM_25
#define RS485_TX    GPIO_NUM_33
#define RS485_RX    GPIO_NUM_26

#define RELAY_SELECT GPIO_NUM_4

#define LED_STATUS_1 GPIO_NUM_2
#define TURN_ON_LED_1   gpio_set_level(LED_STATUS_1, 0)
#define TURN_OFF_LED_1  gpio_set_level(LED_STATUS_1, 1)

#define LED_STATUS_2 GPIO_NUM_5
#define TURN_ON_LED_2   gpio_set_level(LED_STATUS_2, 0)
#define TURN_OFF_LED_2  gpio_set_level(LED_STATUS_2, 1)


struct _time
{
    uint64_t Saved_Time;
    uint64_t Waiting_Time;
};

typedef struct
{
    struct _time time;
    uint8_t Volume;
    uint8_t Volume_tmp[4];
    uint8_t cnt_tmp;
    char description[10];
    uint16_t description_len;
    uint32_t Vol;
    uint32_t cnt;
}_Energy;

typedef struct
{
    char ImageName[100];
    uint8_t status;
    char description[10];
    uint16_t description_len;
}_Camera;

typedef struct
{
    uint8_t ID;
    uint16_t reg;
    uint16_t data;
    uint16_t data_t;
    uint16_t data_t1; // dành cho các cảm biến có nhiều dữ liệu như độ mặn.
    uint16_t data_t2;
    uint16_t data_t3;
    uint16_t data_t4;
    uint16_t data_t5;
    uint16_t data_t6;
    uint8_t status;
    uint16_t description_len;
    char description[256];
    uint8_t active;
    uint8_t cnt_err;
}Sensor_t;

typedef struct {
	  Sensor_t Rain;
	  Sensor_t Water_level;
	  Sensor_t Salinity;
      Sensor_t C02;  //them 1 đối tượng cho sensor mới   bước1
	  Sensor_t Ph;
	  _Energy Acquy;
	  _Camera Cam;


}_StationData;

typedef struct {
	int ID; // id tram
	char *Server_Url;
    char *Data_URL;
    char *Ota_URL;
    char *ApiKey;
    char *UploadUrl;
    uint32_t Delaytime;                //time update data without any change
    uint16_t HL;                //high threshold
    uint16_t LV;                //low threshold
    uint16_t HH;                //H0
    uint8_t IsUpdate;
    uint8_t  IsReset;
    uint8_t	 IsUpdateName;
    char Unix_Time[50];         //time when get config
    uint8_t TurnonLed;
    uint8_t gio;
    uint8_t phut;
    uint32_t Cnt;
}_StationConfig;

extern _StationConfig StConfig;
extern _StationData StData;
extern char ESP_ID_CHAR[20];

uint64_t Get_mili(void);
void delay(int dl);
void  SD_Card_Write_Data(char *Path,char *data);
int Get_int_json(int *des,const char * json,const char *key);
int Get_string_json(char * des,const char * json,const char *key);
void Calib_sanity(uint16_t sample);
void Calib_PH();


extern int Socketcontrol_out(char *buf,int size);

extern char SocketBuf[1025];
extern int SocketBufSize;

#define USER_LOGI(tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO,tag, format, ##__VA_ARGS__);// SocketBufSize = snprintf(SocketBuf,1024,(format), ##__VA_ARGS__); Socketcontrol_out(SocketBuf,SocketBufSize)
#define USER_LOGW(tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_WARN,    tag, format, ##__VA_ARGS__);// SocketBufSize = snprintf(SocketBuf,1024,(format), ##__VA_ARGS__); Socketcontrol_out(SocketBuf,SocketBufSize)
#define USER_LOGE(tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR,   tag, format, ##__VA_ARGS__);// SocketBufSize = snprintf(SocketBuf,1024,(format), ##__VA_ARGS__); Socketcontrol_out(SocketBuf,SocketBufSize)

#endif
