/*
* Copyright (c) 2015 Texas Instruments Incorporated
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
* include this software, other than combinations with devices manufactured by or for TI (â€œTI Devicesâ€�).
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
* THIS SOFTWARE IS PROVIDED BY TI AND TIâ€™S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TIâ€™S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== nhl_device_table.c =============================================
 *  
 */

/*******************************************************************************
 * INCLUDES
 */
#include "nhl_device_table.h"
#include "mac_config.h"

/*******************************************************************************
* LOCAL VARIABLES
*/
//static NHL_DeviceTable_t NHL_deviceTable[NODE_MAX_NUM];
NHL_DeviceTable_t NHL_deviceTable[NODE_MAX_NUM];
static uint16_t nextEntry =0;

#if IS_ROOT
static uint32_t NHL_rxTotalTable[NODE_MAX_NUM];
#endif

/*******************************************************************************
 * APIs
 */
/**************************************************************************************************
* @fn          NHL_deviceTableInit
*
* @brief       This function initializes the device table.
*
* Input parameters
*
* None
*
* output parameters
*
* None
*
* @return      None
**************************************************************************************************
*/
void NHL_deviceTableInit(void)
{
    memset(NHL_deviceTable,0x0,sizeof(NHL_deviceTable));
    nextEntry = 0;

#if IS_ROOT
    memset(NHL_rxTotalTable,0x0,sizeof(NHL_rxTotalTable));
    
    uint16_t nodeIter;
    for (nodeIter=0; nodeIter<NODE_MAX_NUM; nodeIter++)
    {
        memset(NHL_deviceTable[nodeIter].timeslot, 0xffff, sizeof(uint16_t)*MAX_NUM_LINKS_IN_ASSOC);
        memset(NHL_deviceTable[nodeIter].channelOffset, 0xff, sizeof(uint8_t)*MAX_NUM_LINKS_IN_ASSOC);
        memset(NHL_deviceTable[nodeIter].periodOffset, 0xff, sizeof(uint8_t)*MAX_NUM_LINKS_IN_ASSOC);
    }
#endif
}

/**************************************************************************************************
* @fn          NHL_deviceTableUpdateShortAddr
*
* @brief       This function looks up the device table and checks if a short address
*              has been assigned.  If yes, the short address is
*              updated to the new one.
*
* input parameters
*
* @param       oldShortAddr: the current short address
*              newShortAddr: the new short address
*
* output parameters
*
* None
*
* @return      1 if the address is updated in device table, otherwise 0.
**************************************************************************************************
*/
uint16_t NHL_deviceTableUpdateShortAddr(uint16_t oldShortAddr, uint16_t newShortAddr)
{
    uint16_t iter;
    uint16_t retVal = 0;

    for (iter = 0; iter < nextEntry; iter++)
    {
       if (NHL_deviceTable[iter].shortAddr == oldShortAddr)
       {
          retVal = 1;
          NHL_deviceTable[iter].shortAddr = newShortAddr;
          break;
       }
    }

#ifdef FEATURE_MAC_SECURITY
    macSecurityDeviceUpdate(oldShortAddr, newShortAddr);
#endif

    return(retVal);
}

/**************************************************************************************************
* @fn          NHL_deviceTableGetShortAddr
*
* @brief       This function looks up the device table and checks if a short address
*              has been assigned for a given long address.
*
* input parameters
*
* @param       pLongAddr: pointer to MAC long address
*
* output parameters
*
* None
*
* @return      short address if the long address is stored in device table, otherwise 0.
**************************************************************************************************
*/
uint16_t
NHL_deviceTableGetShortAddr(const ulpsmacaddr_t* longAddr)
{
    uint16_t iter;
    int16_t found =-1;

    for (iter=0; iter < nextEntry; iter++)
    {
       if (ulpsmacaddr_long_cmp(&(NHL_deviceTable[iter].extAddr), longAddr))
       {
           found = iter;
           break;
       }
    }

    if (found >= 0)
    {
        return NHL_deviceTable[found].shortAddr;
    }
    else
    {
        return 0;
    }
}

