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
 *  ======== NVSCC26XX.c ========
 */
#ifdef DeviceFamily_CC26X2
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>

#include <ti/drivers/NVS.h>
#include <ti/drivers/nvs/NVSCC26XX.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/flash.h)
#include DeviceFamily_constructPath(driverlib/vims.h)
#include DeviceFamily_constructPath(driverlib/aon_batmon.h)

/* max number of bytes to write at a time to minimize interrupt latency */
#define MAX_WRITE_INCREMENT 8

/* Max number of writes per row of memory */
#define MAX_WRITES_PER_FLASH_ROW    (83)

static int_fast16_t checkEraseRange(NVS_Handle handle, size_t offset,
                        size_t size);
static int_fast16_t doErase(NVS_Handle handle, size_t offset, size_t size);
static uint8_t disableFlashCache(void);
static void restoreFlashCache(uint8_t mode);

extern NVS_Config NVS_config[];
extern const uint8_t NVS_count;

/* NVS function table for NVSCC26XX implementation */
const NVS_FxnTable NVSCC26XX_fxnTable = {
    NVSCC26XX_close,
    NVSCC26XX_control,
    NVSCC26XX_erase,
    NVSCC26XX_getAttrs,
    NVSCC26XX_init,
    NVSCC26XX_lock,
    NVSCC26XX_open,
    NVSCC26XX_read,
    NVSCC26XX_unlock,
    NVSCC26XX_write
};

/*
 *  Semaphore to synchronize access to flash region.
 */
static SemaphoreP_Handle  writeSem;

static size_t sectorSize;         /* fetched during init() */
static size_t sectorBaseMask;     /* for efficient argument checking */

/*
 *  ======== NVSCC26XX_close ========
 */
void NVSCC26XX_close(NVS_Handle handle)
{
    NVSCC26XX_Object *object;

    object = handle->object;
    object->opened = false;
}

/*
 *  ======== NVSCC26XX_control ========
 */
int_fast16_t NVSCC26XX_control(NVS_Handle handle, uint_fast16_t cmd,
                 uintptr_t arg)
{
    return (NVS_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== NVSCC26XX_erase ========
 */
int_fast16_t NVSCC26XX_erase(NVS_Handle handle, size_t offset, size_t size)
{
    int_fast16_t status;

    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    status = doErase(handle, offset, size);

    SemaphoreP_post(writeSem);

    return (status);
}

/*
 *  ======== NVSCC26XX_getAttrs ========
 */
void NVSCC26XX_getAttrs(NVS_Handle handle, NVS_Attrs *attrs)
{
    NVSCC26XX_HWAttrs const *hwAttrs;

    hwAttrs = handle->hwAttrs;

    /* FlashSectorSizeGet() returns the size of a flash sector in bytes. */
    attrs->regionBase  = hwAttrs->regionBase;
    attrs->regionSize  = hwAttrs->regionSize;
    attrs->sectorSize  = FlashSectorSizeGet();
}

/*
 *  ======== NVSCC26XX_init ========
 */
void NVSCC26XX_init()
{
    unsigned int key;
    SemaphoreP_Handle sem;

    /* initialize energy saving variables */
    sectorSize = FlashSectorSizeGet();
    sectorBaseMask = ~(sectorSize - 1);

    /* speculatively create a binary semaphore for thread safety */
    sem = SemaphoreP_createBinary(1);
    /* sem == NULL will be detected in 'open' */

    key = HwiP_disable();

    if (writeSem == NULL) {
        /* use the binary sem created above */
        writeSem = sem;
        HwiP_restore(key);
    }
    else {
        /* init already called */
        HwiP_restore(key);
        /* delete unused Semaphore */
        if (sem) SemaphoreP_delete(sem);
    }
}

/*
 *  ======== NVSCC26XX_lock =======
 */
int_fast16_t NVSCC26XX_lock(NVS_Handle handle, uint32_t timeout)
{
    if (SemaphoreP_pend(writeSem, timeout) != SemaphoreP_OK) {
        return (NVS_STATUS_TIMEOUT);
    }
    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== NVSCC26XX_open =======
 */
NVS_Handle NVSCC26XX_open(uint_least8_t index, NVS_Params *params)
{
    NVSCC26XX_Object *object;
    NVSCC26XX_HWAttrs const *hwAttrs;
    NVS_Handle handle;

    /* Confirm that 'init' has successfully completed */
    if (writeSem == NULL) {
        NVSCC26XX_init();
        if (writeSem == NULL) {
            return (NULL);
        }
    }

    /* verify NVS region index */
    if (index >= NVS_count) {
        return (NULL);
    }

    handle = &NVS_config[index];
    object = NVS_config[index].object;
    hwAttrs = NVS_config[index].hwAttrs;

    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    if (object->opened == true) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }

    /* The regionBase must be aligned on a flash page boundary */
    if ((size_t)(hwAttrs->regionBase) & (sectorSize - 1)) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }

    /* The region cannot be smaller than a sector size */
    if (hwAttrs->regionSize < sectorSize) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }

    /* The region size must be a multiple of sector size */
    if (hwAttrs->regionSize != (hwAttrs->regionSize & sectorBaseMask)) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }

#if defined(NVSCC26XX_INSTRUMENTED)
    /* Check scoreboard parameters are defined & correct */
    if (hwAttrs->scoreboard &&
        (hwAttrs->flashPageSize == 0 || hwAttrs->scoreboardSize <
        (hwAttrs->regionSize / hwAttrs->flashPageSize))) {
        SemaphoreP_post(writeSem);
        return (NULL);
    }
#endif

    object->opened = true;

    SemaphoreP_post(writeSem);

    return (handle);
}

/*
 *  ======== NVSCC26XX_read =======
 */
int_fast16_t NVSCC26XX_read(NVS_Handle handle, size_t offset, void *buffer,
                 size_t bufferSize)
{
    NVSCC26XX_HWAttrs const *hwAttrs;

    hwAttrs = handle->hwAttrs;

    /* Validate offset and bufferSize */
    if (offset + bufferSize > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);
    }

    /*
     *  Get exclusive access to the region.  We don't want someone
     *  else to erase the region while we are reading it.
     */
    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    memcpy(buffer, (char *)(hwAttrs->regionBase) + offset, bufferSize);

    SemaphoreP_post(writeSem);

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== NVSCC26XX_unlock =======
 */
void NVSCC26XX_unlock(NVS_Handle handle)
{
    SemaphoreP_post(writeSem);
}

/*
 *  ======== NVSCC26XX_write =======
 */
int_fast16_t NVSCC26XX_write(NVS_Handle handle, size_t offset, void *buffer,
                 size_t bufferSize, uint_fast16_t flags)
{
    NVSCC26XX_HWAttrs const *hwAttrs;
    unsigned int key;
    unsigned int size;
    uint32_t status = 0;
    int i;
    uint8_t mode;
    uint8_t *srcBuf, *dstBuf;
    size_t writeIncrement;
    int retval = NVS_STATUS_SUCCESS;

#if defined(NVSCC26XX_INSTRUMENTED)
    size_t   bytesWritten;
    uint32_t sbIndex, writeOffset;
#endif

    hwAttrs = handle->hwAttrs;

    /* Validate offset and bufferSize */
    if (offset + bufferSize > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);
    }

    /* Get exclusive access to the Flash region */
    SemaphoreP_pend(writeSem, SemaphoreP_WAIT_FOREVER);

    /* If erase is set, erase destination sector(s) first */
    if (flags & NVS_WRITE_ERASE) {
        size = bufferSize & sectorBaseMask;
        if (bufferSize & (~sectorBaseMask)) {
            size += sectorSize;
        }

        retval = doErase(handle, offset & sectorBaseMask, size);
        if (retval != NVS_STATUS_SUCCESS) {
            SemaphoreP_post(writeSem);
            return (retval);
        }
    }
    else if (flags & NVS_WRITE_PRE_VERIFY) {
        /*
         *  If pre-verify, each destination byte must be able to be changed to the
         *  source byte (1s to 0s, not 0s to 1s).
         *  this is satisfied by the following test:
         *     src == (src & dst)
         */
        dstBuf = (uint8_t *)((uint32_t)(hwAttrs->regionBase) + offset);
        srcBuf = buffer;
        for (i = 0; i < bufferSize; i++) {
            if (srcBuf[i] != (srcBuf[i] & dstBuf[i])) {
                SemaphoreP_post(writeSem);
                return (NVS_STATUS_INV_WRITE);
            }
        }
    }

    srcBuf = buffer;
    size   = bufferSize;
    dstBuf = (uint8_t *)((uint32_t)(hwAttrs->regionBase) + offset);

    mode = disableFlashCache();

    while (size) {
        if (size > MAX_WRITE_INCREMENT) {
            writeIncrement = MAX_WRITE_INCREMENT;
        }
        else {
            writeIncrement = size;
        }
        key = HwiP_disable();
        status = FlashProgram((uint8_t*)srcBuf, (uint32_t)dstBuf,
                     writeIncrement);
        HwiP_restore(key);

        if (status != 0) {
            break;
        }
        else {
            size -= writeIncrement;
            srcBuf += writeIncrement;
            dstBuf += writeIncrement;
        }
    }

    restoreFlashCache(mode);

