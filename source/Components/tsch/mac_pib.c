/*
* Copyright (c) 2010-2014 Texas Instruments Incorporated
*
* All rights reserved not granted herein.
* Limited License. 
*
* Texas Instruments Incorporated grants a world-wide, royalty-free,
* non-exclusive license under copyrights and patents it now or hereafter
* owns or controls to make, have made, use, import, offer to sell and sell ("Utilize")
* this software subject to the terms herein.  With respect to the foregoing patent
*license, such license is granted  solely to the extent that any such patent is necessary
* to Utilize the software alone.  The patent license shall not apply to any combinations which
* include this software, other than combinations with devices manufactured by or for TI (“TI Devices”). 
* No hardware patent is licensed hereunder.
*
* Redistributions must preserve existing copyright notices and reproduce this license (including the
* above copyright notice and the disclaimer and (if applicable) source code license limitations below)
* in the documentation and/or other materials provided with the distribution
*
* Redistribution and use in binary form, without modification, are permitted provided that the following
* conditions are met:
*
*       * No reverse engineering, decompilation, or disassembly of this software is permitted with respect to any
*     software provided in binary form.
*       * any redistribution and use are licensed by TI for use only with TI Devices.
*       * Nothing shall obligate TI to provide you with source code for the software licensed and provided to you in object code.
*
* If software source code is provided to you, modification and redistribution of the source code are permitted
* provided that the following conditions are met:
*
*   * any redistribution and use of the source code, including any resulting derivative works, are licensed by
*     TI for use only with TI Devices.
*   * any redistribution and use of any object code compiled from the source code and any resulting derivative
*     works, are licensed by TI for use only with TI Devices.
*
* Neither the name of Texas Instruments Incorporated nor the names of its suppliers may be used to endorse or
* promote products derived from this software without specific prior written permission.
*
* DISCLAIMER.
*
* THIS SOFTWARE IS PROVIDED BY TI AND TI’S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TI’S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== mac_pib.c =============================================
 *  
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "lib/random.h"

#include "mac_pib.h"
#include "mac_pib_pvt.h"
#include "hw_memmap.h"
#include "lowmac.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */


/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */



/* TSCH PIB default values */
static const TSCH_MAC_PIB_t TSCH_MAC_PIB_Defaults =
{
    0x00,                                                   /* bsn */
    0x00,                                                   /* dsn */
    3,                                                      /* maxFrameRetries */
    1,                                                      /* min BE */
    7,                                                      /* max BE */
    MAC_SHORT_ADDR_NONE,                                    /* shortAddress */
    0xFFFF,                                                 /* panId */
    MAC_SHORT_ADDR_NONE,                                    /* PAN coordinator short address */
#ifdef FEATURE_MAC_SECURITY
   true,                                                   /* securityEnabled */
#else
   false,                                                  /* securityEnabled */
#endif
    false,                                                  /* PAN Cordinator */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},              /* coordinator EUI  */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}              /* Dev EUI  */
};

/* PIB access and min/max table.
 * if min/max of 0/0 means not checked;
 * if min/max are  equal, element is read-only
 */

static const MAC_PIB_TBL_t TSCH_MAC_PIB_Table[] =
{
  {offsetof(TSCH_MAC_PIB_t, bsn), sizeof(uint8_t), 0x00, 0xFF},                     /* MAC_BSN */
  {offsetof(TSCH_MAC_PIB_t, dsn), sizeof(uint8_t), 0x00, 0xFF},                     /* MAC_DSN */
  {offsetof(TSCH_MAC_PIB_t, maxFrameRetries), sizeof(uint8_t), 1, 7},               /* MAC_MAX_FRAME_RETRIES */
  {offsetof(TSCH_MAC_PIB_t, minBe), sizeof(uint8_t), 1, 7},                         /* min BE */
  {offsetof(TSCH_MAC_PIB_t, maxBe), sizeof(uint8_t), 1, 7},                         /* max BE */

  {offsetof(TSCH_MAC_PIB_t, shortAddress), sizeof(uint16_t), 0, 0},                 /* MAC_SHORT_ADDRESS */
  {offsetof(TSCH_MAC_PIB_t, panId), sizeof(uint16_t), 0, 0},                        /* MAC_PAN_ID */
  {offsetof(TSCH_MAC_PIB_t, macCoordinatorShortAddress), sizeof(uint16_t), 0, 0},   /* PAN coordinator short address */
  {offsetof(TSCH_MAC_PIB_t, securityEnabled), sizeof(bool), false, true},           /* MAC_SECURITY_ENABLED */
  {offsetof(TSCH_MAC_PIB_t, PanCoordinator), sizeof(bool), false, true},            /* Pan coordinator*/

  {offsetof(TSCH_MAC_PIB_t, coordinatorEUI), TSCH_DEVICE_EUI_LEN, 0, 0},            /* PAN coordinator_EUI */
  {offsetof(TSCH_MAC_PIB_t, devEUI), TSCH_DEVICE_EUI_LEN, 0, 0},                    /* MAC_DEVICE_EUI */
};


