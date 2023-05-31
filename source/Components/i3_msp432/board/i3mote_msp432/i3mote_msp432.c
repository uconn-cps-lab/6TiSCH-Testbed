/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== MSP_EXP432P401R.c ========
 *  This file is responsible for setting up the board specific items for the
 *  MSP_EXP432P401R board.
 */

#include <stdbool.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerMSP432.h>

#include <msp.h>
#include <rom.h>
#include <rom_map.h>
#include <dma.h>
#include <gpio.h>
#include <i2c.h>
#include <pmap.h>
#include <spi.h>
#include <timer_a.h>
#include <uart.h>
#include <wdt_a.h>
#include <adc14.h>
#include <ref_a.h>
#include <interrupt.h>

#include "i3mote_msp432.h"

/*
 *  =============================== DMA ===============================
 */

#include <ti/drivers/dma/UDMAMSP432.h>


#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(dmaControlTable, 256)
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=256
#elif defined(__GNUC__)
__attribute__ ((aligned (256)))
#endif
static DMA_ControlTable dmaControlTable[8];

/*
 *  ======== dmaErrorHwi ========
 *  This is the handler for the uDMA error interrupt.
 */
static void dmaErrorHwi(uintptr_t arg)
{
    int status = MAP_DMA_getErrorStatus();
    MAP_DMA_clearErrorStatus();

    /* Suppress unused variable warning */
    (void)status;

    while (1);
}

UDMAMSP432_Object udmaMSP432Object;

const UDMAMSP432_HWAttrs udmaMSP432HWAttrs = {
    .controlBaseAddr = (void *)dmaControlTable,
    .dmaErrorFxn = (UDMAMSP432_ErrorFxn)dmaErrorHwi,
    .intNum = INT_DMA_ERR,
    .intPriority = (~0)
};

const UDMAMSP432_Config UDMAMSP432_config = {
    .object = &udmaMSP432Object,
    .hwAttrs = &udmaMSP432HWAttrs
};

/*
 *  ======== MSP_EXP432P401R_initGeneral ========
 */
void MSP_EXP432P401R_initGeneral(void)
{
    Power_init();

    Power_setConstraint(PowerMSP432_DISALLOW_DEEPSLEEP_1);

    MAP_WDT_A_holdTimer();
    MAP_WDT_A_initIntervalTimer(WDT_A_CLOCKSOURCE_XCLK, WDT_A_CLOCKITERATIONS_8192);
    MAP_WDT_A_clearTimer();
    MAP_WDT_A_startTimer();

    MAP_PSS_disableHighSide();

}

/*
 *  =============================== GPIO ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(GPIOMSP432_config, ".const:GPIOMSP432_config")
#endif

#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOMSP432.h>

/*
 * Array of Pin configurations
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in MSP_EXP432P401R.h
 * NOTE: Pins not used for interrupts should be placed at the end of the
 *       array.  Callback entries can be omitted from callbacks array to
 *       reduce memory usage.
 */
GPIO_PinConfig gpioPinConfigs[] = {
    /* MSP_EXP432P401R_BUTTON1 */
    GPIOMSP432_P6_1 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    /* MSP_EXP432P401R_WAKEUP */
    GPIOMSP432_P4_0 | GPIO_CFG_IN_NOPULL | GPIO_CFG_IN_INT_FALLING,
    /* MSP_EXP432P401R_LED1 */
    GPIOMSP432_P6_2 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    /* MSP_EXP432P401R_LED_2 */
    GPIOMSP432_P6_3 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    /* MSP_EXP432P401R_SPI_CS */
    GPIOMSP432_P2_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
    /* Debug GPIO */
    GPIOMSP432_P6_4 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
};

/*
 * Array of callback function pointers
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in MSP_EXP432P401R.h
 * NOTE: Pins not used for interrupts can be omitted from callbacks array to
 *       reduce memory usage (if placed at end of gpioPinConfigs array).
 */
GPIO_CallbackFxn gpioCallbackFunctions[] = {
    /* MSP_EXP432P401R_BUTTON1 */
    NULL,
    /* MSP_EXP432P401R_WAKEUP */
    NULL,
    NULL,
    NULL,
    NULL
};

const GPIOMSP432_Config GPIOMSP432_config = {
    .pinConfigs = (GPIO_PinConfig *)gpioPinConfigs,
    .callbacks = (GPIO_CallbackFxn *)gpioCallbackFunctions,
    .numberOfPinConfigs = sizeof(gpioPinConfigs)/sizeof(GPIO_PinConfig),
    .numberOfCallbacks = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
    .intPriority = (~0)
};