#if defined(NVSCC26XX_INSTRUMENTED)
    if (hwAttrs->scoreboard) {
        /*
         * Write counts are updated even if an error occurs & not all data was
         * written.
         */
        bytesWritten = bufferSize - size;
        writeOffset = offset;

        while (bytesWritten) {
            if (bytesWritten > MAX_WRITE_INCREMENT) {
                writeIncrement = MAX_WRITE_INCREMENT;
            }
            else {
                writeIncrement = bytesWritten;
            }

            sbIndex = writeOffset / hwAttrs->flashPageSize;
            hwAttrs->scoreboard[sbIndex]++;

            /* Spin forever if the write limit is exceeded */
            if (hwAttrs->scoreboard[sbIndex] > MAX_WRITES_PER_FLASH_ROW) {
                while (1);
            }

            writeOffset += writeIncrement;
            bytesWritten -= writeIncrement;
        }
    }
#endif

    if (status != 0) {
        retval = NVS_STATUS_ERROR;
    }
    else if (flags & NVS_WRITE_POST_VERIFY) {
        /*
         *  Note: This validates the entire region even on erase mode.
         */
        dstBuf = (uint8_t *)((uint32_t)(hwAttrs->regionBase) + offset);
        srcBuf = buffer;

        for (i = 0; i < bufferSize; i++) {
            if (srcBuf[i] != dstBuf[i]) {
                retval = NVS_STATUS_ERROR;
                break;
            }
        }
    }

    SemaphoreP_post(writeSem);

    return (retval);
}

/*
 *  ======== checkEraseRange ========
 */
