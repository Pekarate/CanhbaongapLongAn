/*	
	*	Modbus.c
	*	created on:28/3/19
	*	author:Hoang_Tran
	
	MBm_Write_multi_coil //OK
*/
#include "ModbusMaster.h"
#include "stdio.h"
#include <stdlib.h> 

#include "define.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define MB_PORT  UART_NUM_1
#define MB_TX    RS485_TX
#define MB_RX    RS485_RX

#define UART_BAUDRATE 9600
#define UART_BUF_SIZE 1024

#define HALF_DUPLEX 1
#if HALF_DUPLEX      //hafp duplex
        #define MB_DE_RE RS485_E
		#define MB_SEND_ENABLE			gpio_set_level(MB_DE_RE,1);
		#define MB_RECEI_ENABLE			gpio_set_level(MB_DE_RE,0);
#else
#endif

void MBm_Send_Buff_Data(uint8_t *__Data,uint16_t __SIZE,uint16_t Timout)
{		
	uart_write_bytes(MB_PORT,(const char *)__Data,__SIZE);
	uart_wait_tx_done(MB_PORT,Timout);
}
int MBm_Recv_Buff_Data(uint8_t *__Data,uint16_t size,uint16_t Timeout)		  	
{
	uart_flush(MB_PORT);
	return uart_read_bytes(MB_PORT,__Data,size,Timeout);
}

uint8_t Rx_buf[MAX_RX_BUFFER_DATA];
volatile	uint8_t Rx_size =0;


uint8_t Tx_buf[MAX_TX_BUFFER_DATA];
uint8_t MBm_receive_data;