/**************************************************************************************************
* @fn          NHL_deviceTableGetLongAddr
*
* @brief       This function looks up the device table and checks if a long address
*              has been assigned for a given short address.
*
* input parameters
*
* @param       shortAddr: short address
*
* output parameters
*
* None
*
* @return      long address if the short address is stored in device table, otherwise 0.
**************************************************************************************************
*/
ulpsmacaddr_t  NHL_deviceTableGetLongAddr(uint16_t shortAddr)
{
    uint16_t iter;
    int16_t found =-1;

    for (iter=0; iter < nextEntry; iter++)
    {
        if (NHL_deviceTable[iter].shortAddr == shortAddr)
        {
            found = iter;
            break;
        }
    }

    if (found >= 0)
    {
        return NHL_deviceTable[found].extAddr;
    }
    else
    {
        return ulpsmacaddr_null;
    }
}

/**************************************************************************************************
* @fn          NHL_GetTableIndex
*
* @brief       This function looks up the device table and checks if a short address
*              has been stored.
*
* input parameters
*
* @param       deviceShortAddr:  short address to be looked up
*
* output parameters
*
* None
*
* @return      Table index if a given short address is found, NODE_MAX_NUM otherwise.
**************************************************************************************************
*/
uint16_t NHL_getTableIndex(uint16_t shortAddr)
{
    uint16_t iter;
    uint16_t found=NODE_MAX_NUM;


    for (iter=0; iter < nextEntry; iter++)
    {
        if (NHL_deviceTable[iter].shortAddr == shortAddr)
        {
            found = iter;
            break;
        }
    }

    return found;
}

/**************************************************************************************************
* @fn          NHL_deviceTableGetMacSeqNumber
*
* @brief       This function returns the sequence number of the last received MAC packet for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table

*
* output parameters
*
* @param       None
*
* @return      the last received MAC packet sequence number if successful, otherwise 0xFFFF.
*
**************************************************************************************************
*/
uint16_t NHL_deviceTableGetMacSeqNumber(uint16_t tableIndex)
{
    if (tableIndex < nextEntry)
    {
        return NHL_deviceTable[tableIndex].lastMacSeqNumber;
    }
    else
    {
        return 0xFFFF;
    }
}

/**************************************************************************************************
* @fn          NHL_deviceTableSetMacSeqNumber
*
* @brief       This function sets the sequence number of the last received MAC packet for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table
*              macSeqNumber: the latest received MAC packet sequence number
* output parameters
*
* @param       None
*
* @return      None
*
**************************************************************************************************
*/
void NHL_deviceTableSetMacSeqNumber(uint16_t tableIndex, uint16_t macSeqNumber)
{
    if (tableIndex < nextEntry)
    {
        NHL_deviceTable[tableIndex].lastMacSeqNumber = macSeqNumber;
    }
}

/**************************************************************************************************
* @fn          NHL_deviceTableGetRxTotalNumber
*
* @brief       This function returns the counted number of the totally received MAC packet for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table

*
* output parameters
*
* @param       None
*
* @return      the counted totally received number in mac, otherwise 0xFFFFFFFE.
*
**************************************************************************************************
*/
#if IS_ROOT
uint32_t NHL_deviceTableGetRxTotalNumber(uint8_t tableIndex)
{
    if (tableIndex < 200)
    {
        return NHL_rxTotalTable[tableIndex];
    }
    else
    {
        return 0xFFFFFFFE;
    }
}

#else
uint32_t NHL_deviceTableGetRxTotalNumber(uint16_t tableIndex)
{
    if (tableIndex < nextEntry)
    {
        return NHL_deviceTable[tableIndex].rxTotal;
    }
    else
    {
        return 0xFFFFFFFE;
    }
}
#endif

