/******************************************************************************
* File Name:   http_client.c
*
* Description: This file contains task and functions related to HTTP client
* operation.
*
*******************************************************************************
* (c) 2019-2020, Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress's integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*******************************************************************************/

/* Header file includes. */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* FreeRTOS header file. */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Standard C header file. */
#include <string.h>

/* Cypress secure socket header file. */
#include "cy_secure_sockets.h"

/* Wi-Fi connection manager header files. */
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* TCP client task header file. */
#include "flowmeter.h"

/* HTTP Client Library*/
#include "cy_http_client_api.h"

#include "alarm.h"

#include "eeprom.h"

/*******************************************************************************
* Macros
********************************************************************************/
#define BUFFERSIZE							(2048 * 2)
#define SENDRECEIVETIMEOUT					(5000)
#define HTMLRESOURCE						"/html"
//#define ANYTHINGRESOURCE					"estorem.highmindsgroup.com/psoc/sample.php"

#define DEBUG_PIN_LED

#ifdef DEBUG_PIN_LED
#define RELAY_PIN CYBSP_USER_LED
#else
#define RELAY_PIN P12_0
#endif

/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t connect_to_wifi_ap(void);
void disconnect_callback(void *arg);

/*******************************************************************************
* Global Variables
********************************************************************************/
bool connected;
uint32_t newflowrate;
uint32_t bodyLength;
volatile int counterpulse_curr = 0;
volatile int counterpulse_diff = 0;
extern QueueHandle_t flow_eepromQ;
extern QueueHandle_t flow_cloudQ;

/*******************************************************************************
 * Function Name: http_client_task
 *******************************************************************************
 * Summary:
 *  Task used to establish a connection to a remote TCP server and
 *  control the LED state (ON/OFF) based on the command received from TCP server.
 *
 * Parameters:
 *  void *args : Task parameter defined during task creation (unused).
 *
 * Return:
 *  void
 *
 *******************************************************************************/


#define TRANSPORT_SEND_RECV_TIMEOUT_MS    ( 5000 )
#define GET_PATH                        "/tdsApi/readflow.php"
#define USER_BUFFER_LENGTH                ( 2048 )
#define REQUEST_BODY                    "{\"status\":\"ON\",\"device\":\"123\"}"
#define REQUEST_BODY_LENGTH        ( sizeof( REQUEST_BODY ) - 1 )

static cy_rslt_t http_client_create(cy_http_client_t* handle,cy_awsport_server_info_t* server_info)
{
	cy_rslt_t res = CY_RSLT_SUCCESS;

	server_info->host_name = "tds.technomindsindia.com";
	server_info->port = (80);
	/* Initialize the HTTP Client Library. */
	res = cy_http_client_init();
	if( res != CY_RSLT_SUCCESS )
	{
#ifdef DEBUG_ENABLE
		printf("HTTP Client Library Initialization Failed!\n");
#endif		
		return res;
	}
	/* Create an instance of the HTTP client. */
	res = cy_http_client_create(NULL, server_info, NULL, NULL, handle);
	if( res != CY_RSLT_SUCCESS )
	{
		res = cy_http_client_deinit();
		if( res != CY_RSLT_SUCCESS )
		{
#ifdef DEBUG_ENABLE
			printf("cy_http_client_deinit Failed!\n");
#endif			
			return res;
		}		
#ifdef DEBUG_ENABLE
		printf("HTTP Client Creation Failed!\n");
#endif		
		return res;
	}

	return res;

}

static cy_rslt_t log_flowrate(cy_http_client_t handle, char* str_flowrate, uint32_t length)
{
	uint8_t userBuffer[ USER_BUFFER_LENGTH ];
	cy_http_client_request_header_t request;
	cy_http_client_header_t header[2];
	cy_rslt_t res = CY_RSLT_SUCCESS;
	uint32_t num_header;
	cy_http_client_response_t response;

	/* Connect the HTTP client to the server. */
	res = cy_http_client_connect(handle, TRANSPORT_SEND_RECV_TIMEOUT_MS, TRANSPORT_SEND_RECV_TIMEOUT_MS);
	if( res != CY_RSLT_SUCCESS )
	{
#ifdef DEBUG_ENABLE
		printf("HTTP Client Connection Failed!\n");
#endif		
		return res;
	}
	else
	{
#ifdef DEBUG_ENABLE
		printf("\nConnected to HTTP Server Successfully logflow\n\n");
#endif
	}

	request.buffer = userBuffer;
	request.buffer_len = USER_BUFFER_LENGTH;
	request.headers_len = 0;
	request.method = CY_HTTP_CLIENT_METHOD_POST;
	request.range_end = -1;
	request.range_start = 0;
	request.resource_path = GET_PATH;
	header[0].field = "Connection";
	header[0].field_len = strlen("Connection");
	header[0].value = "keep-alive";
	header[0].value_len = strlen("keep-alive");
	header[1].field = "Content-Type";
	header[1].field_len = strlen("Content-Type");
	header[1].value = "application/x-www-form-urlencoded";
	header[1].value_len = strlen("application/x-www-form-urlencoded");
	num_header = 2;
	/* Generate the standard header and user-defined header, and update in the request structure. */
	res = cy_http_client_write_header(handle, &request, &header[0], num_header);
	if( res != CY_RSLT_SUCCESS )
	{
#ifdef DEBUG_ENABLE
		printf("HTTP Client Header Write Failed!\n");
#endif		
		return res;
	}
	/* Send the HTTP request and body to the server, and receive the response from it. */
	res = cy_http_client_send(handle, &request, (uint8_t *)str_flowrate, length, &response);
	if( res != CY_RSLT_SUCCESS )
	{
#ifdef DEBUG_ENABLE
		printf("HTTP Client Send Failed!\n");
#endif		
		return res;
	}

#ifdef DEBUG_ENABLE
	printf("\nResponse received from HTTP Client:\n");
		for(int i = 0; i < response.body_len; i++){
			printf("%c", response.body[i]);
		}
		printf("\n");
#endif

	( void ) memset( &header[0], 0, sizeof( header[0] ) );
	header[0].field = "Connection";
	header[0].field_len = strlen("Connection");
	header[1].field = "Content-Type";
	header[1].field_len = strlen("Content-Type");
	num_header = 2;
	/* Read the header value from the HTTP response. */
	res = cy_http_client_read_header(handle, &response, &header[0], num_header);
	if( res != CY_RSLT_SUCCESS )
	{
		return res;
	}
	/* Disconnect the HTTP client from the server. */
	res = cy_http_client_disconnect(handle);
	if( res != CY_RSLT_SUCCESS )
	{
		return res;
	}

	return res;
}

