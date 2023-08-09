/*
 * esp8266.c
 *
 *  Created on: Jul 18, 2023
 *      Author: Admin
 */

#include "main.h"
#include "cmsis_os.h"
#include "esp8266.h"
#include "string.h"
#include "stdio.h"
#include "task.h"
#include <stdlib.h>

extern UART_HandleTypeDef huart2;
extern osMessageQueueId_t uartTxQueueHandle;
extern osMessageQueueId_t uartRxQueueHandle;
extern osMessageQueueId_t replyQHandle;
extern uint16_t espSecCounter;
extern osTimerId_t espTimerHandle;

esp8266_t espHandler = {0};

uint8_t uartTxDataQueueSend (uint8_t * buf, uint16_t size);

const esp8266_cmd_t cmds[] = {
/*{command											len,		timeout,		retry,      expect_reply}*/
{ "AT\r\n", 										4, 			1000, 			3, 			ESP_REPLY_OK },
{ "AT+CWMODE=1\r\n",								11, 		3000, 			5, 			ESP_REPLY_OK },
{ "AT+CWJAP=\"ASUS\",\"red_horse_wins\"\r\n",		34,			10000, 			60, 		ESP_REPLY_CWJAP },
{ "AT+CIPMODE=1\r\n", 								14, 		2000, 			10,			ESP_REPLY_CIPMODE },
{ "AT+CIPSEND\r\n",									12, 		1000, 			3,			ESP_REPLY_CIPSEND },
{ "AT+CIPSTATUS\r\n",								14,			1000,			1,			ESP_REPLY_STATUS},
{ "AT+RST\r\n",										8,			1000,			1,			ESP_REPLY_OK}
};

//uint8_t esp8266init (void) {
//	uint16_t
//}


uint8_t esp8266Init (void) {

	uint8_t espStatus = 0;

	espHandler.mode = ESP_NOT_TRANSPARENT;
	uartTxDataQueueSend((uint8_t*)"+++", 3);
	osDelay(200);
	esp8266ExecuteCmd(cmds[AT]);
	osDelay(200);
	espStatus = esp8266GetStatus();
	osDelay(100);
	printf ("ESP STATUS = %d\r\n", espStatus);
	if ((espStatus == 20) || (espStatus == 21) || (espStatus == 25)) {
		esp8266WifiConnect(WIFI_SSID, WIFI_PASS, 15000, 1);
	}
	if (espStatus != 23) {
		if(esp8266SetServer(5000, 1) == ESP_REPLY_CONNECTED){
		}
	} else if ((espStatus == 23) || (espStatus == 24)){
		uartTxDataQueueSend((uint8_t*)"AT+CIPCLOSE\r\n", 13);
		osDelay(300);
		esp8266SetServer(5000, 1);
	}
	esp8266ExecuteCmd(cmds[CIPMODE]);
	osDelay(100);
	esp8266ExecuteCmd(cmds[CIPSEND]);
	osDelay(100);
	espHandler.mode = ESP_TRANSPARENT;
	return 1;
}