/**************************************************************************************************
* @fn          NHL_deviceTableSetRxTotalNumber
*
* @brief       This function sets the counted number of the totally received MAC packet for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table
*              macRxToal: the totally received number of mac packets
* output parameters
*
* @param       None
*
* @return      None
*
**************************************************************************************************
*/
#if IS_ROOT

void NHL_deviceTableSetRxTotalNumber(uint8_t tableIndex, uint32_t macRxTotal)
{
    if (tableIndex < 200)
    {
        NHL_rxTotalTable[tableIndex] = macRxTotal;
    }
}

#else

void NHL_deviceTableSetRxTotalNumber(uint16_t tableIndex, uint32_t macRxTotal)
{
    if (tableIndex < nextEntry)
    {
        NHL_deviceTable[tableIndex].rxTotal = macRxTotal;
    }
}

#endif



#if IS_ROOT

void NHL_markDepartedTableEntry(uint16_t shortAddr)                     
{
   uint16_t iter;

   for (iter = 0; iter < nextEntry; iter++)
   {
      if (NHL_deviceTable[iter].shortAddr == shortAddr)
      {
         NHL_deviceTable[iter].timeslot[0] = 0xfffe;
      }
   }
}

void NHL_compactDeviceTable()                   //need to change
{
   uint16_t iter;
   uint16_t i;

   for (iter = 0; iter < nextEntry; iter++)
   {
      if (NHL_deviceTable[iter].timeslot[0] == 0xfffe)
      {
#ifdef FEATURE_MAC_SECURITY
         macSecurityDeviceRemove(NHL_deviceTable[iter].shortAddr);
#endif
         nextEntry--;
         for (i = iter; i < nextEntry; i++)
         {
            NHL_deviceTable[i] = NHL_deviceTable[i+1];
         }
         memset(&(NHL_deviceTable[nextEntry]), 0, sizeof(NHL_deviceTable[nextEntry]));
         break;
      }
   }
}

/**************************************************************************************************
* @fn          NHL_deviceTableNewShortAddr
*
* @brief       This function assigns a new short address to a given node if
*              it is not assigned.  If a short address has been assigned, it will
*              return the found short address.
*
* input parameters
*
* @param       pLongAddr: pointer to MAC long address
*
* output parameters
*
* None
*
* @return      short address if successful, otherwise 0.
**************************************************************************************************
*/
uint16_t NHL_deviceTableNewShortAddr(const ulpsmacaddr_t* pLongAddr)
{
    static uint16_t shortAddrCount=3;
    uint16_t shortAddr;

    shortAddr = NHL_deviceTableGetShortAddr(pLongAddr);
    if (shortAddr > 0)
    {
        return shortAddr;
    }
    else
    {
        if (nextEntry == NODE_MAX_NUM)
        {
           NHL_compactDeviceTable();
        }

        if (nextEntry != NODE_MAX_NUM)
        {
            ulpsmacaddr_long_copy(&(NHL_deviceTable[nextEntry].extAddr), pLongAddr);
            NHL_deviceTable[nextEntry].lastMacSeqNumber = 0xffff;
            if (TSCH_MACConfig.restrict_to_node !=0xffff)
            {
                NHL_deviceTable[nextEntry].shortAddr = shortAddrCount ;
                shortAddrCount++;
            }
            else
            {
                shortAddr = (pLongAddr->u8[1]<<8 ) | pLongAddr->u8[0];
                NHL_deviceTable[nextEntry].shortAddr = shortAddr ;
            }

#ifdef FEATURE_MAC_SECURITY
            macSecurityDeviceAdd(NHL_deviceTable[nextEntry].shortAddr);
#endif

            nextEntry++;

            return NHL_deviceTable[nextEntry-1].shortAddr;
        }
        else
        {
            return 0;
        }
    }
}

