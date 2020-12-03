/*	Modbus.h
*	created on:28/3/19
	author:Hoang_Tran
	version :v1.1
	sourc:http://www.simplymodbus.ca/FC06.htm
*/
#ifndef __MODBUSMASTER_H
#define __MODBUSMASTER_H
#include "stdint.h"


#define MAX_RX_BUFFER_DATA 256
#define MAX_TX_BUFFER_DATA 256
#define  CODE_CRC 0xA001;

//-------------------------------------------------------


#define TIME_OUT  			100  //timeout
#define Mb_data    uint16_t

#define MAX_NUM_SLAVE 10;

#define COIL_LED0 0
#define COIL_LED1 1
#define COIL_LED2 2
#define COIL_LED3 3
#define COIL_LED4 4
#define COIL_LED5 5
#define COIL_LED6 6
#define COIL_LED7 7
/*FUNCTION CODE*/
typedef enum 
{
		 READ_COIL  								= 0X01,
		 READ_DISCRETE_INPUTS				= 0X02,
		 READ_HOLDING_REGISTERS 		= 0X03,
		 READ_INPUT_REGISTERS 			= 0X04,
		 WRITE_SINGLE_COIL 					= 0X05,
		 WRITE_SINGLE_REGISTER 			= 0X06,
		 READ_EXCEPTION_STATUS 			= 0X07,
		 WRITE_MULTIPLE_COILS				= 0X0F,
		 WRITE_MULTIPLE_REGISTERS 	= 0X10
}MODBUS_FUNCTIONS;

typedef enum 
{
	MB_TIME_OUT =0,			//time out
	MB_PAR_ERR,					//parameter error
	MB_ERR,
	MB_CRC_ERR,					//CRC error
	MB_OK,								//no error
	MB_REQUEST
}MBm_StateTypedef;

typedef enum 
{
	ON = 0xFF,
	OFF= 0x00
}MBm_CoilData;

typedef struct 
{
	/*
	*/
	uint16_t	pStart_Reg;	//chua cac thanh ghi bat dau
	uint8_t		pSize;		//chua cac kich thuoc cua thanh ghi theo sau thanh ghi bat dau.
	MBm_CoilData*	pData_coil;
}MBm_pCoil;

typedef struct 
{
	uint16_t	pStart_Reg;	//chua cac thanh ghi bat dau
	uint8_t		pSize;		//chua cac kich thuoc cua thanh ghi theo sau thanh ghi bat dau.
	uint16_t*	pData_hoilding;
}MBm_pHolding;

typedef struct 
{
	uint32_t Baud_rate;
	uint32_t Word_lenght;
	uint32_t Parity;
	uint32_t Stop_Bit;
}MBm_initTypedef;
typedef struct 
{
	MODBUS_FUNCTIONS Function;
	uint16_t Reg_Start;
	uint8_t Size;
	uint8_t Time_out;
}Mbm_RequestTypedef;

typedef struct 
{
	const char* name;
	uint8_t									ID;	
	MBm_initTypedef					        Hardware;
	uint8_t									Size_CoilArray;
	MBm_pCoil**							    pCoil;
	uint8_t 								Size_HoldingArray;
	MBm_pHolding** 					        pReg_hoiding;
	Mbm_RequestTypedef			            request;
	MBm_StateTypedef				        State;
}MB_slave;





void Mbm_init(void);
void MBm_Salve_Init(MB_slave *slave);
int ModbusMaster_Init(void);
//MBm_StateTypedef MBm_init(uint32_t baud,uint8_t par,uint8_t stop);//init uart

MBm_StateTypedef MBm_Read_data_coil(uint8_t SlaveID,uint16_t reg,uint8_t size,MBm_CoilData *data);
MBm_StateTypedef MBm_Read_data_hoilding(uint8_t SlaveID,uint16_t reg,uint8_t size,Mb_data *data);
MBm_StateTypedef MBm_Write_signal_data(uint8_t SlaveID,uint16_t reg,Mb_data value);
MBm_StateTypedef MBm_Write_multi_coil(uint8_t SlaveID,uint16_t reg_start,uint16_t size,MBm_CoilData *value);
MBm_StateTypedef MBm_Write_multi_holding(uint8_t SlaveID,uint16_t reg_start,uint16_t size,Mb_data *value);

void MBm_Get_data(void);
#endif