uint16_t esp8266parcer (uartRxData_t * rxData) {

	uint16_t reply = ESP_REPLY_NONE;
	static uint8_t wifiConnectState = 0;

	if ((rxData->size == 10) && (strncmp((const char *)(rxData->buffer), "AT", 2 ) == 0) && (strncmp((const char *)(rxData->buffer + 6), "OK", 2 ) == 0)) {
		reply = ESP_REPLY_OK;
		return reply;
	}
	switch (wifiConnectState) {
	case 0:
		if ((rxData->size == 13) && (strncmp((const char *)(rxData->buffer), "WIFI", 4 ) == 0) && (strncmp((const char *)(rxData->buffer + 9), "IP", 2 ) == 0)){
			wifiConnectState++;
			break;
		} else if ((rxData->size == 18) && (strncmp((const char*)(rxData->buffer),"+CWJAP", 6) == 0) && (strncmp((const char*)(rxData->buffer + 12), "FAIL", 4) == 0)) {
			reply = ESP_REPLY_WIFI_CONN_FAIL;
			espHandler.isWifiConnected = 0;
			return reply;
		}
		break;
	case 1:
		if ((rxData->size == 6) && (strncmp((const char *)(rxData->buffer + 2), "OK", 2) == 0)){
			reply = ESP_REPLY_CWJAP;
			espHandler.isWifiConnected = 1;
			wifiConnectState = 0;
			return reply;
		}
		break;
	}

	if ((rxData->size >= 30) && (strncmp((const char*)(rxData->buffer + 14),"STATUS:", 7) == 0)) {
		reply = (uint16_t) atoi ((const char*)(rxData->buffer + 21)) + 20;
		return reply;
	}

	if ((rxData->size == 15) && (strncmp((const char *)(rxData->buffer), "CONNECT", 7) == 0) && (strncmp((const char *)(rxData->buffer + 11), "OK", 2 ) == 0)) {
		reply = ESP_REPLY_CONNECTED;
		return reply;
	} else if ((rxData->size == 17) && (strncmp((const char *)(rxData->buffer + 2), "ERROR", 5 ) == 0) && (strncmp((const char *)(rxData->buffer + 9), "CLOSED", 6 ) == 0)) {
		reply = ESP_REPLY_SERVER_CLOSED;
		return reply;
	}

	if ((rxData->size == 20) && (strncmp((const char *)(rxData->buffer), "AT+CIPMODE", 10 ) == 0) && (strncmp((const char *)(rxData->buffer + 16), "OK", 2 ) == 0)) {
		reply = ESP_REPLY_CIPMODE;
		return reply;
	}

	if ((rxData->size == 21) && (strncmp((const char *)(rxData->buffer), "AT+CIPSEND", 10 ) == 0) &&(rxData->buffer[20] == '>')){
		reply = ESP_REPLY_CIPSEND;
		return reply;
	}

	memset ((uint8_t*)espHandler.uartRxBuf.buffer, 0, UART2RX_BUFFER_SIZE);
	return reply;
}

uint16_t esp8266ExecuteCmd(esp8266_cmd_t cmd) {

	uint16_t reply = ESP_REPLY_NONE;
	osStatus_t result;

	do {
		uartTxDataQueueSend((uint8_t*)cmd.data, cmd.len);
		printf ("\r\nCMD %s has sent. Expected response is --%d--\r\n", cmd.data, cmd.expect_retval);
		if ((result = osMessageQueueGet(replyQHandle, &reply, 0, cmd.timeout)) == osOK) {
			if (reply == cmd.expect_retval) {
				printf ("---- Reply %d ----\r\n", reply);
				return reply;
			}
		} else if (result == osErrorTimeout) {
			reply = ESP_REPLY_TIMEOUT;
			printf ("\r\nCmd  %s timed out\r\n", cmd.data);
		}
	} while (--cmd.retry_count);
	return reply;
}

uint16_t esp8266GetStatus(void) {

	uint16_t reply = ESP_REPLY_NONE;
	osStatus_t result;
	uint8_t retryCount = 10;

	do {
		uartTxDataQueueSend((uint8_t*)"AT+CIPSTATUS\r\n", 14);
		if ((result = osMessageQueueGet(replyQHandle, &reply, 0, 2000)) == osOK) {
			printf ("---- ESP STATUS %d ----\r\n", reply - 20);
			return reply;
		} else if (result == osErrorTimeout) {
			reply = ESP_REPLY_TIMEOUT;
			puts ("\r\nGET STATUS timed out\r\n");
		}
	} while (--retryCount);
	return reply;
}

