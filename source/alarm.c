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
#include "alarm.h"

/* HTTP Client Library*/
#include "cy_http_client_api.h"

#include "cy_em_eeprom.h"

#include "cJSON.h"

#include "cy_json_parser.h"


/////////////////////////////////////   EEPROM  /////////////////////////////////////////////

/*******************************************************************************
 * Macros
 ******************************************************************************/
/* Logical Size of Emulated EEPROM in bytes. */
int LOGICAL_EEPROM_SIZE=250;



#define LOGICAL_EEPROM_START    (0u)

/* Location of reset counter in Em_EEPROM. */
#define RESET_COUNT_LOCATION    (13u)
/* Size of reset counter in bytes. */
#define RESET_COUNT_SIZE        (2u)

/* ASCII "9" */
#define ASCII_NINE              (0x39)

/* ASCII "0" */
#define ASCII_ZERO              (0x30)

/* ASCII "P" */
#define ASCII_P                 (0x7b)

/* EEPROM Configuration details. All the sizes mentioned are in bytes.
 * For details on how to configure these values refer to cy_em_eeprom.h. The
 * library documentation is provided in Em EEPROM API Reference Manual. The user
 * access it from ModusToolbox IDE Quick Panel > Documentation>
 * Cypress Em_EEPROM middleware API reference manual
 */
#define EEPROM_SIZE             (256u)
#define BLOCKING_WRITE          (1u)
#define REDUNDANT_COPY          (1u)
#define WEAR_LEVELLING_FACTOR   (2u)
#define SIMPLE_MODE             (0u)

/* Set the macro FLASH_REGION_TO_USE to either USER_FLASH or
 * EMULATED_EEPROM_FLASH to specify the region of the flash used for
 * emulated EEPROM.
 */
#define USER_FLASH              (0u)
#define EMULATED_EEPROM_FLASH   (1u)

#if ((defined(TARGET_CY8CKIT_062S4))||(defined(TARGET_CY8CPROTO_064B0S3)))
/* The target kit CY8CKIT-062S4 and CY8CPROTO-064B0S3 doesn't have a dedicated EEPROM
 * flash region so this example will demonstrate emulation in the user flash region
 */
#define FLASH_REGION_TO_USE     USER_FLASH
#else
#define FLASH_REGION_TO_USE     EMULATED_EEPROM_FLASH
#endif

#define GPIO_LOW                (0u)

#define DEBUG_PIN_LED

#ifdef DEBUG_PIN_LED
#define RELAY_PIN CYBSP_USER_LED
#else
#define RELAY_PIN P12_0
#endif
/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/


/*******************************************************************************
 * Global variables
 ******************************************************************************/
/* EEPROM configuration and context structure. */
cy_stc_eeprom_config_t Em_EEPROM_config =
{
        .eepromSize = EEPROM_SIZE,
        .blockingWrite = BLOCKING_WRITE,
        .redundantCopy = REDUNDANT_COPY,
        .wearLevelingFactor = WEAR_LEVELLING_FACTOR,
};


cy_stc_eeprom_context_t Em_EEPROM_context;

#if (EMULATED_EEPROM_FLASH == FLASH_REGION_TO_USE)
CY_SECTION(".cy_em_eeprom")
#endif /* #if(FLASH_REGION_TO_USE) */
CY_ALIGN(CY_EM_EEPROM_FLASH_SIZEOF_ROW)

#if ((defined(TARGET_CY8CKIT_064B0S2_4343W)||(defined(TARGET_CY8CPROTO_064B0S3))) && (USER_FLASH == FLASH_REGION_TO_USE ))
/* When CY8CKIT-064B0S2-4343W and CY8CPROTO-064B0S3 is selected as the target and EEPROM array is
 * stored in user flash, the EEPROM array is placed in a fixed location in
 * memory. The adddress of the fixed location can be arrived at by determining
 * the amount of flash consumed by the application. In this case, the example
 * consumes approximately 104000 bytes for the above target using GCC_ARM
 * compiler and Debug configuration. The start address specified in the linker
 * script is 0x10000000, providing an offset of approximately 32 KB, the EEPROM
 * array is placed at 0x10021000 in this example. Note that changing the
 * compiler and the build configuration will change the amount of flash
 * consumed. As a resut, you will need to modify the value accordingly. Among
 * the supported compilers and build configurations, the amount of flash
 * consumed is highest for GCC_ARM compiler and Debug build configuration.
 */
#define APP_DEFINED_EM_EEPROM_LOCATION_IN_FLASH  (0x10021000)
#else
/* EEPROM storage in user flash or emulated EEPROM flash. */
const uint8_t EepromStorage[CY_EM_EEPROM_GET_PHYSICAL_SIZE(EEPROM_SIZE, SIMPLE_MODE, WEAR_LEVELLING_FACTOR, REDUNDANT_COPY)] = {0u};

