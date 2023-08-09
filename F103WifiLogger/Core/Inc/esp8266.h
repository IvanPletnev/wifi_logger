/*
 * esp8266.c
 *
 *  Created on: Jul 18, 2023
 *      Author: Admin
 */

#ifndef INC_ESP8266_C_
#define INC_ESP8266_C_

#include "main.h"

#define UART2RX_BUFFER_SIZE				512
#define UART2TX_BUFFER_SIZE				512

#define ESP_REPLY_NONE						0
#define ESP_REPLY_OK						1
#define ESP_REPLY_ERROR						2
#define ESP_REPLY_CWJAP						3
#define ESP_REPLY_CONNECTED					11
#define ESP_REPLY_WIFI_CONN_FAIL			12
#define ESP_REPLY_CIPMODE					13
#define ESP_REPLY_STATUS					14
#define ESP_REPLY_SERVER_CLOSED				15
#define ESP_REPLY_TIMEOUT					18
#define ESP_REPLY_CIPSEND					19

#define ESP_STATUS_WIFI_NOT_INIT			20
#define	ESP_STATUS_WIFI_NOT_STARTED			21
#define ESP_WIFI_CONNECTED					22
#define ESP_STATUS_TCP_CREATED				23
#define ESP_STATUS_TCP_DISCONN				24
#define ESP_STATUS_NOT_CONN_AP				25


//#define WIFI_SSID						"MGTS_GPON_A518"
//#define WIFI_PASS						"E3MC3XNT"
#define WIFI_SSID						"ASUS"
#define WIFI_PASS						"red_horse_wins"
#define	LOG_SERVER						"158.160.51.94"
#define	LOG_PORT						8009



enum {
	AT,
	CWMODE,
	CWJAP,
	CIPMODE,
	CIPSEND,
	ESP_STATUS,
	ESP_RST
};

typedef struct esp8266_cmd_struct
{
	char *data;
	uint16_t len;
	uint32_t timeout;
	int8_t retry_count;
	int8_t expect_retval;

}esp8266_cmd_t;

typedef enum _espMode {
	ESP_NOT_TRANSPARENT,
	ESP_TRANSPARENT
}espMode_t;

typedef struct __attribute__((__packed__)) _uartRxData {
	uint8_t buffer[UART2RX_BUFFER_SIZE];
	uint16_t size;
}uartRxData_t;

typedef struct __attribute__((__packed__)) _uartTxData {
	uint8_t buffer[UART2TX_BUFFER_SIZE];
	uint16_t len;
}uartTxData_t;

typedef struct __attribute__((__packed__)) _esp8266 {
	uartRxData_t uartRxBuf;
	uartTxData_t uartTxBuf;
	espMode_t mode;
	uint8_t isWifiConnected;
	uint8_t serverConnected;
}esp8266_t;

extern esp8266_t espHandler;
extern const esp8266_cmd_t cmds[];

uint8_t esp8266Init (void);
uint16_t esp8266ExecuteCmd(esp8266_cmd_t cmd);
uint16_t esp8266WifiConnect(char *ssid, char *password, uint16_t waitMs, int8_t retryCount);
uint16_t esp8266SetServer(uint16_t waitMs, int8_t retryCount);
uint16_t esp8266GetStatus(void);
uint8_t uartTxDataQueueSend (uint8_t * buf, uint16_t size);
uint8_t uartTxDataQueueSendFromIsr (uint8_t * buf, uint16_t size);
#endif /* INC_ESP8266_C_ */