uint16_t esp8266SetServer(uint16_t waitMs, int8_t retryCount) {

	uint8_t startConnectString[64];
	uint16_t reply = ESP_REPLY_NONE;
	uint16_t len = sprintf((char*) startConnectString, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", LOG_SERVER, LOG_PORT);
	osStatus_t result;

	do {
		uartTxDataQueueSend(startConnectString, len);
		printf("CMD sent: %s\r\n", startConnectString);
		if ((result = osMessageQueueGet(replyQHandle, &reply, 0, waitMs)) == osOK) {
			if (reply == ESP_REPLY_CONNECTED) {
				printf ("-------- Connected to %s, port %d ----------\r\n", LOG_SERVER, LOG_PORT);
				return reply;
			} else if (reply == ESP_REPLY_SERVER_CLOSED) {
				printf ("-------- SERVER %s, PORT %d CLOSED ----------\r\n", LOG_SERVER, LOG_PORT);
			}
		} else if (result == osErrorTimeout) {
			reply = ESP_REPLY_TIMEOUT;
		}
	} while (--retryCount > 0);
	return reply;
}

uint16_t esp8266WifiConnect(char *ssid, char *password, uint16_t waitMs, int8_t retryCount) {

	uint8_t startConnectString[64];
	uint16_t reply = ESP_REPLY_NONE;
	uint16_t len = sprintf((char*) startConnectString, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
	osStatus_t result;

	do {
		uartTxDataQueueSend(startConnectString, len);
		if ((result = osMessageQueueGet(replyQHandle, &reply, 0, waitMs)) == osOK) {
			if (reply == ESP_REPLY_CWJAP) {
				printf ("-------- Wifi connected to AP %s --------\r\n", ssid);
				return reply;
			} else if (reply == ESP_REPLY_WIFI_CONN_FAIL) {
				puts ("-------- Wifi connect FAIL --------\r\n");
				return reply;
			}
		} else if (result == osErrorTimeout) {
			reply = ESP_REPLY_TIMEOUT;
			puts ("------ WIFI CONNECT TIMEOUT ------\r\n");
		}
	} while (--retryCount > 0);
	return reply;
}

uint8_t uartTxDataQueueSend (uint8_t * buf, uint16_t size) {

	uartTxData_t data = {0};

	taskENTER_CRITICAL();
	memcpy((uint8_t*) data.buffer, (const uint8_t *) buf, size);
	data.len = size;
	taskEXIT_CRITICAL();
	if (osMessageQueuePut(uartTxQueueHandle, &data , 0, 500) == osOK){
		return 1;
	} else {
		return 0;
	}
}

uint8_t uartTxDataQueueSendFromIsr (uint8_t * buf, uint16_t size) {

	uartTxData_t data = {0};

	memcpy((uint8_t*) data.buffer, (const uint8_t *) buf, size);
	data.len = size;
	if (osMessageQueuePut(uartTxQueueHandle, &data , 0, 0) == osOK){
		return 1;
	} else {
		return 0;
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == USART2) {
		espHandler.uartRxBuf.size = Size;
		osMessageQueuePut(uartRxQueueHandle, (const void*)&espHandler.uartRxBuf, 0, 0);
		HAL_UARTEx_ReceiveToIdle_DMA(huart, espHandler.uartRxBuf.buffer, UART2RX_BUFFER_SIZE);
	}
}

void transparentParcer (uartRxData_t * data) {

	if ((data->size == 4) && (strncmp((const char*)data->buffer, "pong", 4) == 0)){
		printf("pong received\r\n");
		osTimerStop(espTimerHandle);
	}
	if ((data->size == 9) && (strncmp((const char*)data->buffer, "reset", 5) == 0)) {
		printf("reset received\r\n");
	}
	memset ((uint8_t*)espHandler.uartRxBuf.buffer, 0, UART2RX_BUFFER_SIZE);
	}

void parcerTask(void *argument) {
	uartRxData_t data;
	uint16_t parcerResponse = 0;

	for (;;) {
		if (osMessageQueueGet(uartRxQueueHandle, (uartRxData_t*)&data, 0, osWaitForever) == osOK){
			espSecCounter = 0;
			if (espHandler.mode == ESP_NOT_TRANSPARENT) {
				if ((parcerResponse = esp8266parcer(&data)) != 0){
					osMessageQueuePut(replyQHandle, &parcerResponse, 0, 100);
				}
			} else if (espHandler.mode == ESP_TRANSPARENT){
				transparentParcer(&data);
			}
		}
		osThreadYield();
	}
}

void uartTxTask(void *argument) {
	uartTxData_t data = {0};

	for (;;){
		if (osMessageQueueGet(uartTxQueueHandle, (uartTxData_t *) &data, 0, osWaitForever) == osOK){
			HAL_UART_Transmit_DMA(&huart2, (const uint8_t *)&data.buffer, data.len);
			osThreadYield();
		}
	}
}