/**************************************************************************************************
* @fn          NHL_deviceTableDelAddr
*
* @brief       This function will deletes the node from device table.
*
* input parameters
*
* @param       pLongAddr: pointer to MAC long address
*
* output parameters
*
* @param       None
*
* @return      0: success
*              1: failure
**************************************************************************************************
*/
uint16_t NHL_deviceTableDelAddr(const ulpsmacaddr_t* long_addr)
{
    uint16_t iter,link;
    uint16_t timeslot;
    NHL_DeviceTable_t *pNode;
    int i;
    uint16_t retVal = 1;

    for (iter=0; iter < nextEntry; iter++)
    {
        pNode = &(NHL_deviceTable[iter]);
        if (ulpsmacaddr_long_cmp(&(pNode->extAddr), long_addr))
        {   // found this device
            // free all allocated links
            for (link =0;link<MAX_NUM_LINKS_IN_ASSOC;link++)
            {
                timeslot = pNode->timeslot[link];

                if (timeslot != 0xffff)
                {
                    NHLDB_TSCH_SCHED_del_schedule_link(timeslot);
                    // mark it is free
                    pNode->timeslot[link] = 0xffff;
                    pNode->channelOffset[link] =0xff;;
                    pNode->periodOffset[link] =0xff;;
                }

            }

#ifdef FEATURE_MAC_SECURITY
            macSecurityDeviceRemove(pNode->shortAddr);
#endif
            nextEntry--;
            for (i = iter; i < nextEntry; i++)
            {
               NHL_deviceTable[i] = NHL_deviceTable[i+1];
            }
            // reset to zero
            memset(&(NHL_deviceTable[nextEntry]),0,sizeof(NHL_deviceTable[nextEntry]));
            memset(NHL_deviceTable[nextEntry].timeslot, 0xffff, sizeof(uint16_t)*MAX_NUM_LINKS_IN_ASSOC);
            memset(NHL_deviceTable[nextEntry].channelOffset, 0xff, sizeof(uint8_t)*MAX_NUM_LINKS_IN_ASSOC);
            memset(NHL_deviceTable[nextEntry].periodOffset, 0xff, sizeof(uint8_t)*MAX_NUM_LINKS_IN_ASSOC);

            retVal = 0;
            iter--;
        }

    }

    return(retVal);
}

/**************************************************************************************************
* @fn          NHL_deviceTableGetTimeSlot
*
* @brief       This function returns the timeslot of a link for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table
*              linkIter: link index of a node
*
* output parameters
*
* @param       None
*
* @return      timeslot if successful, otherwise 0.
*
**************************************************************************************************
*/
uint16_t NHL_deviceTableGetTimeSlot(uint16_t tableIndex, uint8_t linkIter)
{
    if ((tableIndex < nextEntry) && (linkIter < MAX_NUM_LINKS_IN_ASSOC))
    {
        return NHL_deviceTable[tableIndex].timeslot[linkIter];
    }
    else
    {
        return 0;
    }
}

/**************************************************************************************************
* @fn          NHL_deviceTableSetTimeSlot
*
* @brief       This function sets the timeslot of a link for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table
*              linkIter: link index of a node
*              timeslot: timeslot value to be set
* output parameters
*
* @param       None
*
* @return      None
*
**************************************************************************************************
*/
void NHL_deviceTableSetTimeSlot(uint16_t tableIndex, uint8_t linkIter, uint16_t timeslot)
{
    if ((tableIndex < nextEntry) && (linkIter < MAX_NUM_LINKS_IN_ASSOC))
    {
        NHL_deviceTable[tableIndex].timeslot[linkIter] = timeslot;
    }
}

/**************************************************************************************************
* @fn          NHL_deviceTableGetChannelOffset
*
* @brief       This function returns the channel offset of a link for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table
*              linkIter: link index of a node
*
* output parameters
*
* @param       None
*
* @return      channel offset if successful, otherwise 0.
*
**************************************************************************************************
*/
uint8_t NHL_deviceTableGetChannelOffset(uint16_t tableIndex, uint8_t linkIter)
{
    if ((tableIndex < nextEntry) && (linkIter < MAX_NUM_LINKS_IN_ASSOC))
    {
        return NHL_deviceTable[tableIndex].channelOffset[linkIter];
    }
    else
    {
        return 0;
    }
}

