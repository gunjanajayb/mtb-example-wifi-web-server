/******************************************************************************
* File Name:   main.c
*
* Description: This application demonstrates a PSoC 6 device hosting an http
*              webserver. The PSoC 6 measures the voltage of the ambient light
*              sensor on the CY8CKIT-028. It then displays that information on
*              a webpage. The PSoC 6 also controls the brightness of the RED
*              led on the board. The brightness can be controlled by the two
*              capsense buttons, capsense slider, or the webpage.
*
* Related Document: README.md
*
********************************************************************************
* Copyright 2021-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
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
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/* Header file includes */
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* FreeRTOS headers */
#include <FreeRTOS.h>
#include <task.h>

/* Server task header file */
#include "web_server.h"

#if defined(CY_DEVICE_PSOC6A512K)
#include "cy_serial_flash_qspi.h"
#include "cycfg_qspi_memslot.h"
#endif

#include "flowmeter.h"
#include "eeprom.h"
/*******************************************************************************
* Macros
******************************************************************************/
/* RTOS related macros. */
#define SERVER_TASK_STACK_SIZE        (10 * 1024)
#define SERVER_TASK_PRIORITY          (1)

/* RTOS related macros. */
#define FLOWSENSE_TASK_STACK_SIZE        (4 * 1024)
#define FLOWSENSE_TASK_PRIORITY          (1)

#define FLOW_QUEUE_SIZE				10
/*******************************************************************************
* Global Variables
********************************************************************************/
/* This enables RTOS aware debugging */
volatile int uxTopUsedPriority;

/* SOFTAP server task handle. */
TaskHandle_t server_task_handle;
TaskHandle_t flowsens_task_handle;
cyhal_gpio_callback_data_t gpio_btn_callback_data;
QueueHandle_t flow_eepromQ;
QueueHandle_t flow_cloudQ;
/*******************************************************************************
 * Function Name: main
 *******************************************************************************
 * Summary:
 *  Entry function for the application.
 *  This function initializes the BSP, UART port for debugging, initializes the
 *  user LED on the kit, and starts the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int: Should never return.
 *
 *******************************************************************************/
int main(void)
{
    /* Variable to capture return value of functions */
    cy_rslt_t result;

    /* This enables RTOS aware debugging in OpenOCD */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    /* Initialize the Board Support Package (BSP) */
    result = cybsp_init();
    CHECK_RESULT(result);

	/* Initialize the user button */
	result = cyhal_gpio_init(P13_5, CYHAL_GPIO_DIR_INPUT,
			CYHAL_GPIO_DRIVE_OPENDRAINDRIVESHIGH, CYBSP_BTN_PRESSED);
	/* User button init failed. Stop program execution */
	CHECK_RESULT(result);

	/* Configure GPIO interrupt */
	gpio_btn_callback_data.callback = gpio_interrupt_handler;
	cyhal_gpio_register_callback(P13_5,
								 &gpio_btn_callback_data);
	cyhal_gpio_enable_event(P13_5, CYHAL_GPIO_IRQ_RISE,
								 GPIO_INTERRUPT_PRIORITY, true);

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

	initEEPROM();

    /* Enable global interrupts */
    __enable_irq();

    #if defined(CY_DEVICE_PSOC6A512K)
        const uint32_t bus_frequency = 50000000lu;
        cy_serial_flash_qspi_init(smifMemConfigs[0], CYBSP_QSPI_D0, CYBSP_QSPI_D1,
                                      CYBSP_QSPI_D2, CYBSP_QSPI_D3, NC, NC, NC, NC,
                                      CYBSP_QSPI_SCK, CYBSP_QSPI_SS, bus_frequency);

        cy_serial_flash_qspi_enable_xip(true);
    #endif

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen */
    APP_INFO(("\x1b[2J\x1b[;H"));
    printf("Visit the below link for step by step instructions to run this code example:\n\n");
    printf("https://github.com/Infineon/mtb-example-anycloud-wifi-web-server#operation\n\n");
    APP_INFO(("============================================================\n"));
    APP_INFO(("               Wi-Fi Web Server                   \n"));
    APP_INFO(("============================================================\n\n"));

    /* Starts the SoftAP and then HTTP server . */
    xTaskCreate(server_task, "HTTP Web Server", SERVER_TASK_STACK_SIZE, NULL,
                SERVER_TASK_PRIORITY, &server_task_handle);

    flow_eepromQ = xQueueCreate(FLOW_QUEUE_SIZE,sizeof(float));
    flow_cloudQ = xQueueCreate(FLOW_QUEUE_SIZE,sizeof(float));
    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never get here */
    CY_ASSERT(0);
}

/* [] END OF FILE */
