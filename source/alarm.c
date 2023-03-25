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

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/
void HandleError(uint32_t status, char *message);


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
#define REQUEST_BODY                    "{\"status\": \"ON\",\"device\": \"400214\"}"
#define REQUEST_BODY_LENGTH        ( sizeof( REQUEST_BODY ) - 1 )


/////////////////////////////////////   HTTP Client End /////////////////////////////////////////////////

char save_response[250];
char save_memory[250];
int year = 0;
int month = 0;
int day = 0;
int hour = 0;
int minute = 0;
int sec = 0;
int current_mday=0, current_month=0, current_year=0, current_sec=0, current_min=0, current_hour=0;

bool date_flag = false;

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
	switch(json_object->value_type)
	{
		case JSON_STRING_TYPE:
		{
			printf("Found a string: %.*s\n", json_object->value_length,json_object->value );
			printf("Found a key: %.*s\n", json_object->object_string_length,json_object->object_string );
			printf("Found a key: %.*s\n", json_object->parent_object->object_string_length,json_object->parent_object->object_string );

			//extract "year"
			if((strncmp(json_object->object_string, "year", strlen("year")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;

				int sum, digit;
				sum = 0;
				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
				printf("the value is %d",sum);
				year = sum;
			}

			//extract "month"
			if((strncmp(json_object->object_string, "month", strlen("month")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;
				int sum, digit;
				sum = 0;

				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
           		printf("the value is %d",sum);
				month = sum;
			}

			//extract "day"
			if((strncmp(json_object->object_string, "day", strlen("day")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;

				int sum, digit;
				sum = 0;
				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
	        	printf("the value is %d",sum);
				day = sum;
			}

			//extract hour
			if((strncmp(json_object->object_string, "hour", strlen("hour")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;

				int sum, digit;
				sum = 0;
				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
	     		printf("the value is %d",sum);
				hour = sum;
			}

			//extract "minute"
			if((strncmp(json_object->object_string, "minute", strlen("minute")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;

				int sum, digit;
				sum = 0;
				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
	        	printf("the value is %d",sum);
				minute = sum;
			}

			//get current year
			if((strncmp(json_object->object_string, "current_year", strlen("current_year")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;

				int sum, digit;
				sum = 0;
				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
           		printf("the value is %d",sum);
				current_year = sum;
			}

			//get current monthjj
			if((strncmp(json_object->object_string, "current_month", strlen("current_month")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;

				int sum, digit;
				sum = 0;
				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
				printf("the value is %d",sum);
				current_month = sum;
			}

			//get current_day
			if((strncmp(json_object->object_string, "current_day", strlen("current_day")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;

				int sum, digit;
				sum = 0;
				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
           		printf("the value is %d",sum);
				current_mday = sum;
			}

			//get current hour
			if((strncmp(json_object->object_string, "current_hour", strlen("current_hour")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;

				int sum, digit;
				sum = 0;
				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
				printf("the value is %d",sum);
				current_hour = sum;
			}

			//get current minute
			if((strncmp(json_object->object_string, "current_minute", strlen("current_minute")) == 0))
			{
				int16_t  c = (int16_t )json_object->value_length;
				int length = (int)c;

				int sum, digit;
				sum = 0;
				for(int i = 0 ; i < length ; i++)
				{
					digit = json_object->value[i] - 0x30;
					sum = (sum * 10) + digit;
				}
				printf("the value is %d",sum);
				current_min = sum;
			}

			break;
		}
		case JSON_NUMBER_TYPE:
		{
			printf("Found a number: %ld\n", (unsigned long)json_object->intval);
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
			printf("Found an ARRAY");
			break;
		}
		default:
		{
			break;
		}
	}
	return 0;
}

int json_parser_snippet(void)
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

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    /*BSP init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        
    }

    /* Initialize the User LED */
    result = cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_ON);
    /* GPIO init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {

    }
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

static void get_Alarm_time(cy_http_client_t handle)
{
	cy_http_client_request_header_t request;
	cy_http_client_header_t header[1];
	uint32_t num_header;
	cy_http_client_response_t response;
	cy_rslt_t res = CY_RSLT_SUCCESS;	
    
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
	num_header = 1;
	/* Generate the standard header and user-defined header, and update in the request structure. */
	res = cy_http_client_write_header(handle, &request, &header[0], num_header);
	if( res != CY_RSLT_SUCCESS )
	{
		printf("HTTP Client Header Write Failed!\n");

	}
	/* Send the HTTP request and body to the server, and receive the response from it. */
	res = cy_http_client_send(handle, &request, (uint8_t *)REQUEST_BODY, REQUEST_BODY_LENGTH, &response);
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


	for(int i = 0; i < response.body_len; i++)
	{
		save_response[i] = response.body[i];
	}

	( void ) memset( &header[0], 0, sizeof( header[0] ) );
	header[0].field = "Connection";
	header[0].field_len = strlen("Connection");
	num_header = 1;
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
}

void alarm_task(void *arg){

	cy_rslt_t rslt;
	char buffer[STRING_BUFFER_SIZE];
	struct tm date_time;

	cy_awsport_server_info_t server_info;
	cy_http_client_t handle;	

	uint8_t eepromReadArray[LOGICAL_EEPROM_SIZE];
	uint8_t eepromWriteArray[LOGICAL_EEPROM_SIZE];	

    init_relay();

	//create client 
	res = http_client_create(&handle, &server_info);
	if(res != CY_RSLT_SUCCESS)
	{
		continue;
	}

	//get time by creating HTTP request
	res = get_Alarm_time(handle);
	if(res != CY_RSLT_SUCCESS)
	{
		continue;
	}		
	   
	//delete http client
	res = delete_http_client(handle);
	if(res != CY_RSLT_SUCCESS)
	{
		continue;
	}	

	for(;;)
	{
		/* Initialize RTC */
	    rslt = cyhal_rtc_init(&rtc_obj);
	    if (CY_RSLT_SUCCESS != rslt)
	    {
	        handle_error();
	    }

		/* Display available commands */
	    printf("Available commands \r\n");
	    printf("1 : Set new time and date\r\n");

		rslt = cyhal_rtc_read(&rtc_obj, &date_time);
		if (CY_RSLT_SUCCESS == rslt)
		{
			handle_error();
		}

		strftime(buffer, sizeof(buffer), "%c", &date_time);
		printf("\r%s", buffer);
		memset(buffer, '\0', sizeof(buffer));	

	    for(int i = 0; i < strlen(save_response); i++){
	    	eepromWriteArray[i] = (uint8_t)save_response[i];
	    }

		printf("\r\n");

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

		/* If first byte of EEPROM is not 'P', then write the data for initializing
		* the EEPROM content.
		*/
		if(flag == false)
		{
			flag = true;
			/* Write initial data to EEPROM. */
			eepromReturnValue = Cy_Em_EEPROM_Write(LOGICAL_EEPROM_START,
													eepromWriteArray,
													LOGICAL_EEPROM_SIZE,
													&Em_EEPROM_context);
			HandleError(eepromReturnValue, "Emulated EEPROM Write failed \r\n");

		}
		
		/* Read contents of EEPROM after write. */
		eepromReturnValue = Cy_Em_EEPROM_Read(LOGICAL_EEPROM_START,
												eepromReadArray, LOGICAL_EEPROM_SIZE,
												&Em_EEPROM_context);
		HandleError(eepromReturnValue, "Emulated EEPROM Read failed \r\n" );

		for(count = 0; count < LOGICAL_EEPROM_SIZE ; count++){
			printf("%c",eepromReadArray[count]);
		}
		printf("\r\n");
		printf("Displaying demo \r\n");									

		for(count = 0; count < LOGICAL_EEPROM_SIZE ; count++){
			char c =(char)(eepromReadArray[count]);
			save_memory[count] = c;
		}
		printf("To here \r\n");
	    printf("\r\n");

		for(count = 0; count < LOGICAL_EEPROM_SIZE ; count++){
				printf("%c", save_memory[count]);
		}
		printf("To here there \r\n");

		int i = json_parser_snippet();

		if(date_flag == false)
		{
			struct tm new_time = {0};
			if (validate_date_time(current_sec, current_min, current_hour, current_mday, current_month, current_year))
			{
				new_time.tm_sec = current_sec;
				new_time.tm_min = current_min;
				new_time.tm_hour = current_hour;
				new_time.tm_mday = current_mday;
				new_time.tm_mon = month - 1;
				new_time.tm_year = year - TM_YEAR_BASE;
				//new_time.tm_wday = get_day_of_week(mday, month, year);

				rslt = cyhal_rtc_write(&rtc_obj, &new_time);
				if (CY_RSLT_SUCCESS == rslt)
				{
					printf("\rRTC time updated\r\n\n");
				}
				else
				{
					handle_error();
				}
			}
			else
			{
				printf("\rInvalid values! Please enter the values in specified"
						" format\r\n");
			}
			date_flag = true;
		}		

        printf("%d",i);
		printf("\r\n");
		printf("%d",year);
		printf("\r\n");

		printf("%d",month);
		printf("\r\n");

		printf("%d",day);
		printf("\r\n");

		printf("%d",hour);
		printf("\r\n");

		printf("%d",minute);
		printf("\r\n");

		printf("%d",(date_time.tm_year+TM_YEAR_BASE));
		printf("\r\n");
		
		printf("%d",(date_time.tm_mon+1));
		printf("\r\n");

		printf("%d",date_time.tm_mday);
		printf("\r\n");

		printf("%d",date_time.tm_hour);
		printf("\r\n");

		printf("%d",date_time.tm_min);
		printf("\r\n");


		if(((date_time.tm_mon+1) == month) && (date_time.tm_mday== day )&& (date_time.tm_hour == hour )&& (date_time.tm_min == minute))
		{
			printf("\r\n hello......................");
			cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);

			cyhal_system_delay_ms(60000);
		}		

		cyhal_system_delay_ms(5000);
	}
}