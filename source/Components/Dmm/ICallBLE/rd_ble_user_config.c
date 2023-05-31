/******************************************************************************

 @file  rd_ble_user_config.c

 @brief This file contains user configurable variables for the BLE
        Application.

 Group: WCS, BTS
 Target Device: CC13xx

 ******************************************************************************
 
 Copyright (c) 2016-2018, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 Release Name: simplelink_cc13x2_sdk_2_10_00_
 Release Date: 2018-04-09 15:16:26
 *****************************************************************************/
#if DMM_ENABLE
/*******************************************************************************
 * INCLUDES
 */

#include "hal_types.h"
#include "ble_user_config.h"
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>

#include <dmm/dmm_rfmap.h>
#include "dmm/dmm_policy.h"
#include "dmm/dmm_scheduler.h"

#include "ecc/ECCROMCC26XX.h"

#include <ti/drivers/AESCCM.h>
#include <ti/drivers/aesccm/AESCCMCC26XX.h>
#include <ti/drivers/AESECB.h>
#include <ti/drivers/aesecb/AESECBCC26XX.h>
#include <ti/drivers/cryptoutils/cryptokey/CryptoKeyPlaintext.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/hal/Hwi.h>

#include "TRNGCC26XX.h"
#include <driverlib/pka.h>

#include "remote_display.h"
#include "led.h"

/*******************************************************************************
 * MACROS
 */

/*******************************************************************************
 * CONSTANTS
 */

// Tx Power
#define NUM_TX_POWER_VALUES (sizeof(TxPowerTable) / sizeof(txPwrVal_t))

// Default Tx Power Index
#if defined(CC13X2P)
#define DEFAULT_TX_POWER              (NUM_TX_POWER_VALUES - 1)
#else // !CC13X2
#define DEFAULT_TX_POWER               7
#endif // CC13X2

// Override NOP
#define OVERRIDE_NOP                   0xC0000001

/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