int ModbusMaster_Init(void)
{
	gpio_config_t io_conf;
    #if HALF_DUPLEX
		
		//interrupt of rising edge
		io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
		//bit mask of the pins, use GPIO4/5 here
		io_conf.pin_bit_mask = ((1ULL<<MB_DE_RE));
		//set as input mode
		io_conf.mode = GPIO_MODE_OUTPUT;
		//enable pull-up mode
		io_conf.pull_up_en =(gpio_pullup_t) 1;
		io_conf.pull_down_en = (gpio_pulldown_t)0;
		gpio_config(&io_conf);
    #endif
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
	//bit mask of the pins, use GPIO4/5 here
	io_conf.pin_bit_mask = ((1ULL<<MB_TX));
	//set as input mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//enable pull-up mode
	io_conf.pull_up_en =(gpio_pullup_t) 1;
	io_conf.pull_down_en = (gpio_pulldown_t)0;
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = ((1ULL<<MB_RX));
	gpio_config(&io_conf);
    uart_config_t uart_config = {
			.baud_rate = UART_BAUDRATE,
			.data_bits = UART_DATA_8_BITS,
			.parity = UART_PARITY_DISABLE,
			.stop_bits = UART_STOP_BITS_1,
			.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
    if (uart_param_config(MB_PORT, &uart_config)) return -3;
    if (uart_set_pin(MB_PORT, MB_TX, MB_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)) return -4;
	if (uart_driver_install(MB_PORT, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 0, NULL, 0)) return -5;
    return 1;
}
static uint16_t calculateOutputCrc1(uint16_t data,uint16_t code)
{
	data = data ^ code;
	for (int i = 0; i < 8; i++)
	{
		if (data & 0x0001)
		{
			data = data >> 1;
			data = data ^ CODE_CRC;
		}
		else
			data = data >> 1;
	}
	return data;
}
/**
	*	@brief	caculate CRC of data
	*	@param	data	data array
						len		size of array
	*	@return CRC of array
*/
uint16_t calculateOutputCrc(uint8_t data[],uint8_t len)
{
	uint16_t tam = calculateOutputCrc1(data[0], 0XFFFF);
	for (int i = 0; i < len - 1; i++)
	{
		tam = calculateOutputCrc1(data[i+1], tam);
	}
	tam = (tam << 8) + (tam >> 8);
	return tam;
}

/**
	*	@brief 	: init slave
	*	@param	:	
*/
void MBm_Salve_Init(MB_slave *slave)
{	
	if (slave->Size_CoilArray != 0)
		slave->pCoil = (MBm_pCoil **)malloc(slave->Size_CoilArray * sizeof(MBm_pCoil*));
	else
		slave->pCoil = NULL;
	if (slave->Size_HoldingArray != 0)
		slave->pReg_hoiding = (MBm_pHolding**)malloc(slave->Size_HoldingArray * sizeof(MBm_pHolding*));
	else
		slave->pReg_hoiding = NULL;
}


MBm_StateTypedef MBm_Read_data_coil(uint8_t SlaveID,uint16_t reg,uint8_t size,MBm_CoilData *data)
{
	Rx_size =0;
	uint8_t i,len;
	Tx_buf[0] = SlaveID;

	switch(reg/10000)
	{
		case 0:Tx_buf[1] = READ_COIL;	break;								//reg 1			-> 999
		case 1:Tx_buf[1] = READ_DISCRETE_INPUTS; 	break;		//reg 10001 -> 19999
		default : return MB_PAR_ERR;
	}
	Tx_buf[2] = ((reg % 10000)-1) >>8;			//data_register
	Tx_buf[3] = ((reg % 10000)-1);					//data_register
	Tx_buf[4] = size >>8;
	Tx_buf[5] = size ;
	uint16_t tam = calculateOutputCrc(Tx_buf,6);		//0x5073
	Tx_buf[6] = tam>>8;						//dao ma crc	->	73
	Tx_buf[7] = tam;					//dao ma crc	-> 	50
#if HALF_DUPLEX      //hafp duplex
		 MB_SEND_ENABLE;	
#endif
	//MBm_HardwareCheck(Slave);
	MBm_Send_Buff_Data(Tx_buf,8,100);
#if HALF_DUPLEX      //hafp duplex
		 MB_RECEI_ENABLE;

#endif
    Rx_size =  MBm_Recv_Buff_Data(Rx_buf,256,100);
	if(Rx_size != (6 + size/8))
	{
			return MB_TIME_OUT;
	}
	if(Rx_buf[1]!=Tx_buf[1])  //Function Code in Exception Response (function code +128)
		return MB_PAR_ERR;
	
	tam = calculateOutputCrc(Rx_buf,Rx_size -2); //CRC bufer RX
	uint16_t checksum = (Rx_buf[Rx_size -2]<<8) + (Rx_buf[Rx_size -1]); //CRC received
	if(tam != checksum)
				return MB_CRC_ERR;
	else
	{
				for(i =0;i<size;i++)
				{
					len =i/8;
					if(	(Rx_buf[3+len]>>(i%8) & 0x01))	
							*data =ON; 
					else 
							*data =OFF;		
					data++;
				}
				return MB_OK;
	}	
}
MBm_StateTypedef MBm_Read_data_hoilding(uint8_t SlaveID,uint16_t reg,uint8_t size,Mb_data *data)
{
	
	uint8_t pSize = 5 + size*2;
	Rx_size =0;
	
	//MBm_HardwareCheck(Slave);
	Tx_buf[0] = SlaveID;

	switch(reg/10000)
	{
		case 3:Tx_buf[1] = READ_INPUT_REGISTERS; 	break;		//reg 30001 -> 39999
		case 4:Tx_buf[1] = READ_HOLDING_REGISTERS;	break;	//reg 40001 -> 49999
		default : return MB_PAR_ERR;
	}
	Tx_buf[2] = ((reg % 10000)-1) >>8;			//data_register
	Tx_buf[3] = ((reg % 10000)-1);					//data_register
	Tx_buf[4] = size >>8;
	Tx_buf[5] = size ;
	uint16_t tam = calculateOutputCrc(Tx_buf,6);		//0x5073
	Tx_buf[6] = tam>>8;						//dao ma crc	->	73
	Tx_buf[7] = tam;					//dao ma crc	-> 	50
    //uart_flush(MB_PORT);

#if HALF_DUPLEX      //hafp duplex
		 MB_SEND_ENABLE;	
#endif
	MBm_Send_Buff_Data(Tx_buf,8,100);
    // printf("uart_write_bytes: %d",uart_write_bytes(MB_PORT,(const char *)Tx_buf,8)); 
    // printf("uart_wait_tx_done: %d",uart_wait_tx_done(MB_PORT,100));
#if HALF_DUPLEX      //hafp duplex
		MB_RECEI_ENABLE;
#endif
	Rx_size =  MBm_Recv_Buff_Data(Rx_buf,256,100);
	if(Rx_size != pSize)
	{
		return MB_TIME_OUT;
	}
	if(Rx_buf[1]!=Tx_buf[1])  //Function Code in Exception Response (function code +128)
		return MB_PAR_ERR;
	
	tam = calculateOutputCrc(Rx_buf,Rx_size -2); //CRC bufer RX
	uint16_t checksum = (Rx_buf[Rx_size -2]<<8) + (Rx_buf[Rx_size -1]); //CRC received
	if(tam != checksum)
				return MB_CRC_ERR;
	else
	{
				for(int i=0;i<size;i++)
				{
					*data = (Rx_buf[3+i*2] <<8) +Rx_buf[4+i*2];
					 data++;
				}
				return MB_OK;
	}	
}
/**
	*	@brief	Wrire data to register
	*	@param	Add		Slave address.
						reg		Register address.
						value	The value written.
	*	@return	MBm_StateTypedef.
	*	@node		client echo request
*/
MBm_StateTypedef MBm_Write_signal_data(uint8_t SlaveID,uint16_t reg,Mb_data value)
{ 
	Rx_size =0;
	Tx_buf[0] = SlaveID;

	switch(reg/10000)
	{
		case 0:
			Tx_buf[1] = WRITE_SINGLE_COIL;	
			Tx_buf[4] = 0x00;
			Tx_buf[5]	=	0X00;
			if(value)
						Tx_buf[4] = 0xFF;
			break;								//reg 1			-> 9999
		case 4:
			Tx_buf[1] = WRITE_SINGLE_REGISTER;
			Tx_buf[4] = value >>8;
			Tx_buf[5]	=	value;
			break;	//reg 40001 -> 49999
		default : return MB_PAR_ERR;
	}
	Tx_buf[2] = ((reg % 10000)-1) >>8;
	Tx_buf[3] = ((reg % 10000)-1);

	uint16_t tam = calculateOutputCrc(Tx_buf,6);		//0x5073
	Tx_buf[6] = tam>>8;						//dao ma crc	->	73
	Tx_buf[7] = tam;					//dao ma crc	-> 	50
#if HALF_DUPLEX      //hafp duplex
		 MB_SEND_ENABLE;	
#endif
	//MBm_HardwareCheck(Slave);
	MBm_Send_Buff_Data(Tx_buf,8,100);
#if HALF_DUPLEX      //hafp duplex
		 MB_RECEI_ENABLE;
#endif
	Rx_size =  MBm_Recv_Buff_Data(Rx_buf,256,100);
	if(Rx_size != 8)
	{
			return MB_TIME_OUT;
	}
	if(Rx_buf[1]!=Tx_buf[1])  //Function Code in Exception Response (function code +128)
		return MB_PAR_ERR;
	for(uint8_t i=0;i<8;i++)
	{
		if(Tx_buf[i]!=Rx_buf[i])
			return MB_ERR;
	}
	return MB_OK;
}
/**
	*	@brief	Wrire data to multi coil
	*	@param	Add		Slave address.
						reg		Register address.
						value	The value written.
						sizen	number register	
	*	@return	MBm_StateTypedef.
*/
MBm_StateTypedef MBm_Write_multi_coil(uint8_t SlaveID,uint16_t reg_start,uint16_t size,MBm_CoilData *value)
{
	if(	(reg_start==0)	||	(reg_start>9999) || (size==0))
			return MB_PAR_ERR;
	uint8_t len=7;
	uint8_t j;
	uint16_t tam;
	Rx_size =0;
	Tx_buf[0] = SlaveID;
	Tx_buf[1] = WRITE_MULTIPLE_COILS;	
	Tx_buf[2] = (reg_start -1) >>8;
	Tx_buf[3] = (reg_start-1);
	Tx_buf[4] = size >>8;
	Tx_buf[5]	=	size;
	Tx_buf[6] = ((size-1)/8+1);	//num of data  if 8bit => 1byte
	for(len =0;len < Tx_buf[6] ;len++)
	{
			Tx_buf[7+len] =0X00;
			if(((len+1)*8) < size )	tam =8; else tam = size - len*8;
			for( j=0;j<tam;j++)
			{
				if (*value == ON)
					Tx_buf[7+len]	=	Tx_buf[7+len] + (1<<j);	
				else	if (*value != OFF)
				{
					return MB_PAR_ERR;
				}
				value++;
			} 
	}
	tam = calculateOutputCrc(Tx_buf,7+len);		
	Tx_buf[7+	len	] = tam>>8;
	Tx_buf[7+	len	+	1] = tam;
	//dao ma crc	-> 	50
#if HALF_DUPLEX      //hafp duplex
		 MB_SEND_ENABLE;	
#endif
	//MBm_HardwareCheck(Slave);
	MBm_Send_Buff_Data(Tx_buf,len+9,100);
#if HALF_DUPLEX      //hafp duplex
		 MB_RECEI_ENABLE;
#endif
	//-----------------------------------------
	Rx_size =  MBm_Recv_Buff_Data(Rx_buf,256,100);
	if(Rx_size != 8)
	{
			return MB_TIME_OUT;
	}
	if(Rx_buf[1]!=Tx_buf[1])  //Function Code in Exception Response (function code +128)
		return MB_PAR_ERR;
	for(uint8_t i=0;i<6;i++)
	{
		if(Tx_buf[i]!=Rx_buf[i])
			return MB_ERR;
	}
	return MB_OK;
}
/**
	*	@brief	Wrire data to multi holding
	*	@param	Add		Slave address.
						reg		Register address.
						size	number register
						value	The value written.
							
	*	@return	MBm_StateTypedef.
*/
MBm_StateTypedef MBm_Write_multi_holding(uint8_t SlaveID,uint16_t reg_start,uint16_t size,Mb_data *value)
{
	//MBm_HardwareCheck(Slave);
	if(	(reg_start<40001)	||	(reg_start>49999))
			return MB_PAR_ERR;
	uint8_t len;
	uint16_t tam;
	Rx_size =0;
	Tx_buf[0] = SlaveID;
	Tx_buf[1] = WRITE_MULTIPLE_REGISTERS;	//0x10
	Tx_buf[2] = (reg_start -40001) >>8;
	Tx_buf[3] = (reg_start-40001);
	Tx_buf[4] = size >>8;
	Tx_buf[5]	=	size;
	Tx_buf[6] = (size*2);	//num of data  (unit: byte) size =2 -> t[6] = 4
	for(len =0;len < size ;len++)
	{
			Tx_buf[7+len*2] =*value>>8;
			Tx_buf[8+len*2] =*value;
			value++;
	}
	tam = calculateOutputCrc(Tx_buf,7+size*2);		
	Tx_buf[7+	size*2	] = tam>>8;
	Tx_buf[7+	size*2	+	1] = tam;
	//dao ma crc	-> 	50
#if HALF_DUPLEX      //hafp duplex
		 MB_SEND_ENABLE;	
#endif
	
	MBm_Send_Buff_Data(Tx_buf,size*2+9,100);
	#if HALF_DUPLEX      //hafp duplex
		 MB_RECEI_ENABLE;
#endif
	//-----------------------------------------
	Rx_size =  MBm_Recv_Buff_Data(Rx_buf,256,100);
	if(Rx_size != 8)
	{
			return MB_TIME_OUT;
	}
	if(Rx_buf[1]!=Tx_buf[1])  //Function Code in Exception Response (function code +128)
		return MB_PAR_ERR;
	
	for(uint8_t i=0;i<6;i++)
	{
		if(Tx_buf[i]!=Rx_buf[i])
			return MB_ERR;
	}
	return MB_OK;
}