/* Invalid PIB table index used for error code */
#define MAC_PIB_INVALID     ((uint8_t) (sizeof(TSCH_MAC_PIB_Table) / sizeof(TSCH_MAC_PIB_Table[0])))

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

/* MAC PIB */
TSCH_MAC_PIB_t TSCH_MAC_Pib;

/**************************************************************************************************//**
 *
 * @brief       This function initializes the default PIB for TSCH MAC PIB
 *              set up the initial dsn and bsn (random number). It also read the EUI from Flash
 *              and store in EUI field.
 *
 * @param       None.
 *
 * @return
 *
 ***************************************************************************************************/
void TSCH_PIB_init(void)
{
    uint8_t idx;
    uint8_t *pFlashEUIAddr;
    uint8_t *pEUIAddr;

    /* copy PIB defaults */
    TSCH_MAC_Pib = TSCH_MAC_PIB_Defaults;


    /* initialize random sequence numbers */
    TSCH_MAC_Pib.dsn = random_rand() & 0xff;

    //TSCH_MAC_Pib.bsn = macRadioRandomByte();
    TSCH_MAC_Pib.bsn = 0;
    TSCH_MAC_Pib.PanCoordinator = false;

    /* read EUI from device flash and store in EUI */
    pFlashEUIAddr = (uint8_t *) ( FCFG1_BASE + EXTADDR_OFFSET);
    pEUIAddr      = (uint8_t *) (&TSCH_MAC_Pib.devEUI[0]);
    for (idx=0;idx<TSCH_DEVICE_EUI_LEN;idx++)
    {
        *pEUIAddr++ = *pFlashEUIAddr++;
    }

}

/**************************************************************************************************//**
 *
 * @brief       This function takes an PIB attribute and returns the index in to
 *              TSCH_MAC_PIB_Table for the attribute.
 *
 * @param[in]   pibAttribute - PIB attribute to look up. This attribute ID is defined in 802.15.4e
 *              spec. From the MAC PIB table, we can get the offset, length, min and max values for
 *              this partular PIB.
 *
 * @return      Index in to TSCH_MAC_PIB_Table for the attribute or MAC_PIB_INVALID.
 *
 ***************************************************************************************************/
uint16_t TSCH_PIB_getIndex(uint16_t pibAttribute)
{

    /*
    The data order in data structure is very important
    */
    switch (pibAttribute)
    {
        case TSCH_MAC_PIB_ID_macBSN: return 0;
        case TSCH_MAC_PIB_ID_macDSN: return 1;
        case TSCH_MAC_PIB_ID_macMaxFrameRetries: return 2;
        case TSCH_MAC_PIB_ID_minBE: return 3;
        case TSCH_MAC_PIB_ID_maxBE: return 4;

        case TSCH_MAC_PIB_ID_macShortAddress: return 5;
        case TSCH_MAC_PIB_ID_macPANId: return 6;
        case TSCH_MAC_PIB_ID_macPANCoordinatorShortAddress: return 7;
        case TSCH_MAC_PIB_ID_macSecurityEnabled: return 8;
        case TSCH_MAC_PIB_ID_PANCoordinator: return 9;

        case TSCH_MAC_PIB_ID_PAN_COORDINATOR_EUI: return 10;
        case TSCH_MAC_PIB_ID_DEV_EUI: return  11;

        default:
            return MAC_PIB_INVALID;
    }

}