#endif /* #if (defined(TARGET_CY8CKIT_064B0S2_4343W)) */

/* RAM arrays for holding EEPROM read and write data respectively. */

/////////////////////////////////////   EEPROM  END /////////////////////////////////////////////////////

/////////////////////////////////////   RTC Module  /////////////////////////////////////////////////////
/*******************************************************************************
* Macros
*******************************************************************************/

#define UART_TIMEOUT_MS (10u)      /* in milliseconds */
#define INPUT_TIMEOUT_MS (120000u) /* in milliseconds */

#define STRING_BUFFER_SIZE (80)

/* Available commands */
#define RTC_CMD_SET_DATE_TIME ('1')
#define RTC_CMD_CONFIG_DST ('2')

#define RTC_CMD_ENABLE_DST ('1')
#define RTC_CMD_DISABLE_DST ('2')
#define RTC_CMD_QUIT_CONFIG_DST ('3')

#define FIXED_DST_FORMAT ('1')
#define RELATIVE_DST_FORMAT ('2')

/* Macro used for checking validity of user input */
#define MIN_SPACE_KEY_COUNT (5)

/* Structure tm stores years since 1900 */
#define TM_YEAR_BASE (1900u)

/* Maximum value of seconds and minutes */
#define MAX_SEC_OR_MIN (60u)

/* Maximum value of hours definition */
#define MAX_HOURS_24H (23UL)

/* Month per year definition */
#define MONTHS_PER_YEAR (12U)

/* Days per week definition */
#define DAYS_PER_WEEK (7u)

/* Days in month */
#define DAYS_IN_JANUARY (31U)   /* Number of days in January */
#define DAYS_IN_FEBRUARY (28U)  /* Number of days in February */
#define DAYS_IN_MARCH (31U)     /* Number of days in March */
#define DAYS_IN_APRIL (30U)     /* Number of days in April */
#define DAYS_IN_MAY (31U)       /* Number of days in May */
#define DAYS_IN_JUNE (30U)      /* Number of days in June */
#define DAYS_IN_JULY (31U)      /* Number of days in July */
#define DAYS_IN_AUGUST (31U)    /* Number of days in August */
#define DAYS_IN_SEPTEMBER (30U) /* Number of days in September */
#define DAYS_IN_OCTOBER (31U)   /* Number of days in October */
#define DAYS_IN_NOVEMBER (30U)  /* Number of days in November */
#define DAYS_IN_DECEMBER (31U)  /* Number of days in December */

/* Flags to indicate the if the entered time is valid */
#define DST_DISABLED_FLAG (0)
#define DST_VALID_START_TIME_FLAG (1)
#define DST_VALID_END_TIME_FLAG (2)
#define DST_ENABLED_FLAG (3)

/* Macro to validate seconds parameter */
#define IS_SEC_VALID(sec) ((sec) <= MAX_SEC_OR_MIN)

/* Macro to validate minutes parameters */
#define IS_MIN_VALID(min) ((min) <= MAX_SEC_OR_MIN)

/* Macro to validate hour parameter */
#define IS_HOUR_VALID(hour) ((hour) <= MAX_HOURS_24H)

/* Macro to validate month parameter */
#define IS_MONTH_VALID(month) (((month) > 0U) && ((month) <= MONTHS_PER_YEAR))

/* Macro to validate the year value */
#define IS_YEAR_VALID(year) ((year) > 0U)

/* Checks whether the year passed through the parameter is leap or not */
#define IS_LEAP_YEAR(year) (((0U == (year % 4UL)) && (0U != (year % 100UL))) || (0U == (year % 400UL)))

/* RTOS related macros. */
#define RTC_TASK_STACK_SIZE        (4 * 1024)
#define RTC_TASK_PRIORITY          (1)

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

//static void set_new_time(uint32_t timeout_ms);

//static int set alarm(uint32_t  timeout_ms);

static bool validate_date_time(int sec, int min, int hour, int mday, int month, int year);
//static int get_day_of_week(int day, int month, int year);
//static int set alarm(uint32_t  timeout_ms);

//static cy_rslt_t fetch_time_data(char *buffer, uint32_t timeout_ms, uint32_t *space_count);


/*******************************************************************************
* Global Variables
*******************************************************************************/

cyhal_rtc_t rtc_obj;
uint32_t dst_data_flag = 0;
TaskHandle_t rtctask_handle;
/*******************************************************************************
* Function Name: handle_error
********************************************************************************
* Summary:
* User defined error handling function
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void handle_error(void)
{
    /* Disable all interrupts. */
    __disable_irq();

    CY_ASSERT(0);
}

/////////////////////////////////////   RTC Module END /////////////////////////////////////////////////

