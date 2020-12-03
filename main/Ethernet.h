#ifndef __ETHERNET_H
#define __ETHERNET_H
#include "stdint.h"
#include "define.h"


//void GPIO_init(void);

void Hspi_denit();
void Hspi_init(void);

void Spi_Ethernet_Init(void);
int Get_image(uint8_t *Image_data,int *Image_size);
void Turn_on_all_relay(void);
void Turn_off_all_relay(void);
void Turn_off_relay(uint8_t Relay);
void Turn_on_relay(uint8_t Relay);

int CAM_SendCommand(uint8_t command);

#define RELAY_1 2
#define RELAY_2 4
#define RELAY_3 8
#define RELAY_4 16

// #define AUTO_FOCUS              14

// #define SET_POSITION_0          30
// #define GOTO_POSITION_0         31
// #define SET_POSITION_1          32
// #define GOTO_POSITION_1         33
// #define SET_POSITION_2          34
// #define GOTO_POSITION_2         35
// #define SET_POSITION_3          36
// #define GOTO_POSITION_3         37
// #define SET_POSITION_4          38
// #define GOTO_POSITION_4         39
// #define SET_POSITION_5          40
// #define GOTO_POSITION_5         41
// #define SET_POSITION_6          42
// #define GOTO_POSITION_6         43
// #define SET_POSITION_7          44
// #define GOTO_POSITION_7         45
// #define SET_POSITION_8          46
// #define GOTO_POSITION_8         47
// #define SET_POSITION_9          48
// #define GOTO_POSITION_9         49
// #define SET_POSITION_10          50
// #define GOTO_POSITION_10         51
// #define SET_POSITION_11          52
// #define GOTO_POSITION_11         53
// #define SET_POSITION_12          54
// #define GOTO_POSITION_12         55
// #define SET_POSITION_13          56
// #define GOTO_POSITION_13         57
// #define SET_POSITION_14          58
// #define GOTO_POSITION_14         59
// #define SET_POSITION_15          60
// #define GOTO_POSITION_15         61




#endif
