/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/dpl/DebugP.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/AESCCM.h>
#include <ti/drivers/aesccm/AESCCMCC26XX.h>
#include <ti/drivers/cryptoutils/sharedresources/CryptoResourceCC26XX.h>
#include <ti/drivers/cryptoutils/cryptokey/CryptoKey.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_ints.h)
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(inc/hw_crypto.h)
#include DeviceFamily_constructPath(driverlib/aes.h)
#include DeviceFamily_constructPath(driverlib/cpu.h)
#include DeviceFamily_constructPath(driverlib/interrupt.h)
#include DeviceFamily_constructPath(driverlib/sys_ctrl.h)
#include DeviceFamily_constructPath(driverlib/smph.h)


/* Forward declarations */
static void AESCCM_hwiFxn (uintptr_t arg0);
static void AESCCM_swiFxn (uintptr_t arg0, uintptr_t arg1);
static void AESCCM_internalCallbackFxn (AESCCM_Handle handle,
                                        int_fast16_t returnValue,
                                        AESCCM_Operation *operation,
                                        AESCCM_OperationType operationType);
static int_fast16_t AESCCM_waitForAccess(AESCCM_Handle handle);
static int_fast16_t AESCCM_startOperation(AESCCM_Handle handle,
                                          AESCCM_Operation *operation,
                                          AESCCM_OperationType operationType);
static void AESCCM_waitForResult(AESCCM_Handle handle);

/* Extern globals */
extern const AESCCM_Config AESCCM_config[];
extern const uint_least8_t AESCCM_count;

/* Static globals */
static bool isInitialized = false;

/*
 *  ======== AESCCM_swiFxn ========
 */
static void AESCCM_swiFxn (uintptr_t arg0, uintptr_t arg1) {
    AESCCMCC26XX_Object *object = ((AESCCM_Handle)arg0)->object;

    /* We need to copy / verify the MAC now so that it is not clobbered when we
     * release the CryptoResourceCC26XX_accessSemaphore semaphore.
     */
    if (object->operationType == AESCCM_OPERATION_TYPE_ENCRYPT) {
        /* If we are encrypting and authenticating a message, we only want to
         * copy the MAC to the target buffer
         */
        AESReadTag(object->operation->mac, object->operation->macLength);
    }
    else {
        /* If we are decrypting and verifying a message, we must now verify that the provided
         * MAC matches the one calculated in the decryption operation.
         */
        uint32_t verifyResult = AESVerifyTag(object->operation->mac, object->operation->macLength);

        object->macVerifyResult = (verifyResult == AES_SUCCESS) ? AESCCM_STATUS_SUCCESS : AESCCM_STATUS_MAC_INVALID;
    }

    /* Since plaintext keys use two reserved (by convention) slots in the keystore,
     * the slots must be invalidated to prevent its re-use without reloading
     * the key material again.
     */
    AESInvalidateKey(AES_KEY_AREA_6);
    AESInvalidateKey(AES_KEY_AREA_7);

    /*  This powers down all sub-modules of the crypto module until needed.
     *  It does not power down the crypto module at PRCM level and provides small
     *  power savings.
     */
    AESSelectAlgorithm(0x00);

    /*  Grant access for other threads to use the crypto module.
     *  The semaphore must be posted before the callbackFxn to allow the chaining
     *  of operations.
     */
    SemaphoreP_post(&CryptoResourceCC26XX_accessSemaphore);

    Power_releaseConstraint(PowerCC26XX_SB_DISALLOW);

    object->callbackFxn((AESCCM_Handle)arg0,
                        object->operationType == AESCCM_OPERATION_TYPE_ENCRYPT ? AESCCM_STATUS_SUCCESS : object->macVerifyResult,
                        object->operation,
                        object->operationType);
}

/*
 *  ======== AESCCM_hwiFxn ========
 */
static void AESCCM_hwiFxn (uintptr_t arg0) {
    AESCCMCC26XX_Object *object = ((AESCCM_Handle)arg0)->object;

    AESIntClear(AES_RESULT_RDY);

    SwiP_post(&(object->callbackSwi));
}

/*
 *  ======== AESCCM_internalCallbackFxn ========
 */
static void AESCCM_internalCallbackFxn (AESCCM_Handle handle,
                                        int_fast16_t returnValue,
                                        AESCCM_Operation *operation,
                                        AESCCM_OperationType operationType) {
    AESCCMCC26XX_Object *object = handle->object;

    /* This function is only ever registered when in AESCCM_RETURN_BEHAVIOR_BLOCKING
     * or AESCCM_RETURN_BEHAVIOR_POLLING.
     */
    if (object->returnBehavior == AESCCM_RETURN_BEHAVIOR_BLOCKING) {
        SemaphoreP_post(&CryptoResourceCC26XX_operationSemaphore);
    }
    else {
        CryptoResourceCC26XX_pollingFlag = 1;
    }
}