/////////////////////////////////////   HTTP Client ////////////////////////////////////////////////////

/*******************************************************************************
* Macros
********************************************************************************/
#define BUFFERSIZE							(2048 * 2)
#define SENDRECEIVETIMEOUT					(5000)
#define HTMLRESOURCE						"/html"
//#define ANYTHINGRESOURCE					"estorem.highmindsgroup.com/psoc/sample.php"

/*******************************************************************************
* Function Prototypes
********************************************************************************/


/*******************************************************************************
* Global Variables
********************************************************************************/


#define TRANSPORT_SEND_RECV_TIMEOUT_MS    ( 5000 )
#define GET_PATH                          "/tdsApi/readts.php"
#define USER_BUFFER_LENGTH                ( 2048 )
#define REQUEST_BODY_ALL                    "action=GET_ALL&ID=400214"
#define REQUEST_BODY_ALL_LENGTH        ( sizeof( REQUEST_BODY_ALL_LENGTH ) - 1 )
#define TRUE true
#define FALSE false
/////////////////////////////////////   HTTP Client End /////////////////////////////////////////////////

char save_response[250];
char save_memory[250];
int year = 0, month = 0, day = 0, hour = 0, minute = 0, sec = 0;
int current_day=0, current_month=0, current_year=0, current_sec=0, current_min=0, current_hour=0;
uint8_t control_status = 0;

bool updateEEPROM_flag = FALSE;
bool updateRTC_flag = FALSE;
bool control_flag = FALSE;
bool booting = TRUE;
bool update_inprog = FALSE;
bool alarm_hit = FALSE;

static void getyear(char *value,int *year)
{
	int i = 0;
	int sum = 0;
	int k = 0;

	for(i = 0; i < 4;i++)
	{
		k = value[i] - 0x30;
		sum = (sum * 10) + k;
	}
	*year = sum;
}

static void getmonth(char *value, int *month)
{
	int i = 0;
	int sum = 0;
	int k = 0;

	for(i = 5; i < 7;i++)
	{
		k = value[i] - 0x30;
		sum = (sum * 10) + k;
	}
	*month = sum;
}

static void getday(char *value,int *day)
{
	int i = 0;
	int sum = 0;
	int k = 0;

	for(i = 8; i < 10;i++)
	{
		k = value[i] - 0x30;
		sum = (sum * 10) + k;
	}
	*day = sum;
}

static void gethour(char *value,int *hour)
{
	int i = 0;
	int sum = 0;
	int k = 0;

	for(i = 0; i < 2;i++)
	{
		k = value[i] - 0x30;
		sum = (sum * 10) + k;
	}
	*hour = sum;
}

static void getminute(char* value,int *minute)
{
	int i = 0;
	int sum = 0;
	int k = 0;

	for(i = 3; i < 5;i++)
	{
		k = value[i] - 0x30;
		sum = (sum * 10) + k;
	}
	*minute = sum;
}

static void getsecond(char* value,int *second)
{
	int i = 0;
	int sum = 0;
	int k = 0;

	for(i = 6; i < 8;i++)
	{
		k = value[i] - 0x30;
		sum = (sum * 10) + k;
	}
	*second = sum;
}

static void HandleError(uint32_t status, char *message)
{

    if(CY_EM_EEPROM_SUCCESS != status)
    {
        if(CY_EM_EEPROM_REDUNDANT_COPY_USED != status)
        {
            cyhal_gpio_write((cyhal_gpio_t) CYBSP_USER_LED, false);
            __disable_irq();

            if(NULL != message)
            {
                printf("%s",message);
            }

            while(1u);
        }
        else
        {
            printf("%s","Main copy is corrupted. Redundant copy in Emulated EEPROM is used \r\n");
        }

    }
}

/*******************************************************************************
* Function Name: validate_date_time
********************************************************************************
* Summary:
*  This function validates date and time value.
*
* Parameters:
*  uint32_t sec     : The second valid range is [0-59].
*  uint32_t min     : The minute valid range is [0-59].
*  uint32_t hour    : The hour valid range is [0-23].
*  uint32_t date    : The date valid range is [1-31], if the month of February
*                     is selected as the Month parameter, then the valid range
*                     is [0-29].
*  uint32_t month   : The month valid range is [1-12].
*  uint32_t year    : The year valid range is [> 0].
*
* Return:
*  false - invalid ; true - valid
*
*******************************************************************************/
static bool validate_date_time(int sec, int min, int hour, int mday, int month, int year)
{
    static const uint8_t days_in_month_table[MONTHS_PER_YEAR] =
        {
            DAYS_IN_JANUARY,
            DAYS_IN_FEBRUARY,
            DAYS_IN_MARCH,
            DAYS_IN_APRIL,
            DAYS_IN_MAY,
            DAYS_IN_JUNE,
            DAYS_IN_JULY,
            DAYS_IN_AUGUST,
            DAYS_IN_SEPTEMBER,
            DAYS_IN_OCTOBER,
            DAYS_IN_NOVEMBER,
            DAYS_IN_DECEMBER,
        };

    uint8_t days_in_month;

    bool rslt = IS_SEC_VALID(sec) & IS_MIN_VALID(min) &
                IS_HOUR_VALID(hour) & IS_MONTH_VALID(month) &
                IS_YEAR_VALID(year);

    if (rslt)
    {
        days_in_month = days_in_month_table[month - 1];

        if (IS_LEAP_YEAR(year) && (month == 2))
        {
            days_in_month++;
        }

        rslt &= (mday > 0U) && (mday <= days_in_month);
    }

    return rslt;
}

