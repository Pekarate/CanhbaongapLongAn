#include "Sensor.h"
#include "esp_log.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "ModbusMaster.h"
#include "define.h"

#define     ADDR_CM 40001
#define     ADDR_MM 40002

#define SENSOR_DEBUG 1
#if SENSOR_DEBUG
    static const char *SENSOR = "[SENSOR]";
#endif

extern _StationConfig StConfig;
extern _StationData StData;


uint8_t IsRain =0;
uint32_t RainStart =0;

int Sensor_Water_lever_read()
{
    MBm_StateTypedef res;
    uint16_t data_t =0;
    if((res = MBm_Read_data_hoilding(StData.Water_level.ID,ADDR_CM,1,&data_t)) == MB_OK){  
                
        //process data 
        #if SENSOR_DEBUG
            ESP_LOGI(SENSOR, "Sensor read data ok: %d cm",data_t);
        #endif
        StData.Water_level.data = data_t;
        //if(StData.Water_level.description_len <200)
            //StData.Water_level.description_len = sprintf(StData.Water_level.description,"%s%d,",StData.Water_level.description,data_t);
        StData.Water_level.cnt_err =0;
        StData.Water_level.status =1;
    }
    else
    {
        #if SENSOR_DEBUG
            ESP_LOGW(SENSOR, "Sensor read data fail:%d ",res);
        #endif
        StData.Water_level.cnt_err++;
        if(StData.Water_level.cnt_err >20)
            StData.Water_level.active =0;
        StData.Water_level.status =2;
    }
    return -1;
}
int Sensor_Rain_Read()
{
    MBm_StateTypedef res;
    uint16_t data_t =0;
    if( (res = MBm_Read_data_hoilding(StData.Rain.ID,40001,1,&data_t)) == MB_OK){  
                
        //process data 
        #if SENSOR_DEBUG
            ESP_LOGI(SENSOR, "RAIN read data ok: %d mm",data_t);
        #endif
        //if(StData.Rain.description_len<200)
            //StData.Rain.description_len = sprintf(StData.Rain.description,"%s%d,",StData.Rain.description,data_t);
        if(data_t>0)
        {
            //Senddata =1;
            if(IsRain== 0)
            {
                RainStart= Get_mili() +180000;
                IsRain = 1;
            }
            if(data_t == StData.Rain.data)
            {
                if(Get_mili() > RainStart)
                {
                    printf("reset rain sensor\n");
                    uint16_t value =0;
                    MBm_Write_multi_holding(StData.Rain.ID,40001,1,&value);
                    IsRain = 0;
                }
            }
            StData.Rain.data = data_t;
        }
        //res = MBm_Write_signal_data(StData.rainss.ID,40001,0);
        //printf("res = :%d",res);
        StData.Rain.cnt_err =0;
        StData.Rain.status =1;
    }
    else
    {
        #if SENSOR_DEBUG
            ESP_LOGW(SENSOR, "RAIN read data fail:%d ",res);
        #endif
        StData.Rain.cnt_err++;
        if(StData.Rain.cnt_err >20)
            StData.Rain.active =0;
        StData.Rain.status=2; 
    }
    return -1;
}
int Sensor_Ph_read()
{
    MBm_StateTypedef res;
   // uint16_t data_t =0;
    if( (res = MBm_Read_data_hoilding(StData.Ph.ID,30001,1,&StData.Ph.data_t)) == MB_OK){         
        //process data 
        #if SENSOR_DEBUG
            ESP_LOGI(SENSOR, "PHSS read data ok: %d",StData.Ph.data_t);
            StData.Ph.data =StData.Ph.data_t;
        #endif
        // if(StData.Ph.description_len<200)
        //     StData.Ph.description_len = sprintf(StData.Ph.description,"%s%d,",StData.Ph.description,StData.Ph.data);
        StData.Ph.cnt_err =0;
        StData.Ph.status =1;
    }
    else
    {
        #if SENSOR_DEBUG
            ESP_LOGW(SENSOR, "PHSS read data fail:%d ",res);
        #endif
        StData.Ph.cnt_err++;
        StData.Ph.status =2;
        if(StData.Ph.cnt_err >20)
            StData.Ph.active =0;
    }
    return -1;
}
int Sensor_Sanlinty_Read() // sửa đổi để đọc nhiều dữ liệu 
{
    MBm_StateTypedef res;
    uint16_t data_t[10]; // đọc 10 thanh ghi mỗi thanh ghi 16 bit
    if( (res = MBm_Read_data_hoilding(StData.Salinity.ID,30001,10,data_t)) == MB_OK){  //số lượng là 10 bắt đầu từ 3001
                
        //process data 
        #if SENSOR_DEBUG
            ESP_LOGI(SENSOR, "COIL read data ok: %d %d %d %d",data_t[0],data_t[2],data_t[4],data_t[6]);

            //thêm mấy cái này là được rồi.tiếp theo qua kia để sửa dữ liệu gửi lên
        #endif
        StData.Salinity.data = data_t[6];          //dò qua bảng kia sẽ biết chỉ số cần bỏ vào 
        StData.Salinity.data_t1 = data_t[0];       // cái này là EC
        StData.Salinity.data_t2 = data_t[2];       // cái này là TDS
        StData.Salinity.data_t3 = data_t[4];      // EC không bù nhiẹt
        // if(StData.Salinity.description_len<200)
        //     StData.Salinity.description_len = sprintf(StData.Salinity.description,"%s%d,",StData.Salinity.description,StData.Salinity.data);
        StData.Salinity.cnt_err =0;
        StData.Salinity.status = 1;
    }
    else
    {
        #if SENSOR_DEBUG
            ESP_LOGW(SENSOR, "COIL read data fail:%d ",res);
        #endif
        StData.Salinity.cnt_err++;
        if(StData.Salinity.cnt_err >20)
            StData.Salinity.active =0;
        StData.Salinity.status = 2;
    }
    return -1;
}