/**************************************************************************************************//**
 *
 * @brief       This direct execute function gets the TSCH MAC PIB attribute size
 *
 *
 * @param[in]   pibAttribute - The attribute identifier.
 *
 * @return      size in bytes. if the attribute is not supported, it will return 0.
 *
 ***************************************************************************************************/
uint16_t TSCH_MlmeGetReqSize(uint16_t pibAttribute )
{
    uint16_t index;

    if ((index = TSCH_PIB_getIndex(pibAttribute)) == MAC_PIB_INVALID)
    {
        return 0;
    }

    return ( TSCH_MAC_PIB_Table[index].len );
}

/**************************************************************************************************//**
 *
 * @brief       This direct execute function retrieves an attribute value
 *              from the TSCH MAC PIB. The caller should reserve the enough space to hold the
 *              returned value. otherwise it will corrupt memory.
 *
 * @param[in]   pibAttribute - The attribute identifier.
 *
 * @param[out]  pValue - pointer to the returned attribute value.
 *
 * @return      The status of the request, as follows:
 *              MAC_PIB_STATUS_SUCCESS Operation successful.
 *              MAC_PIB_STATUS_UNSUPPORTED_ATTRIBUTE Attribute not found.
 *
 ***************************************************************************************************/
uint16_t TSCH_MlmeGetReq(uint16_t pibAttribute, void *pValue)
{
    uint16_t           idx;
    //halIntState_t     intState;

    if ((idx = TSCH_PIB_getIndex(pibAttribute)) == MAC_PIB_INVALID)
    {
        return MAC_UNSUPPORTED_ATTRIBUTE;
    }

    //HAL_ENTER_CRITICAL_SECTION(intState);
    uint32_t key = Task_disable();
    memcpy(pValue, (uint8_t *) &TSCH_MAC_Pib + TSCH_MAC_PIB_Table[idx].offset, TSCH_MAC_PIB_Table[idx].len);
    Task_restore(key);
    //HAL_EXIT_CRITICAL_SECTION(intState);

    return MAC_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This direct execute function sets an attribute value
 *              in the TSCH MAC PIB.
 *
 * @param[in]   pibAttribute - The attribute identifier.
 *              pValue - pointer to the attribute value.
 *
 *
 * @return      The status of the request, as follows:
 *              MAC_SUCCESS: Operation successful.
 *              MAC_UNSUPPORTED_ATTRIBUTE: Attribute not found.
 *              MAC_READ_ONLY: Attribute read only
 *              MAC_INVALID_PARAMETER: parameter is bad (not in range)
 *
 ***************************************************************************************************/
uint16_t TSCH_MlmeSetReq(uint16_t pibAttribute, void *pValue)
{
  uint16_t         idx;
  //halIntState_t intState;

  /* look up attribute in PIB table */
  if ((idx = TSCH_PIB_getIndex(pibAttribute)) == MAC_PIB_INVALID)
  {
    return MAC_UNSUPPORTED_ATTRIBUTE;
  }

  /* do range check; no range check if min and max are zero */
  if ((TSCH_MAC_PIB_Table[idx].min != 0) || (TSCH_MAC_PIB_Table[idx].max != 0))
  {
    /* if min == max, this is a read-only attribute */
    if (TSCH_MAC_PIB_Table[idx].min == TSCH_MAC_PIB_Table[idx].max)
    {
      return MAC_READ_ONLY;
    }

    /* range check for general case */
    if ((*((uint8_t *) pValue) < TSCH_MAC_PIB_Table[idx].min) || (*((uint8_t *) pValue) > TSCH_MAC_PIB_Table[idx].max))
    {
      return MAC_INVALID_PARAMETER;
    }

  }
  uint32_t key = Task_disable();
  memcpy((uint8_t *) &TSCH_MAC_Pib + TSCH_MAC_PIB_Table[idx].offset,pValue,TSCH_MAC_PIB_Table[idx].len);
  Task_restore(key);

  return MAC_SUCCESS;
}