static cy_rslt_t parse_json_snippet_callback (cy_JSON_object_t* json_object, void *arg )
{
	int lyear = 0;
	int lmonth = 0;
	int lday = 0;
	int lhour = 0;
	int lminute = 0;
	int lsec = 0;
	char lcntrl[4] = {0};
	uint8_t lcntrl_stat = 0;

	switch(json_object->value_type)
	{
		case JSON_STRING_TYPE:
		{
#if 0
			printf("Found a string: %.*s\n",json_object->object_string_length,json_object->object_string  );
			printf("Found a key: %.*s\n", json_object->value_length,json_object->value );
#endif
			if((strncmp(json_object->object_string, "device_date", strlen("device_date")) == 0))
			{
				getyear(json_object->value,&lyear);
				getmonth(json_object->value,&lmonth);
				getday(json_object->value,&lday);

				if(booting == TRUE)
				{
					year = lyear;month = lmonth;day = lday;
				}
				else
				{
					if(lyear != year || lmonth != month || lday != day)
					{
						hour = lhour;minute = lminute;sec = lsec;
						updateEEPROM_flag = TRUE;
					}
				}

				printf("year %d\n",lyear);
				printf("month %d\n",lmonth);
				printf("day %d\n",lday);
				printf("\r\n");
			}

			if((strncmp(json_object->object_string, "device_time", strlen("device_time")) == 0))
			{
				gethour(json_object->value,&lhour);
				getminute(json_object->value,&lminute);
				getsecond(json_object->value,&lsec);

				if(booting == TRUE)
				{
					hour = lhour;minute = lminute;sec = lsec;
				}
				else
				{
					if(lhour != hour || lminute != minute || lsec != sec)
					{
						hour = lhour;minute = lminute;sec = lsec;
						updateEEPROM_flag = TRUE;
					}
				}

				printf("hour %d\n",lhour);
				printf("minute %d\n",lminute);
				printf("sec %d\n",lsec);
				printf("\r\n");
			}

			if((strncmp(json_object->object_string, "device_status", strlen("device_status")) == 0))
			{
				if(booting == TRUE)
				{
					strncpy(lcntrl,json_object->value,3);
					if(strncmp (lcntrl, "ON" , strlen("ON")) == 0)
					{
						lcntrl_stat = 1;
					}
					else if(strncmp (lcntrl, "OFF" , strlen("OFF")) == 0)
					{
						lcntrl_stat = 0;
					}
					else
					{
						lcntrl_stat = 0xFF;
					}
					control_status = lcntrl_stat;
				}
				else
				{
					strncpy(lcntrl,json_object->value,3);
					if(strncmp (lcntrl, "ON" , strlen("ON")) == 0)
					{
						lcntrl_stat = 1;
					}
					else if(strncmp (lcntrl, "OFF" , strlen("OFF")) == 0)
					{
						lcntrl_stat = 0;
					}
					else
					{
						lcntrl_stat = 0xFF;
					}

					if(control_status != lcntrl_stat)
					{
						control_status = lcntrl_stat;
						control_flag = TRUE;
					}
				}
			}
			break;
		}
		case JSON_NUMBER_TYPE:
		{
#if 0	//gdb
			printf("Found a string: %.*s\n",json_object->object_string_length,json_object->object_string  );
			printf("Found a number: %ld\n", (unsigned long)json_object->intval);
#endif
			if((strncmp(json_object->object_string, "seconds", strlen("seconds")) == 0))
			{
				current_sec = (int)json_object->intval;
			}

			if((strncmp(json_object->object_string, "minutes", strlen("minutes")) == 0))
			{
				current_min = (int)json_object->intval;
			}

			if((strncmp(json_object->object_string, "hours", strlen("hours")) == 0))
			{
				current_hour = (int)json_object->intval;
			}

			if((strncmp(json_object->object_string, "mday", strlen("mday")) == 0))
			{
				current_day = (int)json_object->intval;
			}

			if((strncmp(json_object->object_string, "mon", strlen("mon")) == 0))
			{
				current_month = (int)json_object->intval;
			}

			if((strncmp(json_object->object_string, "year", strlen("year")) == 0))
			{
			}
				current_year = (int)json_object->intval;
			break;
		}
		case JSON_FLOAT_TYPE:
		{
			printf("Found a float: %f\n",json_object->floatval);
			break;
		}
		case JSON_BOOLEAN_TYPE:
		{
			printf("Found a boolean: %d\n",(unsigned int)json_object->boolval);
			break;
		}
		case JSON_NULL_TYPE:
		{
			printf("Found a NULL type: %.*s\n", json_object->value_length,json_object->value);
			break;
		}
		case JSON_ARRAY_TYPE:
		{
			printf("Found an ARRAY\n");
			break;
		}
		default:
		{
			break;
		}
	}
	return 0;
}