/*
 *  ======== AESCCM_init ========
 */
void AESCCM_init(void) {
    uint_least8_t i;
    uint_fast8_t key;

    key = HwiP_disable();

    if (!isInitialized) {
        /* Call each instances' driver init function */
        for (i = 0; i < AESCCM_count; i++) {
            AESCCM_Handle handle = (AESCCM_Handle)&(AESCCM_config[i]);
            AESCCMCC26XX_Object *object = (AESCCMCC26XX_Object *)handle->object;
            object->isOpen = false;
        }

        isInitialized = true;
    }

    HwiP_restore(key);
}

/*
 *  ======== AESCCM_open ========
 */
AESCCM_Handle AESCCM_open(uint_least8_t index, AESCCM_Params *params) {
    SwiP_Params                 swiParams;
    AESCCM_Handle               handle;
    AESCCMCC26XX_Object        *object;
    AESCCMCC26XX_HWAttrs const *hwAttrs;
    uint_fast8_t                key;

    handle = (AESCCM_Handle)&(AESCCM_config[index]);
    object = handle->object;
    hwAttrs = handle->hwAttrs;

    DebugP_assert(index >= AESCCM_count);

    key = HwiP_disable();

    if (!isInitialized ||  object->isOpen) {
        HwiP_restore(key);
        return NULL;
    }

    object->isOpen = true;

    HwiP_restore(key);

    /* If params are NULL, use defaults */
    if (params == NULL) {
        params = (AESCCM_Params *)&AESCCM_defaultParams;
    }

    /* This is currently not supported. Eventually it will make the TRNG generate the nonce */
    DebugP_assert(!params->nonceInternallyGenerated);
    DebugP_assert(params->returnBehavior == AESCCM_RETURN_BEHAVIOR_CALLBACK ? params->callbackFxn : true);

    object->returnBehavior = params->returnBehavior;
    object->callbackFxn = params->returnBehavior == AESCCM_RETURN_BEHAVIOR_CALLBACK ? params->callbackFxn : AESCCM_internalCallbackFxn;
    object->semaphoreTimeout = params->timeout;

    /* Create Swi object for this AESCCM peripheral */
    SwiP_Params_init(&swiParams);
    swiParams.arg0 = (uintptr_t)handle;
    swiParams.priority = hwAttrs->swiPriority;
    SwiP_construct(&(object->callbackSwi), AESCCM_swiFxn, &swiParams);

    CryptoResourceCC26XX_constructRTOSObjects();

    /* Set power dependency - i.e. power up and enable clock for Crypto (CryptoResourceCC26XX) module. */
    Power_setDependency(PowerCC26XX_PERIPH_CRYPTO);

    return handle;
}

/*
 *  ======== AESCCM_close ========
 */
void AESCCM_close(AESCCM_Handle handle) {
    AESCCMCC26XX_Object         *object;

    DebugP_assert(handle);

    /* Get the pointer to the object and hwAttrs */
    object = handle->object;

    CryptoResourceCC26XX_destructRTOSObjects();

    /* Destroy the Swi */
    SwiP_destruct(&(object->callbackSwi));

    /* Release power dependency on Crypto Module. */
    Power_releaseDependency(PowerCC26XX_PERIPH_CRYPTO);

    /* Mark the module as available */
    object->isOpen = false;
}

/*
 *  ======== AESCCM_startOperation ========
 */