void driverTable_fnSpinlock(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 */

// RF Override Registers
// Note: Used with CMD_RADIO_SETUP; called at boot time and after wake.
// Note: Must be in RAM as these overrides may need to be modified at runtime
//       based on temperature compensation, although it is possible this may
//       be automated in CM0 in PG2.0.
#if defined(USE_FPGA)
regOverride_t rfRegTbl[] =
{
#if defined(CC26X2)
    // CC2642, as update by Helge on 12/12/17: Common Overrides for BLE5
    0x000151D0,
    0x00041110,
    0x00000083,
    0x00800403,
    0x80000303,
    0x02980243,
    0x01080263,
    0x08E90AA3,
    0x00068BA3,
    0x0E490C83,
    0x00005100, // Update matched filter for wired input
    0x721C5104, // Update matched filter for wired input
    0x00725108, // Update matched filter for wired input
    0x48f450d4, // Update matched filter gain for wired input
   0xFFFFFFFF
};
#endif // CC26X2

regOverride_t rfRegTbl1M[] =
{
    0x02405320,
    0x010302A3,
    0xFFFFFFFF,                    // End of override list
};

#if defined(BLE_V50_FEATURES) && (BLE_V50_FEATURES & (PHY_2MBPS_CFG | PHY_LR_CFG))
regOverride_t rfRegTbl2M[] =
{
    0x02405320,
    0x00B502A3,
    0xFFFFFFFF,                    // End of override list
};

regOverride_t rfRegTblCoded[] =
{
    0x01013487,
    0x02405320,
    0x069802A3,
    0xFFFFFFFF,                    // End of override list
};
#endif // PHY_2MBPS_CFG | PHY_LR_CFG

#elif defined(CC26XX_R2)

regOverride_t rfRegTbl[] =
{
    // Add 10 us to RF synth calibration time
    0x04280243,
    // Set advLenMask to 0xFF to avoid ROM patch
    0x00FF8A43,
#ifdef CACHE_AS_RAM
    // Turn off pointer check to allow radio commands in flash cache RAM
    0x00018063,
#endif //CACHE_AS_RAM
   0xFFFFFFFF
};

#if defined ( CC26X2 )
regOverride_t rfRegTbl1M[] =
{
      // override_ble5_setup_override_1mbps.xml
      // PHY: Use MCE RAM patch, RFE ROM bank 3 (mode 0)
   MCE_RFE_OVERRIDE(1, 0, 0, 0, 3, 0),
      // Bluetooth 5: Reduce pilot tone length
   HW_REG_OVERRIDE(0x5320, 0x0240),
      // Bluetooth 5: Compensate for reduced pilot tone length
      0x013302A3,
   0xFFFFFFFF
};

#if defined(BLE_V50_FEATURES) && (BLE_V50_FEATURES & (PHY_2MBPS_CFG | PHY_LR_CFG))
regOverride_t rfRegTbl2M[] =
{
      // override_ble5_setup_override_2mbps.xml
      // PHY: Use MCE RAM patch, RFE ROM bank 3 (mode 2)
   MCE_RFE_OVERRIDE(1, 0, 2, 0, 3, 2),
      // Bluetooth 5: Reduce pilot tone length
   HW_REG_OVERRIDE(0x5320, 0x0240),
      // Bluetooth 5: Compensate for reduced pilot tone length
      0x00D102A3,
   0xFFFFFFFF
};

regOverride_t rfRegTblCoded[] =
{
      // override_ble5_setup_override_coded.xml
      // PHY: Use MCE RAM patch, RFE ROM bank 3 (mode 1)
   MCE_RFE_OVERRIDE(1, 0, 1, 0, 3, 1),
      // Bluetooth 5: Reduce pilot tone length
   HW_REG_OVERRIDE(0x5320, 0x0240),
      // Bluetooth 5: Compensate for reduced pilot tone length
      0x078902A3,
   0xFFFFFFFF
};
#endif // PHY_2MBPS_CFG | PHY_LR_CFG
#else
regOverride_t rfRegTbl1M[] =
{
      0x05F86084,                    // RFC_RFE:SPARE0. Select R1-style gain table
   0xFFFFFFFF
};
#if defined(BLE_V50_FEATURES) && (BLE_V50_FEATURES & (PHY_2MBPS_CFG | PHY_LR_CFG))

regOverride_t rfRegTbl2M[] =
{
   0xFFFFFFFF
};

regOverride_t rfRegTblCoded[] =
{
   0xFFFFFFFF
};
#endif // PHY_2MBPS_CFG | PHY_LR_CFG
#endif // CC26X2 or CC26XX

#elif defined(CC26XX)

// CC26xx Normal Package with Flash Settings for 48 MHz device
#if defined(CC26X2)
regOverride_t rfRegTbl[] =
{
    // override_ble5_setup_override_common.xml
    // Synth: Use 48 MHz crystal, enable extra PLL filtering
    0x02400403,
    // Synth: Configure extra PLL filtering
    0x001C8473,
    // Synth: Configure synth hardware
    0x00088433,
    // Synth: Configure faster calibration
   HW32_ARRAY_OVERRIDE(0x4004, 1),
    // Synth: Configure faster calibration
    0x1C0C0618,
    // Synth: Configure faster calibration
    0xC00401A1,
    // Synth: Configure faster calibration
    0x00010101,
    // Synth: Configure faster calibration
    0xC0040141,
    // Synth: Configure faster calibration
    0x00214AD3,
    // Synth: Decrease synth programming time-out (0x0298 RAT ticks = 166 us)
    0x02980243,
    // DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). In Rx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4).
    0xFCFC08C3,
    // Bluetooth 5: Compensate for reduced pilot tone length
    0x01080263,
    // Bluetooth 5: Compensate for reduced pilot tone length
    0x08E90AA3,
    // Bluetooth 5: Compensate for reduced pilot tone length
    0x00068BA3,
    // Bluetooth 5: Set correct total clock accuracy for received AuxPtr assuming local sleep clock of 50 ppm
    0x0E490C83,
   0xFFFFFFFF
};

regOverride_t rfRegTbl1M[] =
{
      // override_ble5_setup_override_1mbps.xml
      // PHY: Use MCE RAM patch, RFE ROM bank 3 (mode 0)
   MCE_RFE_OVERRIDE(1, 0, 0, 0, 3, 0),
      // Bluetooth 5: Reduce pilot tone length
   HW_REG_OVERRIDE(0x5320, 0x0240),
      // Bluetooth 5: Compensate for reduced pilot tone length
      0x013302A3,
   0xFFFFFFFF
};

#if defined(BLE_V50_FEATURES) && (BLE_V50_FEATURES & (PHY_2MBPS_CFG | PHY_LR_CFG))
regOverride_t rfRegTbl2M[] =
{
      // override_ble5_setup_override_2mbps.xml
      // PHY: Use MCE RAM patch, RFE ROM bank 3 (mode 2)
   MCE_RFE_OVERRIDE(1, 0, 2, 0, 3, 2),
      // Bluetooth 5: Reduce pilot tone length
   HW_REG_OVERRIDE(0x5320, 0x0240),
      // Bluetooth 5: Compensate for reduced pilot tone length
      0x00D102A3,
   0xFFFFFFFF
};

regOverride_t rfRegTblCoded[] =
{
      // override_ble5_setup_override_coded.xml
      // PHY: Use MCE RAM patch, RFE ROM bank 3 (mode 1)
   MCE_RFE_OVERRIDE(1, 0, 1, 0, 3, 1),
      // Bluetooth 5: Reduce pilot tone length
   HW_REG_OVERRIDE(0x5320, 0x0240),
      // Bluetooth 5: Compensate for reduced pilot tone length
      0x078902A3,
   0xFFFFFFFF
};

#endif // PHY_2MBPS_CFG | PHY_LR_CFG

#else // unknown device package

#error "***BLE USER CONFIG BUILD ERROR*** Unknown package type!"

#endif // <board>

#elif defined(CC13XX)

#if defined(CC13X2)
regOverride_t rfRegTbl[] =
{
      // override_ble5_setup_override_common.xml
    // Synth: Use 48 MHz crystal, enable extra PLL filtering
    0x02400403,
    // Synth: Configure extra PLL filtering
    0x001C8473,
    // Synth: Configure synth hardware
    0x00088433,
    // Synth: Configure faster calibration
   HW32_ARRAY_OVERRIDE(0x4004, 1),
    // Synth: Configure faster calibration
    0x1C0C0618,
    // Synth: Configure faster calibration
    0xC00401A1,
    // Synth: Configure faster calibration
    0x00010101,
    // Synth: Configure faster calibration
    0xC0040141,
    // Synth: Configure faster calibration
    0x00214AD3,
    // Synth: Decrease synth programming time-out (0x0298 RAT ticks = 166 us)
    0x02980243,
    // DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). In Rx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4).
    0xFCFC08C3,
    // Bluetooth 5: Compensate for reduced pilot tone length
    0x01080263,
    // Bluetooth 5: Compensate for reduced pilot tone length
    0x08E90AA3,
    // Bluetooth 5: Compensate for reduced pilot tone length
    0x00068BA3,
    // Bluetooth 5: Set correct total clock accuracy for received AuxPtr assuming local sleep clock of 50 ppm
    0x0E490C83,
   0xFFFFFFFF
};

regOverride_t rfRegTbl1M[] =
{
      // override_ble5_setup_override_1mbps.xml
      // PHY: Use MCE RAM patch, RFE ROM bank 3 (mode 0)
   MCE_RFE_OVERRIDE(1, 0, 0, 0, 3, 0),
      // Bluetooth 5: Reduce pilot tone length
   HW_REG_OVERRIDE(0x5320, 0x0240),
      // Bluetooth 5: Compensate for reduced pilot tone length
      0x013302A3,
   0xFFFFFFFF
};

#if defined(BLE_V50_FEATURES) && (BLE_V50_FEATURES & (PHY_2MBPS_CFG | PHY_LR_CFG))
regOverride_t rfRegTbl2M[] =
{
      // override_ble5_setup_override_2mbps.xml
      // PHY: Use MCE RAM patch, RFE ROM bank 3 (mode 2)
   MCE_RFE_OVERRIDE(1, 0, 2, 0, 3, 2),
      // Bluetooth 5: Reduce pilot tone length
   HW_REG_OVERRIDE(0x5320, 0x0240),
      // Bluetooth 5: Compensate for reduced pilot tone length
      0x00D102A3,
   0xFFFFFFFF
};

regOverride_t rfRegTblCoded[] =
{
      // override_ble5_setup_override_coded.xml
      // PHY: Use MCE RAM patch, RFE ROM bank 3 (mode 1)
   MCE_RFE_OVERRIDE(1, 0, 1, 0, 3, 1),
      // Bluetooth 5: Reduce pilot tone length
   HW_REG_OVERRIDE(0x5320, 0x0240),
      // Bluetooth 5: Compensate for reduced pilot tone length
      0x078902A3,
   0xFFFFFFFF
};
#endif // PHY_2MBPS_CFG | PHY_LR_CFG

#elif defined(CC13X2P)
regOverride_t rfRegTbl[] =
{
    // List of hardware and configuration registers to override during common initialization of any Bluetooth 5 PHY format
    0x12400403,                    // Synth: Use 48 MHz crystal, enable extra PLL filtering when modulation starts
    0x001C8473,                    // Synth: Configure extra PLL filtering
    0x00088433,                    // Synth: Configure synth hardware
    0x00038793,                    // Synth: Set minimum RTRIM to 3
   HW32_ARRAY_OVERRIDE(0x4004, 1), // Synth: Configure faster calibration
    0x1C0C0618,                    // Synth: Configure faster calibration
    0xC00401A1,                    // Synth: Configure faster calibration
    0x00010101,                    // Synth: Configure faster calibration
    0xC0040141,                    // Synth: Configure faster calibration
    0x00214AD3,                    // Synth: Configure faster calibration
    0x02980243,                    // Synth: Decrease synth programming time-out (0x0298 RAT ticks = 166 us)
    0xFCFC08C3,                    // DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). In Rx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4).
    0x00038883,                    // Rx: Set LNA bias current offset to adjust +3 (default: 0)
    0x000288A3,                    // Rx: Set RSSI offset to adjust reported RSSI by -2 dB (default: 0)
    0x08E90AA3,                    // Bluetooth 5: Compensate for changed pilot tone length
    0x00068BA3,                    // Bluetooth 5: Compensate for changed pilot tone length
    0x0E490C83,                    // Bluetooth 5: Set correct total clock accuracy for received AuxPtr assuming local sleep clock of 50 ppm
    // Tx Power Gain: Value depends on HP PA 22-bit Tx Power value, otherwise
    //                termination value used for default Tx Power values.
    // WARNING: Do not change this value!
    0x00000000,
   0xFFFFFFFF
};

regOverride_t rfRegTbl1M[] =
{
    // List of hardware and configuration registers to override when selecting Bluetooth 5, LE 1M PHY
   MCE_RFE_OVERRIDE(1, 0, 0, 1, 0, 0),    // PHY: Use MCE RAM patch (mode 0), RFE RAM patch (mode 0)
   HW_REG_OVERRIDE(0x6024, 0x4C20),       // Bluetooth 5: Reconfigure pilot tone length for high output PA
   0xFFFFFFFF
};
      
#if defined(BLE_V50_FEATURES) && (BLE_V50_FEATURES & (PHY_2MBPS_CFG | PHY_LR_CFG))
regOverride_t rfRegTbl2M[] =
{
    // List of hardware and configuration registers to override when selecting Bluetooth 5, LE 2M PHY
   MCE_RFE_OVERRIDE(1, 0, 2, 1, 0, 2),    // PHY: Use MCE RAM patch (mode 2), RFE RAM patch (mode 2)
   HW_REG_OVERRIDE(0x6024, 0x4C22),       // Bluetooth 5: Reconfigure pilot tone length for high output PA
   0xFFFFFFFF
};

regOverride_t rfRegTblCoded[] =
{
    // List of hardware and configuration registers to override when selecting Bluetooth 5, LE Coded PHY
   MCE_RFE_OVERRIDE(1, 0, 1, 1, 0, 1),    // PHY: Use MCE RAM patch (mode 1), RFE RAM patch (mode 1)
   HW_REG_OVERRIDE(0x5320, 0x05A0),       // Bluetooth 5: Reconfigure pilot tone length for high output PA
   HW_REG_OVERRIDE(0x6024, 0x4C21),       // Bluetooth 5: Reconfigure pilot tone length for high output PA
    0x07D102A3,                            // Bluetooth 5: Compensate for changed pilot tone length
   0xFFFFFFFF
};
#endif // PHY_2MBPS_CFG | PHY_LR_CFG

#else // unknown board package

#error "***BLE USER CONFIG BUILD ERROR*** Unknown board type!"

#endif // <board>

#else // unknown platform

#error "ERROR: Unknown platform!"

#endif // <board>

//
// Tx Power Table Used Depends on Device Package
//

#if defined(USE_FPGA)

// Differential Output (same as CC2650EM_7ID for now)
// Tx Power Values (Pout, Tx Power)
const txPwrVal_t TxPowerTable[] =
{
   { TX_POWER_MINUS_21_DBM, 0x06C7 },
    { TX_POWER_MINUS_18_DBM, 0x06C9 },
    { TX_POWER_MINUS_15_DBM, 0x0C88 },
    { TX_POWER_MINUS_12_DBM, 0x108A },
    { TX_POWER_MINUS_9_DBM,  0x0A8D },
    { TX_POWER_MINUS_6_DBM,  0x204D },
    { TX_POWER_MINUS_3_DBM,  0x2851 },
    { TX_POWER_0_DBM,        0x3459 },
    { TX_POWER_1_DBM,        0x385C },
    { TX_POWER_2_DBM,        0x440D },
    { TX_POWER_3_DBM,        0x5411 },
    { TX_POWER_4_DBM,        0x6C16 },
   { TX_POWER_5_DBM,        0x941E }
};

#elif defined(CC26XX)

#if defined(CC26X2)

// Differential Output
// Tx Power Values (Pout, Tx Power)
const txPwrVal_t TxPowerTable[] =
{
   { TX_POWER_MINUS_21_DBM, 0x06C7 },
    { TX_POWER_MINUS_18_DBM, 0x06C9 },
    { TX_POWER_MINUS_15_DBM, 0x0C88 },
    { TX_POWER_MINUS_12_DBM, 0x108A },
    { TX_POWER_MINUS_9_DBM,  0x0A8D },
    { TX_POWER_MINUS_6_DBM,  0x204D },
    { TX_POWER_MINUS_3_DBM,  0x2851 },
    { TX_POWER_0_DBM,        0x3459 },
    { TX_POWER_1_DBM,        0x385C },
    { TX_POWER_2_DBM,        0x440D },
    { TX_POWER_3_DBM,        0x5411 },
    { TX_POWER_4_DBM,        0x6C16 },
   { TX_POWER_5_DBM,        0x941E }
};

#else // unknown board package

#error "***BLE USER CONFIG BUILD ERROR*** Unknown CC26x2 board type!"

#endif // <board>

#elif defined(CC13XX)

#if defined(CC13X2)
// Tx Power Values (Pout, Tx Power)
const txPwrVal_t TxPowerTable[] =
{
   { TX_POWER_MINUS_21_DBM, 0x06C7 },
      { TX_POWER_MINUS_18_DBM, 0x06C9 },
      { TX_POWER_MINUS_15_DBM, 0x0C88 },
      { TX_POWER_MINUS_12_DBM, 0x108A },
      { TX_POWER_MINUS_9_DBM,  0x0A8D },
      { TX_POWER_MINUS_6_DBM,  0x204D },
      { TX_POWER_MINUS_3_DBM,  0x2851 },
      { TX_POWER_0_DBM,        0x3459 },
      { TX_POWER_1_DBM,        0x385C },
      { TX_POWER_2_DBM,        0x440D },
      { TX_POWER_3_DBM,        0x5411 },
      { TX_POWER_4_DBM,        0x6C16 },
   { TX_POWER_5_DBM,        0x941E }
};
  
#elif defined(CC13X2P)
// Tx Power Values (Pout, Tx Power)
const txPwrVal_t TxPowerTable[] =
{
   {TX_POWER_MINUS_21_DBM, RF_TxPowerTable_DefaultPAEntry( 7, 3, 0,  3) },    // 0x000006C7
      {TX_POWER_MINUS_18_DBM, RF_TxPowerTable_DefaultPAEntry( 9, 3, 0,  3) },    // 0x000006C9
      {TX_POWER_MINUS_15_DBM, RF_TxPowerTable_DefaultPAEntry( 8, 2, 0,  6) },    // 0x00000C88
      {TX_POWER_MINUS_12_DBM, RF_TxPowerTable_DefaultPAEntry(10, 2, 0,  8) },    // 0x0000108A
      {TX_POWER_MINUS_10_DBM, RF_TxPowerTable_DefaultPAEntry(12, 2, 0, 11) },    // 0x0000168C
      {TX_POWER_MINUS_9_DBM,  RF_TxPowerTable_DefaultPAEntry(13, 2, 0,  5) },    // 0x00000A8D
      {TX_POWER_MINUS_6_DBM,  RF_TxPowerTable_DefaultPAEntry(13, 1, 0,  6) },    // 0x0000204D
      {TX_POWER_MINUS_5_DBM,  RF_TxPowerTable_DefaultPAEntry(14, 1, 0, 17) },    // 0x0000224E
      {TX_POWER_MINUS_3_DBM,  RF_TxPowerTable_DefaultPAEntry(17, 1, 0, 20) },    // 0x00002851
      {TX_POWER_0_DBM,        RF_TxPowerTable_DefaultPAEntry(25, 1, 0, 26) },    // 0x00003459
      {TX_POWER_1_DBM,        RF_TxPowerTable_DefaultPAEntry(28, 1, 0, 28) },    // 0x0000385C
      {TX_POWER_2_DBM,        RF_TxPowerTable_DefaultPAEntry(13, 0, 0, 34) },    // 0x0000440D
      {TX_POWER_3_DBM,        RF_TxPowerTable_DefaultPAEntry(17, 0, 0, 42) },    // 0x00005411
      {TX_POWER_4_DBM,        RF_TxPowerTable_DefaultPAEntry(22, 0, 0, 54) },    // 0x00006C16
      {TX_POWER_5_DBM,        RF_TxPowerTable_DefaultPAEntry(30, 0, 0, 74) },    // 0x0000941E
      {TX_POWER_6_DBM,        RF_TxPowerTable_HighPAEntry(46, 0, 1, 26,  7) },   // 0x8007352E
      {TX_POWER_9_DBM,        RF_TxPowerTable_HighPAEntry(40, 0, 1, 39, 41) },   // 0x80294F28
      {TX_POWER_10_DBM,       RF_TxPowerTable_HighPAEntry(23, 2, 1, 65,  5) },   // 0x80058397
      {TX_POWER_11_DBM,       RF_TxPowerTable_HighPAEntry(24, 2, 1, 29,  7) },   // 0x80073B98
      {TX_POWER_12_DBM,       RF_TxPowerTable_HighPAEntry(19, 2, 1, 16, 25) },   // 0x80192193
      {TX_POWER_13_DBM,       RF_TxPowerTable_HighPAEntry(27, 2, 1, 19, 13) },   // 0x800D279B
      {TX_POWER_14_DBM,       RF_TxPowerTable_HighPAEntry(24, 2, 1, 19, 27) },   // 0x801B2798
      {TX_POWER_15_DBM,       RF_TxPowerTable_HighPAEntry(23, 2, 1, 20, 39) },   // 0x80272997
      {TX_POWER_16_DBM,       RF_TxPowerTable_HighPAEntry(34, 2, 1, 26, 23) },   // 0x801735A2
      {TX_POWER_17_DBM,       RF_TxPowerTable_HighPAEntry(38, 2, 1, 33, 25) },   // 0x801943A6
      {TX_POWER_18_DBM,       RF_TxPowerTable_HighPAEntry(30, 2, 1, 37, 53) },   // 0x80354B9E
      {TX_POWER_19_DBM,       RF_TxPowerTable_HighPAEntry(36, 2, 1, 57, 59) },   // 0x803B73A4
   {TX_POWER_20_DBM,       RF_TxPowerTable_HighPAEntry(56, 2, 1, 45, 63) }
}; // 0x803F5BB8

#else // unknown board package

#error "***BLE USER CONFIG BUILD ERROR*** Unknown CC135x board type!"

#endif // <board>

#else // unknown platform

#error "ERROR: Unknown platform!"

#endif // <board>

// Tx Power Table
txPwrTbl_t appTxPwrTbl = { TxPowerTable,
                           NUM_TX_POWER_VALUES,  // max
                           DEFAULT_TX_POWER
                         };   // default

ECCParams_CurveParams eccParams_NISTP256 =
{
    .curveType      = ECCParams_CURVE_TYPE_SHORT_WEIERSTRASS,
    .length         = NISTP256_PARAM_SIZE_BYTES,
    .prime          = NISTP256_prime.byte,
    .order          = NISTP256_order.byte,
    .a              = NISTP256_a.byte,
    .b              = NISTP256_b.byte,
    .generatorX     = NISTP256_generator.x.byte,
    .generatorY     = NISTP256_generator.y.byte
};

#ifdef ICALL_JT
#include <icall.h>

#define CLOCK_WRAP_AROUND_VALUE ((uint32_t)(0x80000000)/20)

RF_CmdHandle RF_scheduleCmd_FM(RF_Handle h, RF_Op* pOp, RF_ScheduleCmdParams *pSchParams, RF_Callback pCb, RF_EventMask bmEvent)
{
    RF_CmdHandle rfhndl;
#if DMM_ENABLE && (CONCURRENT_STACKS || BLE_LOCALIZATION_BEFORE_TISCH) && RF_SCHEDULE_KLUGE
    if (dmmOpState != DMM_OP_STATE_BLE_ONLY && !bleConnected &&
            pOp->startTrigger.triggerType == TRIG_ABSTIME)
    {
        uint32_t startTime = pOp->startTime/40;
        uint32_t currTime = RF_getCurrentTime()/40;
        int32_t  cmdStartDelay = startTime - currTime;;

        if (cmdStartDelay < -((int32_t)(CLOCK_WRAP_AROUND_VALUE/2)))
        {
            cmdStartDelay += CLOCK_WRAP_AROUND_VALUE;
        }
        if (cmdStartDelay > 0)
        {
           uint8_t waitCount = 0;
           Task_sleep(cmdStartDelay);  //kluge to deal with the problem of 100% duty cycle of the pending BLE RF command, which messes up the 6tisch

            while(waitCount < 30)  //wait upto 30 slots or 300 msec
            {
                extern unsigned long rtimer_arch_now(void);
                extern unsigned long tsch_next_slot_start_time;
                extern unsigned long TIME_N_MS_TICKS;
                unsigned long currTischTime = rtimer_arch_now();
                unsigned long deltaTime = 0;

                if (currTischTime <= tsch_next_slot_start_time)
                {
                    deltaTime = tsch_next_slot_start_time - currTischTime;
                }
                else if ((tsch_next_slot_start_time + 0x80000000) < currTischTime)
                {
                    deltaTime = 0xffffffff - (currTischTime - tsch_next_slot_start_time) + 1;
                }

                if (deltaTime >= TIME_N_MS_TICKS)
                {
                    break;

                }
                //Wait after the next slot is done.
                bleReqRadioAccess = 1;
                Semaphore_pend(sema_DmmCanSendBleRfCmd, BIOS_WAIT_FOREVER);  //Wait for the time slot tick to start
                waitCount++;
            }
            pOp->startTime = RF_getCurrentTime();
        }
    }
#endif //DMM_ENABLE && CONCURRENT_STACKS

    rfhndl = RF_scheduleCmd(h, pOp, pSchParams, pCb, bmEvent);
    return(rfhndl);
}
// RF Driver API Table
rfDrvTblPtr_t rfDriverTableBLE[] =
{
   (uint32)RF_open,
    (uint32)driverTable_fnSpinlock, // RF_close
#ifdef RF_SINGLEMODE
    (uint32)RF_postCmd,
#else // !RF_SINGLEMODE
    (uint32)driverTable_fnSpinlock, // RF_postCmd
#endif // RF_SINGLEMODE
    (uint32)driverTable_fnSpinlock, // RF_pendCmd
#ifdef RF_SINGLEMODE
    (uint32)RF_runCmd,
#else // !RF_SINGLEMODE
    (uint32)driverTable_fnSpinlock, // RF_runCmd
#endif // RF_SINGLEMODE
    (uint32)RF_cancelCmd,
    (uint32)RF_flushCmd,
    (uint32)driverTable_fnSpinlock, // RF_yield
    (uint32)RF_Params_init,
    (uint32)RF_runImmediateCmd,
    (uint32)RF_runDirectCmd,
    (uint32)RF_ratCompare,
    (uint32)driverTable_fnSpinlock, // RF_ratCapture
    (uint32)RF_ratDisableChannel,
    (uint32)RF_getCurrentTime,
    (uint32)RF_getRssi,
    (uint32)RF_getInfo,
    (uint32)RF_getCmdOp,
    (uint32)RF_control,
    (uint32)driverTable_fnSpinlock, // RF_getTxPower
    (uint32)driverTable_fnSpinlock, // RF_setTxPower
    (uint32)driverTable_fnSpinlock, // RF_TxPowerTable_findPowerLevel
    (uint32)driverTable_fnSpinlock, // RF_TxPowerTable_findValue
#ifndef RF_SINGLEMODE
    (uint32)RF_scheduleCmd_FM,
    (uint32)RF_runScheduleCmd,
    (uint32)driverTable_fnSpinlock, // RF_requestAccess
#endif // !RF_SINGLEMODE
};

cryptoDrvTblPtr_t cryptoDriverTableBLE[] =
{
   (uint32)AESCCM_init,
    (uint32)AESCCM_open,
    (uint32)AESCCM_close,
    (uint32)AESCCM_Params_init,
    (uint32)AESCCM_Operation_init,
    (uint32)AESCCM_oneStepEncrypt,
    (uint32)AESCCM_oneStepDecrypt,
    (uint32)AESECB_init,          
    (uint32)AESECB_open,
    (uint32)AESECB_close,
    (uint32)AESECB_Params_init,
    (uint32)AESECB_Operation_init,
    (uint32)AESECB_oneStepEncrypt,
    (uint32)AESECB_oneStepDecrypt,
    (uint32)CryptoKeyPlaintext_initKey,
   (uint32)CryptoKeyPlaintext_initBlankKey
};

// Swi APIs needed by BLE controller
rtosApiTblPtr_t rtosApiTable[] =
{
  (uint32_t) Swi_disable,
  (uint32_t) Swi_restore,
  (uint32_t) Hwi_disableInterrupt,
  (uint32_t) Hwi_enableInterrupt
};

// BLE Stack Configuration Structure
const stackSpecific_t bleStackConfig =
{
  .maxNumConns                          = MAX_NUM_BLE_CONNS,
  .maxNumPDUs                           = MAX_NUM_PDU,
  .maxPduSize                           = MAX_PDU_SIZE,
  .maxNumPSM                            = L2CAP_NUM_PSM,
  .maxNumCoChannels                     = L2CAP_NUM_CO_CHANNELS,
  .maxWhiteListElems                    = MAX_NUM_WL_ENTRIES,
  .maxResolvListElems                   = MAX_NUM_RL_ENTRIES,
  .pfnBMAlloc                           = &pfnBMAlloc,
  .pfnBMFree                            = &pfnBMFree,
  .rfDriverParams.powerUpDurationMargin = RF_POWER_UP_DURATION_MARGIN,
  .rfDriverParams.inactivityTimeout     = RF_INACTIVITY_TIMEOUT,
  .rfDriverParams.powerUpDuration       = RF_POWER_UP_DURATION,
  .rfDriverParams.pErrCb                = &(RF_ERR_CB),
  .eccParams                            = &eccParams_NISTP256,
  .fastStateUpdateCb                    = RemoteDisplay_bleFastStateUpdateCb,
  .bleStackType                         = DMMPolicy_StackType_BlePeripheral
};

#ifdef OSAL_SNV_EXTFLASH
const extflashDrvTblPtr_t extflashDriverTable[] =
{
  (uint32) ExtFlash_open,
  (uint32) ExtFlash_close,
  (uint32) ExtFlash_read,
  (uint32) ExtFlash_write,
  (uint32) ExtFlash_erase
};
#endif // OSAL_SNV_EXTFLASH

// Table for Driver can be found in icall_user_config.c
// if a driver is not to be used, then the pointer shoul dbe set to NULL,
// for this example, this is done in ble_user_config.h
const drvTblPtr_t driverTable =
{
  .rfDrvTbl       = rfDriverTableBLE,
  .eccDrvTbl      = eccDriverTable,
  .cryptoDrvTbl   = cryptoDriverTableBLE,
  .trngDrvTbl     = trngDriverTable,
  .rtosApiTbl     = rtosApiTable,
  .nvintfStructPtr  = &nvintfFncStruct,
#ifdef OSAL_SNV_EXTFLASH
  .extflashDrvTbl = extflashDriverTable,
#endif // OSAL_SNV_EXTFLASH
};

const boardConfig_t boardConfig =
{
  .rfFeModeBias = RF_FE_MODE_AND_BIAS,
  .rfRegTbl      = rfRegTbl,
  .rfRegTbl1M    = rfRegTbl1M,
#if defined(BLE_V50_FEATURES) && (BLE_V50_FEATURES & (PHY_2MBPS_CFG | PHY_LR_CFG))
  // Currently, no overrides for 2M and Coded, so exclude from build.
  .rfRegTbl2M    = rfRegTbl2M,
  .rfRegTblCoded = rfRegTblCoded,
#endif // PHY_2MBPS_CFG | PHY_LR_CFG
  .txPwrTbl      = &appTxPwrTbl
};

#else /* !(ICALL_JT) */

// RF Driver API Table
rfDrvTblPtr_t rfDriverTable[] =
{
  (uint32) RF_open,
  (uint32) driverTable_fnSpinlock, // RF_close
#ifdef RF_SINGLEMODE
  (uint32) RF_postCmd,
#else // !RF_SINGLEMODE
  (uint32) driverTable_fnSpinlock, // RF_postCmd
#endif// RF_SINGLEMODE
  (uint32) driverTable_fnSpinlock, // RF_pendCmd
#ifdef RF_SINGLEMODE
  (uint32) RF_runCmd,
#else // !RF_SINGLEMODE
  (uint32) driverTable_fnSpinlock, // RF_runCmd
#endif// RF_SINGLEMODE
  (uint32) RF_cancelCmd,
  (uint32) RF_flushCmd,
  (uint32) driverTable_fnSpinlock, // RF_yield
  (uint32) RF_Params_init,
  (uint32) RF_runImmediateCmd,
  (uint32) RF_runDirectCmd,
  (uint32) RF_ratCompare
  (uint32) driverTable_fnSpinlock, // RF_ratCapture
  (uint32) driverTable_fnSpinlock, // RF_ratDisableChannel
  (uint32) RF_getCurrentTime,
  (uint32) RF_getRssi,
  (uint32) RF_getInfo,
  (uint32) RF_getCmdOp,
  (uint32) RF_control,
  (uint32) driverTable_fnSpinlock, // RF_getTxPower
  (uint32) driverTable_fnSpinlock, // RF_setTxPower
  (uint32) driverTable_fnSpinlock, // RF_TxPowerTable_findPowerLevel
  (uint32) driverTable_fnSpinlock, // RF_TxPowerTable_findValue
#ifndef RF_SINGLEMODE
  (uint32) RF_scheduleCmd,
  (uint32) RF_runScheduleCmd,
  (uint32) driverTable_fnSpinlock  // RF_requestAccess
#endif // !RF_SINGLEMODE
};

// ECC Driver API Table
eccDrvTblPtr_t eccDriverTable[] =
{
  (uint32) ECDH_init,
  (uint32) ECDH_Params_init,
  (uint32) ECDH_open,
  (uint32) ECDH_close,
  (uint32) ECDH_OperationGeneratePublicKey_init,
  (uint32) ECDH_OperationComputeSharedSecret_init,
  (uint32) ECDH_generatePublicKey,
  (uint32) ECDH_computeSharedSecret
};

// Crypto Driver API Table
cryptoDrvTblPtr_t cryptoDriverTable[] =
{
  (uint32)AESCCM_init,          
  (uint32)AESCCM_open,
  (uint32)AESCCM_close,
  (uint32)AESCCM_Params_init,
  (uint32)AESCCM_Operation_init,
  (uint32)AESCCM_oneStepEncrypt,
  (uint32)AESCCM_oneStepDecrypt,
  (uint32)AESECB_init,          
  (uint32)AESECB_open,
  (uint32)AESECB_close,
  (uint32)AESECB_Params_init,
  (uint32)AESECB_Operation_init,
  (uint32)AESECB_oneStepEncrypt,
  (uint32)AESECB_oneStepDecrypt,
  (uint32)CryptoKeyPlaintext_initKey,
  (uint32)CryptoKeyPlaintext_initBlankKey
};

trngDrvTblPtr_t trngDriverTable[] =
{
  (uint32) TRNGCC26XX_init,
  (uint32) TRNGCC26XX_open,
  (uint32) TRNGCC26XX_getNumber
};


#endif /* ICALL_JT */

/*******************************************************************************
 * @fn          RegisterAssertCback
 *
 * @brief       This routine registers the Application's assert handler.
 *
 * input parameters
 *
 * @param       appAssertHandler - Application's assert handler.
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      None.
 */
void RegisterAssertCback(assertCback_t appAssertHandler)
{
  appAssertCback = appAssertHandler;

#ifdef EXT_HAL_ASSERT
  // also set the Assert callback pointer used by halAssertHandlerExt
  // Note: Normally, this pointer will be intialized by the stack, but in the
  //       event HAL_ASSERT is used by the Application, we initialize it
  //       directly here.
  halAssertCback = appAssertHandler;
#endif // EXT_HAL_ASSERT

  return;
}

/*******************************************************************************
 * @fn          driverTable_fnSpinLock
 *
 * @brief       This routine is used to trap calls to unpopulated indexes of
 *              driver function pointer tables.
 *
 * input parameters
 *
 * @param       None.
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      None.
 */
void driverTable_fnSpinlock(void)
{
  volatile uint8 i = 1;

  while(i);
}

/*******************************************************************************
 * @fn          DefaultAssertCback
 *
 * @brief       This is the Application default assert callback, in the event
 *              none is registered.
 *
 * input parameters
 *
 * @param       assertCause    - Assert cause as defined in hal_assert.h.
 * @param       assertSubcause - Optional assert subcause (see hal_assert.h).
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      None.
 */
void DefaultAssertCback(uint8 assertCause, uint8 assertSubcause)
{
#ifdef HAL_ASSERT_SPIN
  driverTable_fnSpinlock();
#endif // HAL_ASSERT_SPIN

  return;
}

// Application Assert Callback Function Pointer
assertCback_t appAssertCback = DefaultAssertCback;

/*******************************************************************************
 */
#endif //DMM_ENABLE