int json_parser_snippet()
{
	cy_JSON_parser_register_callback ( parse_json_snippet_callback, NULL );
	if (cy_JSON_parser(save_memory , strlen(save_memory)) == CY_RSLT_SUCCESS)
	{
		printf("Successfully parsed the JSON input\n");
		return 0;
	}
	else
	{
		printf("Failed to parse the JSON input\n");
		return -1;
	}
}

void init_relay()
{
    cy_rslt_t result;

    /* Initialize the User LED */
    result = cyhal_gpio_init(RELAY_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_ON);
    /* GPIO init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {

    }
}

static void init_RTC(struct tm *date_time)
{
	cy_rslt_t rslt;
	char buffer[STRING_BUFFER_SIZE];

	memset(buffer,0x00,STRING_BUFFER_SIZE);

	/* Initialize RTC */
    rslt = cyhal_rtc_init(&rtc_obj);
    if (CY_RSLT_SUCCESS != rslt)
    {
        handle_error();
    }

	rslt = cyhal_rtc_read(&rtc_obj, date_time);
	if (CY_RSLT_SUCCESS != rslt)
	{
		handle_error();
	}

	printf("RTC init\r\n");
	strftime(buffer, sizeof(buffer), "%c", date_time);
	printf("\r%s\n\n", buffer);
}


static cy_rslt_t http_client_create(cy_http_client_t* handle,cy_awsport_server_info_t* server_info)
{
	cy_rslt_t res = CY_RSLT_SUCCESS;

	server_info->host_name = "tds.technomindsindia.com";
	server_info->port = (80);
	/* Initialize the HTTP Client Library. */
	res = cy_http_client_init();
	if( res != CY_RSLT_SUCCESS )
	{
#ifdef FYI_ENABLE		
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
#ifdef FYI_ENABLE			
			printf("cy_http_client_deinit Failed!\n");
#endif			
			return res;
		}		
#ifdef FYI_ENABLE		
		printf("HTTP Client Creation Failed!\n");
#endif		
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

static cy_rslt_t get_http_response(cy_http_client_t handle, char* req_body, int req_len, bool isSave)
{
	cy_http_client_request_header_t request;
	cy_http_client_header_t header[2];
	uint32_t num_header;
	cy_http_client_response_t response;
	cy_rslt_t res = CY_RSLT_SUCCESS;	
	uint8_t userBuffer[ USER_BUFFER_LENGTH ];

	memset(&response,0x00,sizeof(response));
    
	/* Connect the HTTP client to the server. */
	res = cy_http_client_connect(handle, TRANSPORT_SEND_RECV_TIMEOUT_MS, TRANSPORT_SEND_RECV_TIMEOUT_MS);
	if( res != CY_RSLT_SUCCESS )
	{
		printf("HTTP Client Connection Failed!\n");

	}else{
		printf("\nConnected to HTTP Server Successfully\n\n");

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
		printf("HTTP Client Header Write Failed!\n");

	}

	/* Send the HTTP request and body to the server, and receive the response from it. */
	res = cy_http_client_send(handle, &request, (uint8_t *)req_body, req_len, &response);
	if( res != CY_RSLT_SUCCESS )
	{
		printf("HTTP Client Send Failed!\n");

	}

	printf("\nResponse received from HTTP Client:\n");

	for(int i = 0; i < response.body_len; i++)
	{
		printf("%c", response.body[i]);
	}
	printf("\n");

	if(isSave)
	{
		for(int i = 0; i < response.body_len; i++)
		{
			save_response[i] = response.body[i];
		}
	}

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
		/* Failure path. */
	}
	/* Disconnect the HTTP client from the server. */
	res = cy_http_client_disconnect(handle);
	if( res != CY_RSLT_SUCCESS )
	{
		/* Failure path. */
	}

	return res;
}