/*
 *  ======== MSP_EXP432P401R_initGPIO ========
 */
void MSP_EXP432P401R_initGPIO(void)
{
    /* Initialize peripheral and pins */
    GPIO_init();

    MAP_PSS_disableHighSide();
}

/*
 *  =============================== I2C ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(I2C_config, ".const:I2C_config")
#pragma DATA_SECTION(i2cMSP432HWAttrs, ".const:i2cMSP432HWAttrs")
#endif

#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CMSP432.h>

I2CMSP432_Object i2cMSP432Objects[MSP_EXP432P401R_I2CCOUNT];

const I2CMSP432_HWAttrs i2cMSP432HWAttrs[MSP_EXP432P401R_I2CCOUNT] = {
    {
        .baseAddr = EUSCI_B2_BASE,
        .intNum = INT_EUSCIB2,
        .intPriority = (~0),
        .clockSource = EUSCI_B_I2C_CLOCKSOURCE_SMCLK
    }
};

const I2C_Config I2C_config[] = {
    {
        .fxnTablePtr = &I2CMSP432_fxnTable,
        .object = &i2cMSP432Objects[0],
        .hwAttrs = &i2cMSP432HWAttrs[0]
    },
    {NULL, NULL, NULL}
};

/*
 *  ======== MSP_EXP432P401R_initI2C ========
 */
void MSP_EXP432P401R_initI2C(void)
{
    /*
     * NOTE: TI-RTOS examples configure EUSCIB0 as either SPI or I2C.  Thus,
     * a conflict occurs when the I2C & SPI drivers are used simultaneously in
     * an application.  Modify the pin mux settings in this file and resolve the
     * conflict before running your the application.
     */
    /* Configure Pins 1.6 & 1.7 as SDA & SCL, respectively. */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3,
        GPIO_PIN6 | GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Initialize the I2C driver */
    I2C_init();
}

/*
 *  =============================== Power ===============================
 */
const PowerMSP432_ConfigV1 PowerMSP432_config = {
    .policyInitFxn = &PowerMSP432_initPolicy,
    //.policyFxn = &PowerMSP432_sleepPolicy,
    .policyFxn = &PowerMSP432_deepSleepPolicy,
    .initialPerfLevel = 2,
    .enablePolicy = true,
    .enablePerf = true,
    .enableParking = true
};

/*
 *  =============================== SPI ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(SPI_config, ".const:SPI_config")
#pragma DATA_SECTION(spiMSP432DMAHWAttrs, ".const:spiMSP432DMAHWAttrs")
#endif

#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPIMSP432DMA.h>

SPIMSP432DMA_Object spiMSP432DMAObjects[MSP_EXP432P401R_SPICOUNT];

const SPIMSP432DMA_HWAttrs spiMSP432DMAHWAttrs[MSP_EXP432P401R_SPICOUNT] = {
    {
        .baseAddr = EUSCI_A1_BASE,
        .bitOrder = EUSCI_A_SPI_MSB_FIRST,
        .clockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK,

        .defaultTxBufValue = 0,

        .dmaIntNum = INT_DMA_INT1,
        .intPriority = (~0),
        .rxDMAChannelIndex = DMA_CH3_EUSCIA1RX,
        .txDMAChannelIndex = DMA_CH2_EUSCIA1TX
    }
};

const SPI_Config SPI_config[] = {
    {
        .fxnTablePtr = &SPIMSP432DMA_fxnTable,
        .object = &spiMSP432DMAObjects[0],
        .hwAttrs = &spiMSP432DMAHWAttrs[0]
    },
    {NULL, NULL, NULL},
};

/*
 *  ======== MSP_EXP432P401R_initSPI ========
 */
void MSP_EXP432P401R_initSPI(void)
{
    MSP_EXP432P401R_prepareSPI();
    /* Configure CS, CLK, MOSI for SPI0 (EUSCI_B0) */
    SPI_init();
}

/*
 *  ======== MSP_EXP432P401R_prepareSPI ========
 */
void MSP_EXP432P401R_prepareSPI(void){

    /* Configure CS, CLK, MOSI for SPI0 (EUSCI_A1) */
      MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,
          GPIO_PIN1 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configure MISO for SPI0 (EUSCI_A1) */
      MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P2,
          GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);
}


/*
 *  ======== MSP_EXP432P401R_disableSPI ========
 */

void MSP_EXP432P401R_disableSPI(void){
    /*Disable SPI Pins for energy efficiency*/
      ROM_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P2, GPIO_PIN1 | GPIO_PIN2 |  GPIO_PIN3);
}