static cy_rslt_t delete_http_client(cy_http_client_t handle)
{
	cy_rslt_t res = CY_RSLT_SUCCESS;

	/* Delete the instance of the HTTP client. */
	res = cy_http_client_delete(handle);
	if( res != CY_RSLT_SUCCESS )
	{
		return res;
	}
	
	/* Deinit the HTTP library. */
	res = cy_http_client_deinit();
	if( res != CY_RSLT_SUCCESS )
	{
		return res;
	}

	return res;
}

void flowmeter_logger(void *arg){

	float adc_out=0;
	float newflowrate=0;
	float flowrate=0;
	char device_id[10] = {0};
	int counter = 0;
	uint8_t* ptr;

	//read device ID from EEPROM
	readDeviceID((uint8_t*)&device_id[0],8);

	readFlowData((uint8_t*)&flowrate,sizeof(float));
	ptr=(uint8_t*)&flowrate;
	if(*ptr == 0xFF &&  *(ptr+1) == 0xFF && *(ptr+2) == 0xFF && *(ptr+3) == 0xFF)
	{
		flowrate = 0.0;
	}

	printf("today flow %f\r\n",flowrate);

#ifdef DEBUG_ENABLE
	printf("devID 1 %s\n",device_id);
#endif

	while(1)
	{
		counter++;
		// Poll the flowmeter once a second
		cyhal_system_delay_ms(1000);

		adc_out = (float)(counterpulse_curr)/ (float)73 ;
		counterpulse_curr = 0;
		
		newflowrate =  adc_out/60;
		
		flowrate = flowrate + newflowrate;


		if((counter%10) == 0)	//write flow data on every 10 seconds
		{
			xQueueSendToBack(flow_eepromQ,(const void*)&flowrate,pdMS_TO_TICKS(200));
		}

		if(counter == 60)	//log data to table every 5 minutes
		{
			counter = 0;
			xQueueSendToBack(flow_cloudQ,(const void*)&flowrate,pdMS_TO_TICKS(200));
		}
	}
}

/*******************************************************************************
 * Function Name: disconnect_callback
 *******************************************************************************
 * Summary:
 *  Invoked when the server disconnects
 *
 * Parameters:
 *  void *arg : unused
 *
 * Return:
 *  void
 *
 *******************************************************************************/
void disconnect_callback(void *arg){
    printf("Disconnected from HTTP Server\n");
    connected = false;
}

void gpio_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event)
{
	counterpulse_curr++;
}

void flow_cloud(void* arg){
	char eventValue[256];
	cy_rslt_t res = CY_RSLT_SUCCESS;
	cy_http_client_t handle;
	cy_awsport_server_info_t server_info;
	bool gpiostate = 0;
	int tds = 0;
	char stat[2][4] = {"ON","OFF"};
	char device_id[10] = {0};
	float flowrate=0;
	uint32_t bodyLength=0;

	//read device ID from EEPROM
	readDeviceID((uint8_t*)&device_id[0],8);

	while(1){

		xQueueReceive(flow_cloudQ,(void * const)&flowrate,portMAX_DELAY);

		res = CY_RSLT_SUCCESS;

		gpiostate = cyhal_gpio_read(RELAY_PIN);

#ifdef DEBUG_ENABLE
printf("sending flowrate: %f.\n", flowrate);
#endif

		tds = rand() % 100;
		sprintf(eventValue, "action=UPDATE_FLOW&device_id=%s&litre=%f&tds=%d&status=%s",device_id,flowrate,tds,stat[gpiostate]);
		bodyLength = strlen(eventValue);

		//create client
		res = http_client_create(&handle, &server_info);
		if(res != CY_RSLT_SUCCESS)
		{
			continue;
		}

		res = log_flowrate(handle,eventValue,bodyLength);
		if(res != CY_RSLT_SUCCESS)
		{
	#ifdef DEBUG_ENABLE
			printf("log flow rate failed %d\n",res);
	#endif
		}

		res = delete_http_client(handle);
		if(res != CY_RSLT_SUCCESS)
		{
	#ifdef DEBUG_ENABLE
			printf("delete_http_client failed %d\n",res);
	#endif
		}
	}
}

void flow_eeprom(void* arg){
	float flowrate=0;

	while(1){

		xQueueReceive(flow_eepromQ,(void * const)&flowrate,portMAX_DELAY);
		writeFlowData((uint8_t*)&flowrate,sizeof(float));
#if DEBUG_ENABLE
		printEEPROMContent();
#endif
	}
}