static void updateRTC()
{
	cy_rslt_t rslt;
	char buffer[STRING_BUFFER_SIZE] = {0};
	struct tm new_time = {0};

	if (validate_date_time(current_sec, current_min, current_hour, current_day, current_month, current_year))
	{
		new_time.tm_sec = current_sec;
		new_time.tm_min = current_min;
		new_time.tm_hour = current_hour;
		new_time.tm_mday = current_day;
		new_time.tm_mon = current_month - 1;
		new_time.tm_year = current_year - TM_YEAR_BASE;
		//new_time.tm_wday = get_day_of_week(mday, month, year);

		rslt = cyhal_rtc_write(&rtc_obj, &new_time);
		if (CY_RSLT_SUCCESS == rslt)
		{
			printf("\rRTC time updated\r\n\n");
			strftime(buffer, sizeof(buffer), "%c", &new_time);
			printf("\r%s\n\n", buffer);
		}
		else
		{
			printf("error %d\n",rslt);
			handle_error();
		}
	}
	else
	{
		printf("\rInvalid values! Please enter the values in specified"
				" format\r\n");
	}
}

static void updateEEPROM()
{
	cy_en_em_eeprom_status_t eepromReturnValue;
	uint8_t eepromReadArray[LOGICAL_EEPROM_SIZE];
	uint8_t eepromWriteArray[LOGICAL_EEPROM_SIZE];
	int i =0, j =0;

	memset(eepromReadArray,0x00,LOGICAL_EEPROM_SIZE);
	memset(eepromWriteArray,0x00,LOGICAL_EEPROM_SIZE);

	printf("EmEEPROM demo \r\n");

	/* Initialize the flash start address in EEPROM configuration structure. */
#if ((defined(TARGET_CY8CKIT_064B0S2_4343W)||(defined(TARGET_CY8CPROTO_064B0S3))) && (USER_FLASH == FLASH_REGION_TO_USE ))
	Em_EEPROM_config.userFlashStartAddr = (uint32_t) APP_DEFINED_EM_EEPROM_LOCATION_IN_FLASH;
#else
	Em_EEPROM_config.userFlashStartAddr = (uint32_t) EepromStorage;
#endif

	eepromReturnValue = Cy_Em_EEPROM_Init(&Em_EEPROM_config, &Em_EEPROM_context);
	HandleError(eepromReturnValue, "Emulated EEPROM Initialization Error \r\n");

	/* Read 15 bytes out of EEPROM memory. */
	eepromReturnValue = Cy_Em_EEPROM_Read(LOGICAL_EEPROM_START, eepromReadArray,
											LOGICAL_EEPROM_SIZE, &Em_EEPROM_context);
	HandleError(eepromReturnValue, "Emulated EEPROM Read failed \r\n");

	printf("prev\r\n");
	for(i = 0; i < LOGICAL_EEPROM_SIZE ; i++){
		printf("%c",eepromReadArray[i]);
	}
	printf("\r\n");

	read_ID((char*)eepromWriteArray,8);	//read device ID from EEPROM

    for(i = 8, j =0; i < LOGICAL_EEPROM_SIZE; i++, j++){
    	eepromWriteArray[i] = (uint8_t)save_response[j];
    }

	/* Write initial data to EEPROM. */
	eepromReturnValue = Cy_Em_EEPROM_Write(LOGICAL_EEPROM_START,
											eepromWriteArray,
											LOGICAL_EEPROM_SIZE,
											&Em_EEPROM_context);
	HandleError(eepromReturnValue, "Emulated EEPROM Write failed \r\n");

	//////////////////////////////////////////////////////////////////////////////////////

	/* Read contents of EEPROM after write. */
	printf("after\r\n");
	eepromReturnValue = Cy_Em_EEPROM_Read(LOGICAL_EEPROM_START,
											eepromReadArray, LOGICAL_EEPROM_SIZE,
											&Em_EEPROM_context);
	HandleError(eepromReturnValue, "Emulated EEPROM Read failed \r\n" );

	for(i = 0; i < LOGICAL_EEPROM_SIZE ; i++){
		printf("%c",eepromReadArray[i]);
	}
	printf("\r\n");

}

