#include "Ethernet.h"
#include <stdio.h>
#include <string.h>

#include "driver/spi_master.h"
#include "driver/gpio.h"


#include "socket.h"	
#include "wizchip_conf.h"
#include "define.h"



#define USE_STARCAM 0


// #define PIN_NUM_MISO GPIO_NUM_12
// #define PIN_NUM_MOSI GPIO_NUM_13
// #define PIN_NUM_CLK  GPIO_NUM_14 
#define PIN_NUM_CS   ETH_CS_PIN
extern _StationConfig StConfig;



uint8_t OutputData =0x00;


#define GPIO_OUTPUT_CS_SEL  ((1ULL<<PIN_NUM_CS))


spi_device_handle_t Wiz_5500;
spi_device_handle_t relay;
spi_device_handle_t Lora;

void Hspi_init(void);
static void Eth_network_init(void);
void  wizchip_select(void);
void  wizchip_deselect(void);
uint8_t wizchip_read();
void  wizchip_write(uint8_t wb);
void SPI_read_bus(uint8_t* pBuf, uint16_t len);
void SPI_Write_bus(uint8_t* pBuf, uint16_t len);


void Hspi_init(void)
{
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=4096
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=8*1000*1000,           //Clock out at 10 MHz
        .mode=3,                                //SPI mode 0
        .spics_io_num=-1,                       //CS pin 
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        //.pre_cb=W5500_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &Wiz_5500);
    ESP_ERROR_CHECK(ret);

    devcfg.clock_speed_hz = 5*1000*1000;
    devcfg.mode = 3;
    devcfg.spics_io_num = RELAY_SELECT;
    devcfg.queue_size =7;
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &relay);
    ESP_ERROR_CHECK(ret);

    // devcfg.clock_speed_hz =12*1000*1000;
    // devcfg.mode = 3;
    // devcfg.spics_io_num = LORA_NSS;
    // devcfg.queue_size =7;
    // ret=spi_bus_add_device(HSPI_HOST, &devcfg, &Lora);
    // ESP_ERROR_CHECK(ret);
    
    // spi_bus_remove_device(Wiz_5500);
    // spi_bus_remove_device(relay);
    
    // ESP_LOGW(TAG, "spi_bus_free :%d",spi_bus_free(HSPI_HOST));
    // ESP_LOGI(TAG, "Using SPI peripheral");

    // gpio_pad_select_gpio(PIN_NUM_CS);
    // /* Set the GPIO as a push/pull output */
    // gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);

    // gpio_pad_select_gpio(RELAY_SELECT);
    // /* Set the GPIO as a push/pull output */
    // gpio_set_direction(RELAY_SELECT, GPIO_MODE_OUTPUT);

    // gpio_set_level(PIN_NUM_CS, 1);
    // gpio_set_level(RELAY_SELECT, 1);

}
void Hspi_denit()
{

    gpio_set_level(PIN_NUM_CS, 1);
    gpio_set_level(RELAY_SELECT,1);

    spi_bus_remove_device(Wiz_5500);
    spi_bus_remove_device(relay);
    spi_bus_free(HSPI_HOST);

    gpio_set_level(PIN_NUM_CS, 1);
    gpio_set_level(RELAY_SELECT, 1);
    
}
void Turn_on_all_relay(void) 
{
    OutputData = 0x00;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8; 
    t.tx_data[0] = OutputData;
    t.flags = SPI_TRANS_USE_TXDATA;
    esp_err_t ret = spi_device_polling_transmit(relay, &t);
    assert( ret == ESP_OK );
}

void Turn_off_all_relay(void)
{
    OutputData = 0xFF;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8; 
    t.tx_data[0] = OutputData;
    t.flags = SPI_TRANS_USE_TXDATA;
    esp_err_t ret = spi_device_polling_transmit(relay, &t);
    assert( ret == ESP_OK );
}
void Turn_on_relay(uint8_t Relay)
{
    OutputData  &= ~Relay;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8; 
    t.tx_data[0] = OutputData;
    t.flags = SPI_TRANS_USE_TXDATA;
    esp_err_t ret = spi_device_polling_transmit(relay, &t);
    assert( ret == ESP_OK );
}