/**************************************************************************************************
* @fn          NHL_deviceTableSetChannelOffset
*
* @brief       This function sets the channel offset of a link for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table
*              linkIter: link index of a node
*              channelOffset: channel offset value to be set
* output parameters
*
* @param       None
*
* @return      None
*
**************************************************************************************************
*/
void NHL_deviceTableSetChannelOffset(uint16_t tableIndex, uint8_t linkIter, uint8_t channelOffset)
{
    if ((tableIndex < nextEntry) && (linkIter < MAX_NUM_LINKS_IN_ASSOC))
    {
        NHL_deviceTable[tableIndex].channelOffset[linkIter] = channelOffset;
    }
}

/**************************************************************************************************
* @fn          NHL_deviceTableGetPeriodOffset
*
* @brief       This function returns the period offset of a link for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table
*              linkIter: link index of a node
*
* output parameters
*
* @param       None
*
* @return      period offset if successful, otherwise 0.
*
**************************************************************************************************
*/
uint8_t NHL_deviceTableGetPeriodOffset(uint16_t tableIndex, uint8_t linkIter)
{
    if ((tableIndex < nextEntry) && (linkIter < MAX_NUM_LINKS_IN_ASSOC))
    {
        return NHL_deviceTable[tableIndex].periodOffset[linkIter];
    }
    else
    {
        return 0;
    }
}

/**************************************************************************************************
* @fn          NHL_deviceTableSetPeriodOffset
*
* @brief       This function sets the channel offset of a link for a node in device table
*
* input parameters
*
* @param       tableIndex: node index of device table
*              linkIter: link index of a node
*              periodOffset: period offset value to be set
* output parameters
*
* @param       None
*
* @return      None
*
**************************************************************************************************
*/
void NHL_deviceTableSetPeriodOffset(uint16_t tableIndex, uint8_t linkIter, uint8_t periodOffset)
{
    if ((tableIndex < nextEntry) && (linkIter < MAX_NUM_LINKS_IN_ASSOC))
    {
        NHL_deviceTable[tableIndex].periodOffset[linkIter] = periodOffset;
    }
}
#else
void NHL_deleteTableEntry(uint16_t shortAddr,const ulpsmacaddr_t *pLongAddr)
{
   uint16_t iter;
   uint16_t i;

   for (iter=0; iter < nextEntry; iter++)
   {
      if (NHL_deviceTable[iter].shortAddr == shortAddr ||
            (pLongAddr != NULL && memcmp(pLongAddr->u8, NHL_deviceTable[iter].extAddr.u8, sizeof(pLongAddr->u8)) == 0))
      {
#ifdef FEATURE_MAC_SECURITY
         macSecurityDeviceRemove(shortAddr);
#endif
         nextEntry--;
         for (i = iter; i < nextEntry; i++)
         {
            NHL_deviceTable[i] = NHL_deviceTable[i+1];
         }
         memset(&(NHL_deviceTable[nextEntry]), 0, sizeof(NHL_deviceTable[nextEntry]));
         iter--;
      }
   }
}

uint8_t NHL_newTableEntry(uint16_t shortAddr,const ulpsmacaddr_t *pLongAddr)
{
    NHL_deleteTableEntry(shortAddr, pLongAddr);
    if (nextEntry != NODE_MAX_NUM)
    {
        NHL_deviceTable[nextEntry].shortAddr = shortAddr;
        NHL_deviceTable[nextEntry].lastMacSeqNumber = 0xffff;
        ulpsmacaddr_long_copy(&(NHL_deviceTable[nextEntry].extAddr), pLongAddr);
#ifdef FEATURE_MAC_SECURITY
        macSecurityDeviceAdd(NHL_deviceTable[nextEntry].shortAddr);
#endif
        nextEntry++;
        return ULPSMAC_TRUE;
    }
    else
    {
        return ULPSMAC_FALSE;
    }
}
#endif