void alarm_task(void *arg){

	cy_rslt_t rslt;
	cy_awsport_server_info_t server_info;
	cy_http_client_t handle;	
	struct tm date_time;
	char devID[10] = {0};
	char req_body[256];
	char stat[2][4] = {"ON","OFF"};
	bool gpiostate = 0;

	memset(req_body,0x00,256);

    init_relay();	//initialize GPIO for relay control

    //Give some delay for flowmeter_logger task to write device ID in Flash
    //This is temporary and it will be removed when device ID is stored in EEPROM
    cyhal_system_delay_ms(10000);

	read_ID(devID,8);	//read device ID from EEPROM

	gpiostate = cyhal_gpio_read(RELAY_PIN);		//take initial state of Relay
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Get server time for RTC

    memset(save_memory,0x00,LOGICAL_EEPROM_SIZE);	//reset JASON parser array before getting data
    memset(save_response,0x00,LOGICAL_EEPROM_SIZE);

	//create client
	rslt = http_client_create(&handle, &server_info);
	if(rslt != CY_RSLT_SUCCESS)
	{
		printf("error http_client_create\r\n");
	}

	//create a request body to get time from server
	sprintf(req_body,"action=GET_TIME");
	rslt = get_http_response(handle,req_body,strlen(req_body),TRUE);
	if(rslt != CY_RSLT_SUCCESS)
	{
		printf("error get_http_response\r\n");
	}

	//delete http client
	rslt = delete_http_client(handle);
	if(rslt != CY_RSLT_SUCCESS)
	{
		printf("error delete_http_client\r\n");
	}

	memcpy(save_memory,save_response,250);		//update save_memory array with response

	json_parser_snippet();

	printf("current_year %d\n",current_year);
	printf("current_month %d\n",current_month);
	printf("current_day %d\n",current_day);
	printf("current_hour %d\n",current_hour);
	printf("current_minute %d\n",current_min);
	printf("current_sec %d\n",current_sec);
	printf("\r\n");

	memset(&date_time, 0x00,sizeof(date_time));
	init_RTC(&date_time);

	//this function will update RTC time to what we got from server
	updateRTC();

	//give some delay
	cyhal_system_delay_ms(2000);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//get all details

    memset(save_memory,0x00,LOGICAL_EEPROM_SIZE);	//reset JASON parser array before getting data
    memset(save_response,0x00,LOGICAL_EEPROM_SIZE);

	//create client
	rslt = http_client_create(&handle, &server_info);
	if(rslt != CY_RSLT_SUCCESS)
	{
		printf("error http_client_create\r\n");
	}

	//get time by creating HTTP request
	read_ID(devID,8);	//read device ID from EEPROM
	sprintf(req_body,"action=GET_ALL&DEVICE_ID=%s",devID);
	rslt = get_http_response(handle,req_body,strlen(req_body),TRUE);
	if(rslt != CY_RSLT_SUCCESS)
	{
		printf("error get_http_response\r\n");
	}

	//delete http client
	rslt = delete_http_client(handle);
	if(rslt != CY_RSLT_SUCCESS)
	{
		printf("error delete_http_client\r\n");
	}

	memcpy(save_memory,save_response,250);		//update save_memory array with response

	json_parser_snippet();

	//save deadline time and control info to EEPROM
	updateEEPROM();

	//this control_status variable is updated in json parsing function.
	//take action according to its value
	if(control_status == 1)
	{
		cyhal_gpio_write(RELAY_PIN, CYBSP_LED_STATE_OFF);
		printf("LED OFF\n");
	}
	else
	{
		cyhal_gpio_write(RELAY_PIN, CYBSP_LED_STATE_ON);
		printf("LED ON\n");
	}

	booting = FALSE;

	//This task's job is to check whether deadline is hit or not and take action accordingly
	xTaskCreate(rtc_task, "rtc_task", RTC_TASK_STACK_SIZE, NULL, RTC_TASK_PRIORITY, &rtctask_handle);

	for(;;)
	{
		cyhal_system_delay_ms(10000);

		memset(save_memory,0x00,LOGICAL_EEPROM_SIZE);	//reset JASON parser array before getting data
		memset(save_response,0x00,LOGICAL_EEPROM_SIZE);
		//create client
		rslt = http_client_create(&handle, &server_info);
		if(rslt != CY_RSLT_SUCCESS)
		{
			printf("error http_client_create\r\n");
		}

		//get time by creating HTTP request
		read_ID(devID,8);	//read device ID from EEPROM
		sprintf(req_body,"action=GET_ALL&DEVICE_ID=%s",devID);
		rslt = get_http_response(handle,req_body,strlen(req_body),TRUE);
		if(rslt != CY_RSLT_SUCCESS)
		{
			printf("error get_http_response\r\n");
		}

		//delete http client
		rslt = delete_http_client(handle);
		if(rslt != CY_RSLT_SUCCESS)
		{
			printf("error delete_http_client\r\n");
		}

		update_inprog = TRUE;

		memcpy(save_memory,save_response,250);		//update save_memory array with response

		json_parser_snippet();

		if(updateEEPROM_flag == TRUE)
		{
			printf("updateEEPROM\n");
			updateEEPROM_flag = FALSE;
			updateEEPROM();
			alarm_hit = FALSE;
		}

		if(updateRTC_flag == TRUE)
		{
			printf("updateRTC\n");
			updateRTC_flag = FALSE;
			updateRTC();
		}

		update_inprog = FALSE;

		if(control_flag == TRUE)
		{
			printf("update control\n");
			control_flag = FALSE;
			if(control_status == 1)
			{
				alarm_hit = FALSE;		//make this flag to false so that next deadline can be hit correctly
				cyhal_gpio_write(RELAY_PIN, CYBSP_LED_STATE_OFF);
			}
			else
			{
				cyhal_gpio_write(RELAY_PIN, CYBSP_LED_STATE_ON);
			}
		}
	}
}