void Turn_off_relay(uint8_t Relay)
{
    OutputData  |= Relay;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8; 
    t.tx_data[0] = OutputData;
    t.flags = SPI_TRANS_USE_TXDATA;
    esp_err_t ret = spi_device_polling_transmit(relay, &t);
    assert( ret == ESP_OK );
} 
void wizchip_hw_reset()
{

    gpio_set_level(ETH_RESET_PIN,0);
    delay(100);
    gpio_set_level(ETH_RESET_PIN,1);
}
static void Eth_network_init(void)
{
	//printf("wizchip_sw_reset W5500 ");
	//wizchip_sw_reset();
	//delay(1000);
	uint8_t memsize[2][8] = {{16,0,0,0,0,0,0,0},{16,0,0,0,0,0,0,0}};
	if(ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1)
	{
       printf("WIZCHIP Initialized memmory fail.\r\n");
	}
	printf("WIZCHIP Initialized memmory done.\r\n");
    wiz_NetInfo gWIZNETINFO = { .mac = {0x00, 0x08, 0xdc,0x00, 0xab, 0xcd},
                            .ip = {192, 168, 1,2},
                            .sn = {255,255,255,0},
                            .gw = {192, 168, 1, 108},
                            .dns = {0,0,0,0},
                            .dhcp = NETINFO_STATIC };
    gWIZNETINFO.ip[3]=123;
    printf("StConfig.ID: %d,gWIZNETINFO.ip[3]:%d\n",StConfig.ID ,gWIZNETINFO.ip[3]);
    uint8_t tmpstr[6];
	ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
	
	gWIZNETINFO.ip[3]=0;
	ctlnetwork(CN_GET_NETINFO, (void*)&gWIZNETINFO);
	
	
	
	wiz_NetTimeout gWIZNETTIME = {.retry_cnt = 6,         //RCR = 3
                               .time_100us = 3000};     //RTR = 2000
	ctlnetwork(CN_SET_TIMEOUT,(void*)&gWIZNETTIME);
															
	// Display Network Information
	ctlwizchip(CW_GET_ID,(void*)tmpstr);
	printf("\r\n=== %s NET CONF ===\r\n",(char*)tmpstr);
	printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],
		  gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
	printf("SIP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
	printf("GAR: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
	printf("SUB: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
	printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
	printf("======================\r\n");
}

void  wizchip_select(void)
{
   gpio_set_level(PIN_NUM_CS,0);
}

void  wizchip_deselect(void)
{
   gpio_set_level(PIN_NUM_CS,1);
}

void  wizchip_write(uint8_t wb)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8; 
    t.tx_buffer = &wb;
    esp_err_t ret = spi_device_polling_transmit(Wiz_5500, &t);
    assert( ret == ESP_OK );
}

uint8_t wizchip_read(void)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8; 
    t.rxlength = 8;
    t.flags = SPI_TRANS_USE_RXDATA;
    esp_err_t ret = spi_device_polling_transmit(Wiz_5500, &t);
    assert( ret == ESP_OK );
    return t.rx_data[0];
}

void SPI_read_bus(uint8_t* pBuf, uint16_t len)

{
	spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8*len; 
    t.rxlength = 8*len;
    t.rx_buffer = pBuf;
    esp_err_t ret = spi_device_polling_transmit(Wiz_5500, &t);
    assert( ret == ESP_OK );
}
void SPI_Write_bus(uint8_t* pBuf, uint16_t len)
{
	spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=8*len; 
    t.tx_buffer = pBuf;
    esp_err_t ret = spi_device_polling_transmit(Wiz_5500, &t);
    assert( ret == ESP_OK );
}



// These vars will contain the hash
void Spi_Ethernet_Init(void)
{
    Hspi_init();
    //Ethernet_chipcs_Init();
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
	reg_wizchip_spiburst_cbfunc(SPI_read_bus,SPI_Write_bus);
	Eth_network_init();
}
