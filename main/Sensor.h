
#ifndef __SENSOR_H
#define __SENSOR_H




int SensorInit(void);

void Read_all_data(void);

void Sensor_Led_Task(void *pvParameters);

typedef enum
{
    SENSOR_OK =0,
    SENSOR_STATR,
    SENSOR_DEBUG,
    SENSOR_ERR
}SensorState;
typedef struct 
{
    SensorState State;
    int SensorData_reg;
    int SensorData_cur;
}_SensorStruct;

#endif