/*
 *  =============================== UART ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(UART_config, ".const:UART_config")
#pragma DATA_SECTION(uartMSP432HWAttrs, ".const:uartMSP432HWAttrs")
#endif

#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTMSP432.h>

UARTMSP432_Object uartMSP432Objects[MSP_EXP432P401R_UARTCOUNT];
unsigned char uartMSP432RingBuffer[MSP_EXP432P401R_UARTCOUNT][32];

/*
 * The baudrate dividers were determined by using the MSP430 baudrate
 * calculator
 * http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
 */
const UARTMSP432_BaudrateConfig uartMSP432Baudrates[] = {
    /* {baudrate, input clock, prescalar, UCBRFx, UCBRSx, oversampling} */
    {
        .outputBaudrate = 115200,
        .inputClockFreq = 12000000,
        .prescalar = 6,
        .hwRegUCBRFx = 8,
        .hwRegUCBRSx = 32,
        .oversampling = 1
    },
    {115200, 6000000,   3,  4,   2, 1},
    {115200, 3000000,   1, 10,   0, 1},
    {9600,   12000000, 78,  2,   0, 1},
    {9600,   6000000,  39,  1,   0, 1},
    {9600,   3000000,  19,  8,  85, 1},
    {9600,   32768,     3,  0, 146, 0}
};

const UARTMSP432_HWAttrs uartMSP432HWAttrs[MSP_EXP432P401R_UARTCOUNT] = {
    {
        .baseAddr = EUSCI_A0_BASE,
        .intNum = INT_EUSCIA0,
        .intPriority = (~0),
        .clockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK,
        .bitOrder = EUSCI_A_UART_LSB_FIRST,
        .numBaudrateEntries = sizeof(uartMSP432Baudrates) /
            sizeof(UARTMSP432_BaudrateConfig),
        .baudrateLUT = uartMSP432Baudrates,
        .ringBufPtr  = uartMSP432RingBuffer[0],
        .ringBufSize = sizeof(uartMSP432RingBuffer[0])
    },
    {
        .baseAddr = EUSCI_A2_BASE,
        .intNum = INT_EUSCIA2,
        .intPriority = (~0),
        .clockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK,
        .bitOrder = EUSCI_A_UART_LSB_FIRST,
        .numBaudrateEntries = sizeof(uartMSP432Baudrates) /
            sizeof(UARTMSP432_BaudrateConfig),
        .baudrateLUT = uartMSP432Baudrates,
        .ringBufPtr  = uartMSP432RingBuffer[1],
        .ringBufSize = sizeof(uartMSP432RingBuffer[1])
    }
};

const UART_Config UART_config[] = {
    {
        .fxnTablePtr = &UARTMSP432_fxnTable,
        .object = &uartMSP432Objects[0],
        .hwAttrs = &uartMSP432HWAttrs[0]
    },
    {
        .fxnTablePtr = &UARTMSP432_fxnTable,
        .object = &uartMSP432Objects[1],
        .hwAttrs = &uartMSP432HWAttrs[1]
    },
    {NULL, NULL, NULL}
};

/*
 *  ======== MSP_EXP432P401R_initUART ========
 */
void MSP_EXP432P401R_initUART(void)
{
    /* Set P1.2 & P1.3 in UART mode */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
        GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Set P3.2 & P3.3 in UART mode */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3,
        GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Initialize the UART driver */
    UART_init();
}

/*
 *  =============================== Watchdog ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(Watchdog_config, ".const:Watchdog_config")
#pragma DATA_SECTION(watchdogMSP432HWAttrs, ".const:watchdogMSP432HWAttrs")
#endif

#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogMSP432.h>

WatchdogMSP432_Object watchdogMSP432Objects[MSP_EXP432P401R_WATCHDOGCOUNT];

const WatchdogMSP432_HWAttrs
    watchdogMSP432HWAttrs[MSP_EXP432P401R_WATCHDOGCOUNT] = {
    {
        .baseAddr = WDT_A_BASE,
        .intNum = INT_WDT_A,
        .intPriority = (~0),
        .clockSource = WDT_A_CLOCKSOURCE_SMCLK,
        .clockDivider = WDT_A_CLOCKDIVIDER_8192K
    },
};

const Watchdog_Config Watchdog_config[] = {
    {
        .fxnTablePtr = &WatchdogMSP432_fxnTable,
        .object = &watchdogMSP432Objects[0],
        .hwAttrs = &watchdogMSP432HWAttrs[0]
    },
    {NULL, NULL, NULL}
};

/*
 *  ======== MSP_EXP432P401R_initWatchdog ========
 */
void MSP_EXP432P401R_initWatchdog(void)
{
    /* Initialize the Watchdog driver */
    Watchdog_init();
}