static int_fast16_t AESCCM_startOperation(AESCCM_Handle handle,
                                          AESCCM_Operation *operation,
                                          AESCCM_OperationType operationType) {
    AESCCMCC26XX_Object *object = handle->object;
    AESCCMCC26XX_HWAttrs const *hwAttrs = handle->hwAttrs;

    /* Only plaintext CryptoKeys are supported for now */
    uint16_t keyLength = operation->key->u.plaintext.keyLength;
    uint8_t *keyingMaterial = operation->key->u.plaintext.keyMaterial;

    DebugP_assert(handle);
    DebugP_assert(key);
    DebugP_assert(nonce && (nonceLength >= 7 && nonceLength <= 13));
    DebugP_assert((aad && aadLength) || (input && inputLength));
    DebugP_assert(mac && (macLength <= 16));
    DebugP_assert(key->encoding == CryptoKey_PLAINTEXT);

    /* Try and obtain access to the crypto module */
    if (AESCCM_waitForAccess(handle) != SemaphoreP_OK) {
        return AESCCM_STATUS_RESOURCE_UNAVAILABLE;
    }

    object->operationType = operationType;
    object->operation = operation;

    Power_setConstraint(PowerCC26XX_SB_DISALLOW);

    /* We need to set the HWI function and priority since the same physical interrupt is shared by multiple
     * drivers and they all need to coexist. Whenever a driver starts an operation, it
     * registers its HWI callback with the OS.
     */
    HwiP_setFunc(&CryptoResourceCC26XX_hwi, AESCCM_hwiFxn, (uintptr_t)handle);
    HwiP_setPriority(INT_CRYPTO_RESULT_AVAIL_IRQ, hwAttrs->intPriority);

    CryptoResourceCC26XX_pollingFlag = 0;

    /* Load the key from RAM or flash into the key store at a hardcoded and reserved location */
    if (AESWriteToKeyStore(keyingMaterial, keyLength, AES_KEY_AREA_6) != AES_SUCCESS) {
        return AESCCM_STATUS_ERROR;
    }

    /* Power the AES sub-module of the crypto module */
    AESSelectAlgorithm(AES_ALGSEL_AES);

    /* Load the key from the key store into the internal register banks of the AES sub-module */
    if (AESReadFromKeyStore(AES_KEY_AREA_6) != AES_SUCCESS) {
        return AESCCM_STATUS_ERROR;
    }

    AESWriteCCMInitializationVector(operation->nonce, operation->nonceLength);

    AESConfigureCCMCtrl(operation->nonceLength, operation->macLength, operationType == AESCCM_OPERATION_TYPE_ENCRYPT);

    AESSetDataLength(operation->inputLength);
    AESSetAuthLength(operation->aadLength);

    if (operation->aadLength) {
        /* If aadLength were 0, AESWaitForIRQFlags() would never return as the AES_DMA_IN_DONE flag
         * would never trigger.
         */
        AESStartDMAOperation(operation->aad, operation->aadLength,  NULL, 0);
        AESWaitForIRQFlags(AES_DMA_IN_DONE | AES_DMA_BUS_ERR);
    }

    AESStartDMAOperation(operation->input, operation->inputLength, operation->output, operation->inputLength);

    AESCCM_waitForResult(handle);

    if (operationType == AESCCM_OPERATION_TYPE_ENCRYPT ||
        object->returnBehavior == AESCCM_RETURN_BEHAVIOR_CALLBACK) {
        return AESCCM_STATUS_SUCCESS;
    }
    else {
        return object->macVerifyResult == AESCCM_STATUS_SUCCESS ? AESCCM_STATUS_SUCCESS : AESCCM_STATUS_MAC_INVALID;
    }
}

/*
 *  ======== AESCCM_waitForAccess ========
 */
static int_fast16_t AESCCM_waitForAccess(AESCCM_Handle handle) {
    AESCCMCC26XX_Object *object = handle->object;
    uint32_t timeout;

    /* Set to SemaphoreP_NO_WAIT to start operations from SWI or HWI context */
    timeout = object->returnBehavior == AESCCM_RETURN_BEHAVIOR_BLOCKING ? object->semaphoreTimeout : SemaphoreP_NO_WAIT;

    return SemaphoreP_pend(&CryptoResourceCC26XX_accessSemaphore, timeout);
}

/*
 *  ======== AESCCM_waitForResult ========
 */
static void AESCCM_waitForResult(AESCCM_Handle handle){
    AESCCMCC26XX_Object *object = handle->object;

    if (object->returnBehavior == AESCCM_RETURN_BEHAVIOR_POLLING) {
        while(!CryptoResourceCC26XX_pollingFlag);
    }
    else if (object->returnBehavior == AESCCM_RETURN_BEHAVIOR_BLOCKING) {
        SemaphoreP_pend(&CryptoResourceCC26XX_operationSemaphore, 100000);//1s, instead of SemaphoreP_WAIT_FOREVER);
    }
}

/*
 *  ======== AESCCM_oneStepEncrypt ========
 */
int_fast16_t AESCCM_oneStepEncrypt(AESCCM_Handle handle, AESCCM_Operation *operationStruct) {

    return AESCCM_startOperation(handle, operationStruct, AESCCM_OPERATION_TYPE_ENCRYPT);
}

/*
 *  ======== AESCCM_oneStepDecrypt ========
 */
int_fast16_t AESCCM_oneStepDecrypt(AESCCM_Handle handle, AESCCM_Operation *operationStruct) {

    return AESCCM_startOperation(handle, operationStruct, AESCCM_OPERATION_TYPE_DECRYPT);
}