static int_fast16_t checkEraseRange(NVS_Handle handle, size_t offset,
                        size_t size)
{
    NVSCC26XX_HWAttrs const *hwAttrs = handle->hwAttrs;

    if (offset != (offset & sectorBaseMask)) {
        return (NVS_STATUS_INV_ALIGNMENT);    /* poorly aligned start */
                                              /* address */
    }

    if (offset >= hwAttrs->regionSize) {
        return (NVS_STATUS_INV_OFFSET);   /* offset is past end of region */
    }

    if (offset + size > hwAttrs->regionSize) {
        return (NVS_STATUS_INV_SIZE);     /* size is too big */
    }

    if (size != (size & sectorBaseMask)) {
        return (NVS_STATUS_INV_SIZE);     /* size is not a multiple of */
                                          /* sector size */
    }

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== doErase ========
 */
static int_fast16_t doErase(NVS_Handle handle, size_t offset, size_t size)
{
    NVSCC26XX_HWAttrs const *hwAttrs = handle->hwAttrs;
    unsigned int key;
    uint8_t mode;
    uint32_t status = 0;
    uint32_t sectorBase;
    int_fast16_t rangeStatus;

#if defined(NVSCC26XX_INSTRUMENTED)
    uint32_t i;
    uint32_t sbIndex;
#endif

    /* sanity test the erase args */
    rangeStatus = checkEraseRange(handle, offset, size);

    if (rangeStatus != NVS_STATUS_SUCCESS) {
        return (rangeStatus);
    }

    sectorBase = (uint32_t)hwAttrs->regionBase + offset;

    mode = disableFlashCache();

    while (size) {
        key = HwiP_disable();
        status = FlashSectorErase(sectorBase);
        HwiP_restore(key);

        if (status != FAPI_STATUS_SUCCESS) {
            break;
        }

#if defined(NVSCC26XX_INSTRUMENTED)
        if (hwAttrs->scoreboard) {
            /*
             * Sector successfully erased; now we must clear scoreboard write
             * counts for all pages in the sector.
             */
            sbIndex = (sectorBase - (uint32_t) hwAttrs->regionBase) /
                hwAttrs->flashPageSize;

            for (i = 0; i < (sectorSize / hwAttrs->flashPageSize); i++) {
                hwAttrs->scoreboard[sbIndex + i] = 0;
            }
        }
#endif

        sectorBase += sectorSize;
        size -= sectorSize;
    }

    restoreFlashCache(mode);

    if (status != FAPI_STATUS_SUCCESS) {
        return (NVS_STATUS_ERROR);
    }

    return (NVS_STATUS_SUCCESS);
}

/*
 *  ======== disableFlashCache ========
 *  When updating the Flash, the VIMS (Vesatile Instruction Memory System)
 *  mode must be set to GPRAM or OFF, before programming, and both VIMS
 *  flash line buffers must be set to disabled.
 */
static uint8_t disableFlashCache(void)
{
    uint8_t mode = VIMSModeGet(VIMS_BASE);

    VIMSLineBufDisable(VIMS_BASE);

    if (mode != VIMS_MODE_DISABLED) {
        VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
        while (VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED);
    }

    return (mode);
}

/*
 *  ======== restoreFlashCache ========
 */
static void restoreFlashCache(uint8_t mode)
{
    if (mode != VIMS_MODE_DISABLED) {
        VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
    }

    VIMSLineBufEnable(VIMS_BASE);
}
#else  //DeviceFamily_CC26X2
#include <stdbool.h>
#include <stdint.h>
#include <string.h>  /* for string support */
#include <stdlib.h>

#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Log.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
// TODO: Why not hal/Hwi.h?
#include <ti/sysbios/family/arm/m3/Hwi.h>

#include <driverlib/flash.h>
#include <driverlib/vims.h>

#include <ti/drivers/NVS.h>
#include <ti/drivers/nvs/NVSCC26XX.h>


/* NVSCC26XX functions */
void        NVSCC26XX_close(NVS_Handle handle);
int         NVSCC26XX_control(NVS_Handle handle, unsigned int cmd,
                                uintptr_t arg);
void        NVSCC26XX_exit(NVS_Handle handle);
int         NVSCC26XX_getAttrs(NVS_Handle handle, NVS_Attrs *attrs);
void        NVSCC26XX_init(NVS_Handle handle);
NVS_Handle  NVSCC26XX_open(NVS_Handle handle, NVS_Params *params);
int         NVSCC26XX_read(NVS_Handle handle, size_t offset, void *buffer,
                             size_t bufferSize);
int         NVSCC26XX_write(NVS_Handle handle, size_t offset, void *buffer,
                              size_t bufferSize, unsigned int flags);

/* NVS function table for NVSCC26XX implementation */
const NVS_FxnTable NVSCC26XX_fxnTable = {
    NVSCC26XX_close,
    NVSCC26XX_control,
    NVSCC26XX_exit,
    NVSCC26XX_getAttrs,
    NVSCC26XX_init,
    NVSCC26XX_open,
    NVSCC26XX_read,
    NVSCC26XX_write
};

/*
 *  Semaphore to synchronize access to flash block.
 */
static Semaphore_Struct  writeSem;

/*
 *  ======== disableFlashCache ========
 *  When updating the Flash, the VIMS (Vesatile Instruction Memory System)
 *  mode must be set to GPRAM or OFF, before programming, and both VIMS
 *  flash line buffers must be set to disabled.
 */
static uint8_t disableFlashCache(void)
{
    uint8_t mode = VIMSModeGet(VIMS_BASE);

    if (mode != VIMS_MODE_DISABLED) {
        VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
        while (VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED);
    }

    return (mode);
}

/*
 *  ======== enableFlashCache ========
 */
static void enableFlashCache(uint8_t mode)
{
    if (mode != VIMS_MODE_DISABLED) {
        VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
    }
}

/*
 *  ======== NVSCC26XX_close ========
 */
void NVSCC26XX_close(NVS_Handle handle)
{
}

/*
 *  ======== NVSCC26XX_control ========
 */
int NVSCC26XX_control(NVS_Handle handle, unsigned int cmd, uintptr_t arg)
{
    NVSCC26XX_HWAttrs *hwAttrs = (NVSCC26XX_HWAttrs *)(handle->hwAttrs);
    NVSCC26XX_CmdSetCopyBlockArgs *cmdArgs = (NVSCC26XX_CmdSetCopyBlockArgs *)arg;
    uint8_t *copyBlock = (uint8_t *)(cmdArgs->copyBlock);

    if (cmd == NVSCC26XX_CMD_SET_COPYBLOCK) {
        if ((copyBlock == NULL) || ((uint32_t)copyBlock & 0x3)) {
            return (NVSCC26XX_STATUS_ECOPYBLOCK);
        }

        hwAttrs->copyBlock = cmdArgs->copyBlock;
        hwAttrs->isRam = cmdArgs->isRam;

        if (!hwAttrs->isRam) {
            /* If copy block is in flash, check that it is page aligned */
            if ((uint32_t)(hwAttrs->copyBlock) & (FlashSectorSizeGet() - 1)) {

                Log_warning1("NVS:(%p) Copy block not aligned on page boundary.",
                        (IArg)(hwAttrs->block));
                return (NVSCC26XX_STATUS_ECOPYBLOCK);
            }
        }

        return (NVS_STATUS_SUCCESS);
    }

    return (NVS_STATUS_UNDEFINEDCMD);
}

/*
 *  ======== NVSCC26XX_exit ========
 */
void NVSCC26XX_exit(NVS_Handle handle)
{
}

/*
 *  ======== NVSCC26XX_getAttrs ========
 */
int NVSCC26XX_getAttrs(NVS_Handle handle, NVS_Attrs *attrs)
{
    NVSCC26XX_HWAttrs const  *hwAttrs = handle->hwAttrs;

    /* FlashSectorSizeGet() returns the size of a flash sector in bytes. */
    attrs->pageSize   = FlashSectorSizeGet();
    attrs->blockSize  = hwAttrs->blockSize;

    return (NVS_SOK);
}

/*
 *  ======== NVSCC26XX_init ========
 */
void NVSCC26XX_init(NVS_Handle handle)
{
    Semaphore_construct(&writeSem, 1, NULL);
}

/*
 *  ======== NVSCC26XX_open =======
 */
NVS_Handle NVSCC26XX_open(NVS_Handle handle, NVS_Params *params)
{
    NVSCC26XX_Object         *object = handle->object;
    NVSCC26XX_HWAttrs const  *hwAttrs = handle->hwAttrs;
    int                       status = NVS_SOK;

    Semaphore_pend(Semaphore_handle(&writeSem), BIOS_WAIT_FOREVER);

    if (object->opened == true) {
        Semaphore_post(Semaphore_handle(&writeSem));

        Log_warning1("NVS:(%p) already in use.", (IArg)(hwAttrs->block));
        return (NULL);
    }

    /* The block must be aligned on a flaah page boundary */
    if ((uint32_t)(hwAttrs->block) & (FlashSectorSizeGet() - 1)) {
        Semaphore_post(Semaphore_handle(&writeSem));

        Log_warning1("NVS:(%p) block not aligned on flash page boundary.",
                (IArg)(hwAttrs->block));
        return (NULL);
    }

    /* The block cannot be larger than a flash page */
    if ((uint32_t)(hwAttrs->blockSize) > FlashSectorSizeGet()) {
        Semaphore_post(Semaphore_handle(&writeSem));

        Log_warning1("NVS:(%p) blockSize must not be greater than page size.",
                (IArg)(hwAttrs->block));
        return (NULL);
    }

    /* Flash copy block must be aligned on a flaah page boundary */
    if (hwAttrs->copyBlock && !(hwAttrs->isRam) &&
            ((uint32_t)(hwAttrs->copyBlock) & (FlashSectorSizeGet() - 1))) {
        Semaphore_post(Semaphore_handle(&writeSem));

        Log_warning1("NVS:(%p) Flash copyBlock not page boundary aligned.",
                (IArg)(hwAttrs->block));
        return (NULL);
    }

    /* Ram copy block must be 4-byte aligned */
    if ((uint32_t)(hwAttrs->copyBlock) & 0x3) {
        Semaphore_post(Semaphore_handle(&writeSem));

        Log_warning1("NVS:(%p) copyBlock not 4-byte aligned.",
                (IArg)(hwAttrs->block));
        return (NULL);
    }

    if (params->eraseOnOpen == true) {
        status = NVSCC26XX_write(handle, 0, NULL, 0, 0);

        if  (status != NVS_SOK) {
            Log_warning1("NVS:(%p) copyBlock not 4-byte aligned.",
                    (IArg)(hwAttrs->block));
        }
    }

    if (status == NVS_SOK) {
        object->opened = true;
    }

    Semaphore_post(Semaphore_handle(&writeSem));

    return (handle);
}

/*
 *  ======== NVSCC26XX_read =======
 */
int NVSCC26XX_read(NVS_Handle handle, size_t offset, void *buffer,
        size_t bufferSize)
{
    NVSCC26XX_HWAttrs const  *hwAttrs = handle->hwAttrs;
    int retval = NVS_SOK;

    /* Validate offset and bufferSize */
    if (offset + bufferSize > hwAttrs->blockSize) {
        return (NVS_EOFFSET);
    }

    /*
     *  Get exclusive access to the block.  We don't want someone
     *  else to erase the block while we are reading it.
     */
    Semaphore_pend(Semaphore_handle(&writeSem), BIOS_WAIT_FOREVER);

    memcpy(buffer, (Char *)(hwAttrs->block) + offset, bufferSize);

    Semaphore_post(Semaphore_handle(&writeSem));

    return (retval);
}

/*
 *  ======== NVSCC26XX_write =======
 */
int NVSCC26XX_write(NVS_Handle handle, size_t offset, void *buffer,
                      size_t bufferSize, unsigned int flags)
{
    NVSCC26XX_HWAttrs const  *hwAttrs = handle->hwAttrs;
    unsigned int size;
    uint32_t status = 0;
    int i;
    uint8_t mode;
    uint8_t *srcBuf, *dstBuf;
    int retval = NVS_SOK;

    /* Buffer to copy into flash must be 4-byte aligned */
    if (((uint32_t)buffer & 0x3) || (bufferSize & 0x3)) {
        Log_warning1("NVS:(%p) Buffer and buffer size must be 4-byte aligned.",
                (IArg)(hwAttrs->block));
        return (NVS_EALIGN);
    }

    /* Check if offset is not a multiple of 4 */
    if (offset & 0x3) {
        Log_warning1("NVS:(%p) offset size must be 4-byte aligned.",
                (IArg)(hwAttrs->block));
        return (NVS_EALIGN);
    }

    /* Validate offset and bufferSize */
    if (offset + bufferSize > hwAttrs->blockSize) {
        return (NVS_EOFFSET);
    }

    /* Get exclusive access to the Flash block */
    Semaphore_pend(Semaphore_handle(&writeSem), BIOS_WAIT_FOREVER);

    /*
     *  A NULL buffer signifies that we are just erasing the
     *  flash block.
     */
    if (buffer == NULL) {
        mode = disableFlashCache();
        status = FlashSectorErase((uint32_t)hwAttrs->block);
        enableFlashCache(mode);

        Semaphore_post(Semaphore_handle(&writeSem));

        if (status != FAPI_STATUS_SUCCESS) {
            Log_warning1("NVS:(%p) FlashSectorErase() failed.",
                    (IArg)(hwAttrs->block));
            return (NVS_EFAIL);
        }

        return (NVS_SOK);
    }

    /*
     *  If exclusive write, check that the region has not been
     *  written to since the last erase.  (Erasing leaves flash
     *  set to 0xFF)
     */
    if (flags & NVS_WRITE_EXCLUSIVE) {
        dstBuf = (uint8_t *)((uint32_t)(hwAttrs->block) + offset);
        for (i = 0; i < bufferSize; i++) {
            if (dstBuf[i] != 0xFF) {
                Semaphore_post(Semaphore_handle(&writeSem));
                return (NVS_EALREADYWRITTEN);
            }
        }
    }

    /* If erase is set, determine whether to use RAM or the flash copyBlock */
    if (flags & NVS_WRITE_ERASE) {

        /* Must have copy block for erase */
        if (hwAttrs->copyBlock == NULL) {
            Semaphore_post(Semaphore_handle(&writeSem));
            Log_warning1("NVS:(%p) copyBlock must be non-NULL.",
                    (IArg)(hwAttrs->block));
            return (NVS_ECOPYBLOCK);
        }

        srcBuf = (uint8_t *)(hwAttrs->copyBlock);

        if (hwAttrs->isRam) {
            /* Copy flash contents up to the offset into temporary buffer */
            memcpy(srcBuf, hwAttrs->block, offset);

            /* Update the temporary buffer with the data to be written */
            memcpy((void *)((uint32_t)srcBuf + offset), buffer, bufferSize);

            /* Copy remaining flash contents into temporary buffer */
            memcpy(srcBuf + offset + bufferSize,
                    (void *)((uint32_t)hwAttrs->block + offset + bufferSize),
                    hwAttrs->blockSize - bufferSize - offset);
        }
        else {
            /* Erase the flash copy block */
            mode = disableFlashCache();
            status = FlashSectorErase((uint32_t)(hwAttrs->copyBlock));
            if (status != 0) {
                enableFlashCache(mode);
                Semaphore_post(Semaphore_handle(&writeSem));

                Log_warning1("NVS:(%p) FlashSectorErase() failed.",
                        (IArg)(hwAttrs->block));
                return (NVS_EFAIL);
            }

            /*  Copy up to offset */
            status = FlashProgram((uint8_t *)(hwAttrs->block),    /* src  */
                                  (uint32_t)hwAttrs->copyBlock,   /* dst  */
                                  (uint32_t)offset);              /* size */

            /*  Copy buffer */
            status |= FlashProgram((uint8_t *)buffer,
                                  (uint32_t)(hwAttrs->copyBlock) + offset,
                                  (uint32_t)bufferSize);

            /*  Copy after offset + bufferSize */
            status |= FlashProgram(
                (uint8_t *)((uint32_t)hwAttrs->block + offset + bufferSize),
                (uint32_t)(hwAttrs->copyBlock) + offset + bufferSize,
                hwAttrs->blockSize - bufferSize - offset);

            enableFlashCache(mode);

            if (status != 0) {
                Semaphore_post(Semaphore_handle(&writeSem));

                Log_warning1("NVS:(%p) FlashProgram() failed.",
                        (IArg)(hwAttrs->block));
                return (NVS_EFAIL);
            }
        }

        mode = disableFlashCache();
        status = FlashSectorErase((uint32_t)hwAttrs->block);
        enableFlashCache(mode);

        if (status != 0) {
            Semaphore_post(Semaphore_handle(&writeSem));
            Log_warning1("NVS:(%p) FlashSectorErase() failed.",
                    (IArg)(hwAttrs->block));
            return (NVS_EFAIL);
        }
        size = hwAttrs->blockSize;
        dstBuf = hwAttrs->block;
    }
    else {
        srcBuf = buffer;
        size   = bufferSize;
        dstBuf = (uint8_t *)((uint32_t)(hwAttrs->block) + offset);
    }

    mode = disableFlashCache();
    status = FlashProgram((uint8_t*)srcBuf, (uint32_t)dstBuf, size);
    enableFlashCache(mode);

    if (status != 0) {
        retval = NVS_EFAIL;
    }
    else if ((flags & NVS_WRITE_VALIDATE)) {
        /*
         *  Note: This validates the entire block even on erase mode.
         */
        for (i = 0; i < size; i++) {
            if (srcBuf[i] != dstBuf[i]) {
                retval = NVS_EFAIL;
                break;
            }
        }
    }

    Semaphore_post(Semaphore_handle(&writeSem));

    return (retval);
}
#endif  //DeviceFamily_CC26X2