void rtc_task(void *arg)
{
	struct tm date_time;
	cy_rslt_t rslt;
	cy_awsport_server_info_t server_info;
	cy_http_client_t handle;
	char devID[10] = {0};
	char req_body[256];
	char stat[2][4] = {"ON","OFF"};

	for(;;)
	{
		//read first RTC
		if(update_inprog == FALSE)
		{
			memset(&date_time, 0x00,sizeof(date_time));
			init_RTC(&date_time);

			if(alarm_hit == FALSE)
			{
				if(((date_time.tm_year + TM_YEAR_BASE) == year) && ((date_time.tm_mon + 1) == month) && (date_time.tm_mday== day )&& (date_time.tm_hour == hour )&& (date_time.tm_min == minute))
				{
					printf("\r\n hello\n");
					cyhal_gpio_write(RELAY_PIN, CYBSP_LED_STATE_ON);

					//update the database
					memset(req_body,0x00,256);
					//create client
					rslt = http_client_create(&handle, &server_info);
					if(rslt != CY_RSLT_SUCCESS)
					{
						printf("error http_client_create\r\n");
					}

					//get time by creating HTTP request
					read_ID(devID,8);	//read device ID from EEPROM
					sprintf(req_body,"action=UPDATE_CTRL&DEVICE_ID=%s&CONTROL=%s",devID,stat[1]);
					rslt = get_http_response(handle,req_body,strlen(req_body),FALSE);
					if(rslt != CY_RSLT_SUCCESS)
					{
						printf("error get_http_response\r\n");
					}

					//delete http client
					rslt = delete_http_client(handle);
					if(rslt != CY_RSLT_SUCCESS)
					{
						printf("error delete_http_client\r\n");
					}
					control_status = 0;		//0 means off
					alarm_hit = true;

				}
			}
		}
		cyhal_system_delay_ms(1000);
	}

}

void read_ID(char* id, uint8_t len)
{
	cy_en_em_eeprom_status_t eepromReturnValue;
	/* Initialize the flash start address in EEPROM configuration structure. */
#if ((defined(TARGET_CY8CKIT_064B0S2_4343W)||(defined(TARGET_CY8CPROTO_064B0S3))) && (USER_FLASH == FLASH_REGION_TO_USE ))
	Em_EEPROM_config.userFlashStartAddr = (uint32_t) APP_DEFINED_EM_EEPROM_LOCATION_IN_FLASH;
#else
	Em_EEPROM_config.userFlashStartAddr = (uint32_t) EepromStorage;
#endif

	eepromReturnValue = Cy_Em_EEPROM_Init(&Em_EEPROM_config, &Em_EEPROM_context);
	HandleError(eepromReturnValue, "Emulated EEPROM Initialization Error \r\n");

	/* Read 15 bytes out of EEPROM memory. */
	eepromReturnValue = Cy_Em_EEPROM_Read(LOGICAL_EEPROM_START, id, len, &Em_EEPROM_context);
	HandleError(eepromReturnValue, "Emulated EEPROM Read failed \r\n");
}

#if 1	//write to EEPROM
void write_ID(char* id, uint8_t len)
{
	cy_en_em_eeprom_status_t eepromReturnValue;
		/* Initialize the flash start address in EEPROM configuration structure. */
	#if ((defined(TARGET_CY8CKIT_064B0S2_4343W)||(defined(TARGET_CY8CPROTO_064B0S3))) && (USER_FLASH == FLASH_REGION_TO_USE ))
		Em_EEPROM_config.userFlashStartAddr = (uint32_t) APP_DEFINED_EM_EEPROM_LOCATION_IN_FLASH;
	#else
		Em_EEPROM_config.userFlashStartAddr = (uint32_t) EepromStorage;
	#endif

		eepromReturnValue = Cy_Em_EEPROM_Init(&Em_EEPROM_config, &Em_EEPROM_context);
		HandleError(eepromReturnValue, "Emulated EEPROM Initialization Error \r\n");

		/* Write initial data to EEPROM. */
		eepromReturnValue = Cy_Em_EEPROM_Write(LOGICAL_EEPROM_START, id, len,&Em_EEPROM_context);

		HandleError(eepromReturnValue, "Emulated EEPROM Write failed \r\n");
}
#endif