int Sensor_C02_read()
{
    MBm_StateTypedef res;
   // uint16_t data_t =0;
    if( (res = MBm_Read_data_hoilding(StData.C02.ID,40001,1,& StData.C02.data)) == MB_OK){       //chỉ cần xài hàm này là đọc được dữ liệu.mấy cái kia t viết chỉ để dự phòng thôi  
        //process data 
        #if SENSOR_DEBUG
            ESP_LOGI(SENSOR, "PHSS read data ok: %d",StData.C02.data_t);
           
        #endif
         StData.C02.data =StData.C02.data_t; // nếu ko thích có thể làm thế này
        // if(StData.Ph.description_len<200)
        //     StData.Ph.description_len = sprintf(StData.Ph.description,"%s%d,",StData.Ph.description,StData.Ph.data);
        StData.C02.cnt_err =0;
        StData.C02.status =1;
    }
    else
    {
        #if SENSOR_DEBUG
            ESP_LOGW(SENSOR, "PHSS read data fail:%d ",res);
        #endif
        StData.C02.cnt_err++;
        StData.C02.status =2;
        if(StData.C02.cnt_err >20)
            StData.C02.active =0;
    }
    return -1;
}


void Read_all_data(void)
{
    Sensor_C02_read();   //đẻ khỏi if luôn cũng được.mất thêm tý thời gian không sao.giờ chỉ qua bên hàm gửi lên thêm mấy cái dẽ liệu này nữa thôi
    if(StData.Water_level.active)
        Sensor_Water_lever_read();
    else
        ESP_LOGW(SENSOR, "Water_lever_sensor not active ");
    vTaskDelay(1);
    if(StData.Rain.active)
        Sensor_Rain_Read();
    else
        ESP_LOGW(SENSOR, "Rain_sensor not active ");
    vTaskDelay(1);
    if(StData.Salinity.active)
        Sensor_Sanlinty_Read();
    else
        ESP_LOGW(SENSOR, "Sanlinty_sensor not active ");
    vTaskDelay(1);
    if(StData.Ph.active)
        Sensor_Ph_read();
    else
        ESP_LOGW(SENSOR, "Ph_sensor not active ");
}