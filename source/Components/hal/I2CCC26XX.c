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
 *  =============================== Includes ===================================
 */
/* STD header files */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
/* XDC header files */
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Log.h>
#include <xdc/runtime/Types.h>
/* RTOS driver header files */
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/pin/PINCC26XX.h>
/* SYSBIOS header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
/* driverlib header files */
#include <inc/hw_types.h>
#include <driverlib/i2c.h>
#include <driverlib/ioc.h>

/*
 *  =============================== Macros =====================================
 *
 * Specific I2C CMD MACROs that are not defined in I2C.h are defined here. Their
 * equivalent values are taken from the existing MACROs in I2C.h
 *
 */
#ifndef I2C_MASTER_CMD_BURST_RECEIVE_START_NACK
#define I2C_MASTER_CMD_BURST_RECEIVE_START_NACK     I2C_MASTER_CMD_BURST_SEND_START
#endif

#ifndef I2C_MASTER_CMD_BURST_RECEIVE_STOP
#define I2C_MASTER_CMD_BURST_RECEIVE_STOP           I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP
#endif

#ifndef I2C_MASTER_CMD_BURST_RECEIVE_CONT_NACK
#define I2C_MASTER_CMD_BURST_RECEIVE_CONT_NACK      I2C_MASTER_CMD_BURST_SEND_CONT
#endif

/*
 *  =============================== Prototypes =================================
 */
void       I2CCC26XX_init(I2C_Handle handle);
I2C_Handle I2CCC26XX_open(I2C_Handle handle, I2C_Params *params);
bool       I2CCC26XX_transfer(I2C_Handle handle, I2C_Transaction *transaction);
void       I2CCC26XX_cancel(I2C_Handle handle);
void       I2CCC26XX_close(I2C_Handle handle);
int        I2CCC26XX_control(I2C_Handle handle, unsigned int cmd, void *arg);

/*
 *  ========================== Local Prototypes ================================
 */
static int      I2CCC26XX_primeTransfer(I2C_Handle handle, I2C_Transaction *transferMessage);
static void     I2CCC26XX_blockingCallback(I2C_Handle handle, I2C_Transaction *msg, bool transferStatus);
static void     I2CCC26XX_initHw(I2C_Handle handle);
static int      I2CCC26XX_initIO(I2C_Handle handle, uintptr_t pinCfg);
static int i2cPostNotify(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg);

/*
 *  ============================== Constants ===================================
 */
/* I2C function table for I2CCC26XX implementation */
const I2C_FxnTable I2CCC26XX_fxnTable = {
    I2CCC26XX_cancel,
    I2CCC26XX_close,
    I2CCC26XX_control,
    I2CCC26XX_init,
    I2CCC26XX_open,
    I2CCC26XX_transfer
};

static const uint32_t bitRate[] = {
    false,  /*  I2C_100kHz = 0 */
    true    /*  I2C_400kHz = 1 */
};


/*
 *  ============================= Functions ====================================
 */

/*!
 *  @brief Function to cancel any in progress or queued transactions.
 *
 *  After calling the cancel function, the I2C is enabled.
 *
 *  @pre    I2CCC26XX_transfer() should have been called first.
 *          Calling context: Task
 *
 *  @param handle An I2C_Handle returned by I2C_open()
 *
 *  @note  The generic I2C API should be used when accessing the I2CCC26XX.
 *
 *  ======== I2CCC26XX_cancel ========
 */
void I2CCC26XX_cancel(I2C_Handle handle)
{
    unsigned int key;
    I2CCC26XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    I2CCC26XX_Object *object = handle->object;

    /* just return if no transfer is in progress */
    if (!object->headPtr) {
        return;
    }

    /* disable interrupts, send STOP to try to complete all transfers */
    key = Hwi_disable();
    I2CMasterIntDisable(hwAttrs->baseAddr);
    I2CMasterControl(hwAttrs->baseAddr, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);

    /* call the transfer callback for the current transfer, indicate failure */
    object->transferCallbackFxn(handle, object->currentTransaction, false);

    /* release the constraint to disallow standby */
    Power_releaseConstraint(PowerCC26XX_SB_DISALLOW);

    /* also dequeue and call the transfer callbacks for any queued transfers */
    while (object->headPtr != object->tailPtr) {
        object->headPtr = object->headPtr->nextPtr;
        object->transferCallbackFxn(handle, object->headPtr, false);
        Power_releaseConstraint(PowerCC26XX_SB_DISALLOW);
    }

    /* clean up object */
    object->mode = I2CCC26XX_IDLE_MODE;
    object->currentTransaction = NULL;
    object->headPtr = NULL;
    object->tailPtr = NULL;

    /* re-initialize the I2C peripheral */
    I2CMasterDisable(hwAttrs->baseAddr);
    I2CCC26XX_initHw(handle);

    Hwi_restore(key);
}

/*!
 *  @brief Function to close a given CC26XX I2C peripheral specified by the
 *         I2C handle.
 *
 *  After calling the close function, the I2C is disabled.
 *
 *  @pre    I2CCC26XX_open() has to be called first.
 *          Calling context: Task
 *
 *  @param handle An I2C_Handle returned by I2C_open()
 *
 *  @note  The generic I2C API should be used when accessing the I2CCC26XX.
 *
 *  @sa     I2CCC26XX_open(), I2C_close(), I2C_open()
 */
void I2CCC26XX_close(I2C_Handle handle)
{
    unsigned int                key;
    I2CCC26XX_Object            *object;
    I2CCC26XX_HWAttrsV1 const  *hwAttrs;

    /* Get the pointer to the object and hwAttrs */
    hwAttrs = handle->hwAttrs;
    object = handle->object;

    /* Check to see if a I2C transaction is in progress */
    Assert_isTrue(object->headPtr == NULL, NULL);

    /* Deallocate pins */
    PIN_close(object->hPin);

    /* Mask I2C interrupts */
    I2CMasterIntDisable(hwAttrs->baseAddr);

    /* Disable the I2C Master */
    I2CMasterDisable(hwAttrs->baseAddr);

    /* Power off the I2C module */
    Power_releaseDependency(hwAttrs->powerMngrId);

    /* Desctruct modules used in driver */
    Hwi_destruct(&(object->hwi));
    Swi_destruct(&(object->swi));
    Semaphore_destruct(&(object->mutex));
    if (object->transferMode == I2C_MODE_BLOCKING) {
        Semaphore_destruct(&(object->transferComplete));
    }

    /* Unregister power post notification object */
    Power_unregisterNotify(&object->i2cPostObj);

    /* Mark the module as available */
    key = Hwi_disable();
    object->isOpen = false;
    Hwi_restore(key);

    Log_print1(Diags_USER1, "I2C: Object closed 0x%x", hwAttrs->baseAddr);

    return;
}

/*!
 *  @brief  Function for setting control parameters of the I2C driver
 *          after it has been opened.
 *
 *  @note   Currently not in use.
 */
int I2CCC26XX_control(I2C_Handle handle, unsigned int cmd, void *arg)
{
    /* No implementation */
    return (I2C_STATUS_UNDEFINEDCMD);
}


/*
 *  ======== I2CCC26XX_hwiFxn ========
 *  Hwi interrupt handler to service the I2C peripheral
 *
 *  The handler is a generic handler for a I2C object.
 */
static void I2CCC26XX_hwiFxn(UArg arg)
{
    I2CDataType                 errStatus;
    I2CCC26XX_Object            *object;
    I2CCC26XX_HWAttrsV1 const  *hwAttrs;

    /* Get the pointer to the object and hwAttrs */
    object = ((I2C_Handle)arg)->object;
    hwAttrs = ((I2C_Handle)arg)->hwAttrs;

    /* Get the interrupt status of the I2C controller */
    errStatus = I2CMasterErr(hwAttrs->baseAddr);

    /* Clear interrupt source to avoid additional interrupts */
    I2CMasterIntClear(hwAttrs->baseAddr);

    /* Check for I2C Errors */
    if ((errStatus == I2C_MASTER_ERR_NONE) || (object->mode == I2CCC26XX_ERROR)) {

        /* No errors, now check what we need to do next */
        switch (object->mode) {

            /*
             * ERROR case : Error detected and STOP bit sent in previous interrupt;
             * this interrupt triggered by stop condition on bus.
             * Post SWI to complete the transfer
             */
            case I2CCC26XX_ERROR:
                Swi_post(Swi_handle(&(object->swi)));
                break;

            case I2CCC26XX_IDLE_MODE:
                Swi_post(Swi_handle(&(object->swi)));
                break;

            case I2CCC26XX_WRITE_MODE:
                /* Decrement write Counter */
                object->writeCountIdx--;

                /* Check if more data needs to be sent */
                if (object->writeCountIdx) {
                    Log_print3(Diags_USER2,
                            "I2C:(%p) ISR I2CCC26XX_WRITE_MODE: Data to write: 0x%x; "
                            "To slave: 0x%x",
                            hwAttrs->baseAddr,
                            *(object->writeBufIdx),
                            object->currentTransaction->slaveAddress);

                    /* Write data contents into data register */
                    I2CMasterDataPut(hwAttrs->baseAddr,
                                   *(object->writeBufIdx));
                    object->writeBufIdx++;

                    if ((object->writeCountIdx < 2) && !(object->readCountIdx)) {
                        /* Everything has been sent, nothing to receive */
                        /* Next state: Idle mode */
                        object->mode = I2CCC26XX_IDLE_MODE;

                        /* Send last byte with STOP bit */
                        I2CMasterControl(hwAttrs->baseAddr,
                                I2C_MASTER_CMD_BURST_SEND_FINISH);

                        Log_print1(Diags_USER2,
                                "I2C:(%p) ISR I2CCC26XX_WRITE_MODE: ACK received; "
                                "Writing w/ STOP bit",
                                hwAttrs->baseAddr);
                    }
                    else {
                        /*
                         * Either there is more date to be transmitted or some
                         * data needs to be received next
                         */
                        I2CMasterControl(hwAttrs->baseAddr,
                                I2C_MASTER_CMD_BURST_SEND_CONT);

                        Log_print1(Diags_USER2,
                                "I2C:(%p) ISR I2CCC26XX_WRITE_MODE: ACK received; Writing",
                                hwAttrs->baseAddr);
                    }
                }

                /* At this point, we know that we need to receive data */
                else {
                    /*
                     * We need to check after we are done transmitting data, if
                     * we need to receive any data.
                     * In a corner case when we have only one byte transmitted
                     * and no data to receive, the I2C will automatically send
                     * the STOP bit. In other words, here we only need to check
                     * if data needs to be received. If so, how much.
                     */
                    if (object->readCountIdx) {
                        /* Next state: Receive mode */
                        object->mode = I2CCC26XX_READ_MODE;

                        /* Switch into Receive mode */
                        I2CMasterSlaveAddrSet(hwAttrs->baseAddr,
                                object->currentTransaction->slaveAddress, true);

                        if (object->readCountIdx > 1) {
                            /* Send a repeated START */
                            I2CMasterControl(hwAttrs->baseAddr,
                                    I2C_MASTER_CMD_BURST_RECEIVE_START);

                            Log_print1(Diags_USER2,
                                "I2C:(%p) ISR I2CCC26XX_WRITE_MODE: -> I2CCC26XX_READ_MODE; "
                                "Reading w/ RESTART and ACK",
                                hwAttrs->baseAddr);
                        }
                        else {
                            /*
                             * Send a repeated START with a NACK since it's the
                             * last byte to be received.
                             * I2C_MASTER_CMD_BURST_RECEIVE_START_NACK is
                             * is locally defined because there is no macro to
                             * receive data and send a NACK after sending a
                             * start bit (0x00000003)
                             */
                            I2CMasterControl(hwAttrs->baseAddr,
                                    I2C_MASTER_CMD_BURST_RECEIVE_START_NACK);

                            Log_print1(Diags_USER2,
                                "I2C:(%p) ISR I2CCC26XX_WRITE_MODE: -> I2CCC26XX_READ_MODE; "
                                "Reading w/ RESTART and NACK",
                                hwAttrs->baseAddr);
                        }
                    }
                    else {
                        /* Done with all transmissions */
                        object->mode = I2CCC26XX_IDLE_MODE;
                        /*
                         * No more data needs to be received, so follow up with
                         * a STOP bit
                         * Again, there is no equivalent macro (0x00000004) so
                         * I2C_MASTER_CMD_BURST_RECEIVE_STOP is used.
                         */
                        I2CMasterControl(hwAttrs->baseAddr,
                                I2C_MASTER_CMD_BURST_RECEIVE_STOP);

                        Log_print1(Diags_USER2,
                                "I2C:(%p) ISR I2CCC26XX_WRITE_MODE: -> I2CCC26XX_IDLE_MODE; "
                                "Sending STOP bit",
                                hwAttrs->baseAddr);

                    }
                }
                break;

            case I2CCC26XX_READ_MODE:
                /* Save the received data */
                *(object->readBufIdx) =
                    I2CMasterDataGet(hwAttrs->baseAddr);

                Log_print2(Diags_USER2,
                        "I2C:(%p) ISR I2CCC26XX_READ_MODE: Read data byte: 0x%x",
                        hwAttrs->baseAddr,
                        *(object->readBufIdx));

                object->readBufIdx++;

                /* Check if any data needs to be received */
                object->readCountIdx--;
                if (object->readCountIdx) {
                    if (object->readCountIdx > 1) {
                        /* More data to be received */
                        I2CMasterControl(hwAttrs->baseAddr,
                                I2C_MASTER_CMD_BURST_RECEIVE_CONT);

                        Log_print1(Diags_USER2,
                                "I2C:(%p) ISR I2CCC26XX_READ_MODE: Reading w/ ACK",
                                hwAttrs->baseAddr);
                    }
                    else {
                        /*
                         * Send NACK because it's the last byte to be received
                         * There is no NACK macro equivalent (0x00000001) so
                         * I2C_MASTER_CMD_BURST_RECEIVE_CONT_NACK is used
                         */
                        I2CMasterControl(hwAttrs->baseAddr,
                                I2C_MASTER_CMD_BURST_RECEIVE_CONT_NACK);

                        Log_print1(Diags_USER2,
                                "I2C:(%p) ISR I2CCC26XX_READ_MODE: Reading w/ NACK",
                                hwAttrs->baseAddr);
                    }
                }
                else {
                    /* Next state: Idle mode */
                    object->mode = I2CCC26XX_IDLE_MODE;

                    /*
                     * No more data needs to be received, so follow up with a
                     * STOP bit
                     * Again, there is no equivalent macro (0x00000004) so
                     * I2C_MASTER_CMD_BURST_RECEIVE_STOP is used
                     */
                    I2CMasterControl(hwAttrs->baseAddr,
                            I2C_MASTER_CMD_BURST_RECEIVE_STOP);

                    Log_print1(Diags_USER2,
                            "I2C:(%p) ISR I2CCC26XX_READ_MODE: -> I2CCC26XX_IDLE_MODE; "
                            "Sending STOP bit",
                            hwAttrs->baseAddr);

                }

                break;

            default:
                object->mode = I2CCC26XX_ERROR;
                break;
        }

    }
    else {

        /* Some sort of error happened! */
        object->mode = I2CCC26XX_ERROR;

        if (errStatus & I2C_MASTER_ERR_ARB_LOST) {
            Swi_post(Swi_handle(&(object->swi)));
        }
        else {
            /* Try to send a STOP bit to end all I2C communications immediately */
            /*
             * I2C_MASTER_CMD_BURST_SEND_ERROR_STOP -and-
             * I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP
             * have the same values
             */
            I2CMasterControl(hwAttrs->baseAddr,
                             I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);

            /* Do not post SWI here to avoid subsequent transfers being stopped.
               Wait till the stop condition interrupt to post SWI */
        }

        Log_print2(Diags_USER1,
                "I2C:(%p) ISR I2C Bus fault (Status Reg: 0x%x)",
                hwAttrs->baseAddr,
                errStatus);
    }
    return;
}

/*
 *  ======== I2CCC26XX_swiFxn ========
 *  SWI interrupt handler to service the I2C peripheral
 *
 *  Takes care of cleanup and the callback in SWI context after an I2C transfer
 */
static void I2CCC26XX_swiFxn(UArg arg0, UArg arg1){
    I2CCC26XX_Object         *object;
    I2C_Handle               handle;
    int                      status;

    /* Get the I2C handle */
    handle = ((I2C_Handle)arg0);

    /* Get the pointer to the object */
    object = handle->object;

    Log_print1(Diags_USER2, "I2C:(%p) ISR Transfer Complete",
               ((I2CCC26XX_HWAttrsV1 const  *)(handle->hwAttrs))->baseAddr);

    /*
     * Perform callback in a SWI context, thus any tasks
     */
    object->transferCallbackFxn(handle,
                                object->currentTransaction,
                                !((bool)object->mode));

    /* See if we need to process any other transactions */
    if (object->headPtr == object->tailPtr) {
        /* No other transactions need to occur */
        object->currentTransaction = NULL;
        object->headPtr = NULL;

        /* Release standby disallow constraint. */
        Power_releaseConstraint(PowerCC26XX_SB_DISALLOW);

        Log_print1(Diags_USER2,
                "I2C:(%p) ISR No other I2C transaction in queue",
               ((I2CCC26XX_HWAttrsV1 const  *)(handle->hwAttrs))->baseAddr);
    }
    else {
        /* Another transfer needs to take place */
        object->headPtr = object->headPtr->nextPtr;

        Log_print2(Diags_USER2,
                "I2C:(%p) ISR Priming next I2C transaction "
                "(%p) from queue",
                ((I2CCC26XX_HWAttrsV1 const  *)(handle->hwAttrs))->baseAddr,
                (UArg)object->headPtr);

        status = I2CCC26XX_primeTransfer(handle, object->headPtr);

        /* Call back now if not able to start transfer */
        if (status == I2C_STATUS_ERROR) {
            object->mode = I2CCC26XX_BUSBUSY_MODE;
            Swi_post(Swi_handle(&(object->swi)));
        }
    }
}

/*!
 *  @brief I2C CC26XX initialization
 *
 *  @param handle  An I2C_Handle
 *
 *  @pre    Calling context: Hwi, Swi, Task, Main
 *
 *  @note  The generic I2C API should be used when accessing the I2CCC26XX.
 */
void I2CCC26XX_init(I2C_Handle handle)
{
    I2CCC26XX_Object           *object;

    /* Get the pointer to the object */
    object = handle->object;

    /* Initially the drivers is not open */
    object->isOpen = false;

}

/*!
 *  @brief Function to initialize a given I2C CC26XX peripheral specified by the
 *         particular handle. The parameter specifies which mode the I2C
 *         will operate.
 *
 *  After calling the open function, the I2C is enabled. If there is no active
 *  I2C transactions, the device can enter standby.
 *
 *  @pre    The I2CCC26XX_Config structure must exist and be persistent before this
 *          function can be called. I2CCC26XX has been initialized with I2CCC26XX_init().
 *          Calling context: Task
 *
 *  @param  handle   An I2C_Handle
 *
 *  @param  params   Pointer to a parameter block, if NULL it will use default values.
 *
 *  @return A I2C_Handle on success, or a NULL on an error or if it has been
 *          already opened.
 *
 *  @note  The generic I2C API should be used when accessing the I2CCC26XX.
 *
 *  @sa     I2CCC26XX_close(), I2CCC26XX_init(), I2C_open(), I2C_init()
 */
I2C_Handle I2CCC26XX_open(I2C_Handle handle, I2C_Params *params)
{
    union {
        Hwi_Params              hwiParams;
        Swi_Params              swiParams;
        Semaphore_Params        semParams;
    } paramsUnion;
    UInt                            key;
    I2CCC26XX_Object                *object;
    I2CCC26XX_HWAttrsV1 const      *hwAttrs;

    /* Get the pointer to the object and hwAttrs */
    object = handle->object;
    hwAttrs = handle->hwAttrs;

    /* Determine if the device index was already opened */
    key = Hwi_disable();
    if(object->isOpen == true){
        Hwi_restore(key);
        return (NULL);
    }

    /* Mark the handle as being used */
    object->isOpen = true;
    Hwi_restore(key);

    /* Configure the IOs early to ensure allocation is allowed (PIN driver and IO config setup only).*/
    if (I2CCC26XX_initIO(handle, params->custom)) {
      /* If initialization and allocation of IOs failed, log error and return NULL pointer */
      Log_print1(Diags_USER1, "I2C: Pin allocation failed, open did not succeed (baseAddr:0x%x)", hwAttrs->baseAddr);
      return (NULL);
    }

    /* Save parameters */
    object->transferMode = params->transferMode;
    object->transferCallbackFxn = params->transferCallbackFxn;
    object->bitRate = params->bitRate;

    /* Create Hwi object for this I2C peripheral */
    Hwi_Params_init(&paramsUnion.hwiParams);
    paramsUnion.hwiParams.arg = (UArg)handle;
    paramsUnion.hwiParams.priority = hwAttrs->intPriority;
    Hwi_construct(&(object->hwi), hwAttrs->intNum, I2CCC26XX_hwiFxn,
                  &paramsUnion.hwiParams, NULL);

    /* Create Swi object for this I2C peripheral */
    Swi_Params_init(&(paramsUnion.swiParams));
    paramsUnion.swiParams.arg0 = (UArg)handle;
    paramsUnion.swiParams.priority = hwAttrs->swiPriority;
    Swi_construct(&(object->swi), I2CCC26XX_swiFxn, &(paramsUnion.swiParams), NULL);

    /*
     * Create thread safe handles for this I2C peripheral
     * Semaphore to provide exclusive access to the I2C peripheral
     */
    Semaphore_Params_init(&paramsUnion.semParams);
    paramsUnion.semParams.mode = Semaphore_Mode_BINARY;
    Semaphore_construct(&(object->mutex), 1, &paramsUnion.semParams);

    /*
     * Store a callback function that posts the transfer complete
     * semaphore for synchronous mode
     */
    if (object->transferMode == I2C_MODE_BLOCKING) {
        /*
         * Semaphore to cause the waiting task to block for the I2C
         * to finish
         */
        Semaphore_construct(&(object->transferComplete), 0, &paramsUnion.semParams);

        /* Store internal callback function */
        object->transferCallbackFxn = I2CCC26XX_blockingCallback;
    }
    else {
        /* Check to see if a callback function was defined for async mode */
        Assert_isTrue(object->transferCallbackFxn != NULL, NULL);
    }

    /* Specify the idle state for this I2C peripheral */
    object->mode = I2CCC26XX_IDLE_MODE;

    /* Clear the head pointer */
    object->headPtr = NULL;
    object->tailPtr = NULL;

    /* Power on the I2C module */
    Power_setDependency(hwAttrs->powerMngrId);

    /* Initialize the I2C hardware module */
    I2CCC26XX_initHw(handle);

    /* Register notification functions */
    Power_registerNotify(&object->i2cPostObj, PowerCC26XX_AWAKE_STANDBY, (Fxn)i2cPostNotify, (UInt32)handle);

    Log_print1(Diags_USER1, "I2C: Object created 0x%x", hwAttrs->baseAddr);

    /* Return the address of the handle */
    return (handle);
}

/*
 *  ======== I2CCC26XX_primeTransfer =======
 */
static int I2CCC26XX_primeTransfer(I2C_Handle handle,
                                   I2C_Transaction *transaction)
{
    I2CCC26XX_Object            *object;
    I2CCC26XX_HWAttrsV1 const  *hwAttrs;

    /* Get the pointer to the object and hwAttrs */
    object = handle->object;
    hwAttrs = handle->hwAttrs;

    /* Store the new internal counters and pointers */
    object->currentTransaction = transaction;

    object->writeBufIdx = transaction->writeBuf;
    object->writeCountIdx = transaction->writeCount;

    object->readBufIdx = transaction->readBuf;
    object->readCountIdx = transaction->readCount;

    Log_print2(Diags_USER1,
            "I2C:(%p) Starting transaction to slave: 0x%x",
            hwAttrs->baseAddr,
            object->currentTransaction->slaveAddress);

        /* Start transfer in Transmit mode */
        if (object->writeCountIdx) {
            /* Specify the I2C slave address */
            I2CMasterSlaveAddrSet(hwAttrs->baseAddr,
                                  object->currentTransaction->slaveAddress, false);
            /* Update the I2C mode */
            object->mode = I2CCC26XX_WRITE_MODE;

            Log_print3(Diags_USER2,
                    "I2C:(%p) I2CCC26XX_IDLE_MODE: Data to write: 0x%x; To Slave: 0x%x",
                    hwAttrs->baseAddr,
                    *(object->writeBufIdx),
                      object->currentTransaction->slaveAddress);

            /* Write data contents into data register */
            I2CMasterDataPut(hwAttrs->baseAddr,
                    *((object->writeBufIdx)++));

            /* Check bus status, return with error if busy */
            if (I2CMasterBusBusy(hwAttrs->baseAddr)) {
                return I2C_STATUS_ERROR;
            }

            /* Start the I2C transfer in master transmit mode */
            I2CMasterControl(hwAttrs->baseAddr, I2C_MASTER_CMD_BURST_SEND_START);

            Log_print1(Diags_USER2,
                    "I2C:(%p) I2CCC26XX_IDLE_MODE: -> I2CCC26XX_WRITE_MODE; "
                    "Writing w/ START",
                    hwAttrs->baseAddr);
        }

        /* Start transfer in Receive mode */
        else {
            /* Specify the I2C slave address */
            I2CMasterSlaveAddrSet(hwAttrs->baseAddr,
                                  object->currentTransaction->slaveAddress, true);

            /* Update the I2C mode */
            object->mode = I2CCC26XX_READ_MODE;

            /* Check bus status, return with error if busy */
            if (I2CMasterBusBusy(hwAttrs->baseAddr)) {
                return I2C_STATUS_ERROR;
            }

            if (object->readCountIdx < 2) {
                /* Start the I2C transfer in master receive mode */
                I2CMasterControl(hwAttrs->baseAddr,
                        I2C_MASTER_CMD_BURST_SEND_START);

                Log_print1(Diags_USER2,
                        "I2C:(%p) I2CCC26XX_IDLE_MODE: -> I2CCC26XX_READ_MODE; "
                        "Reading w/ NACK",
                        hwAttrs->baseAddr);
            }
            else {
                /* Start the I2C transfer in master receive mode */
                I2CMasterControl(hwAttrs->baseAddr,
                        I2C_MASTER_CMD_BURST_RECEIVE_START);

                Log_print1(Diags_USER2,
                        "I2C:(%p) I2CCC26XX_IDLE_MODE: -> I2CCC26XX_READ_MODE; "
                        "Reading w/ ACK",
                        hwAttrs->baseAddr);
            }
        }

    return I2C_STATUS_SUCCESS;
}

/*!
 *  @brief Function to start a transfer from the CC26XX I2C peripheral specified
 *         by the I2C handle.
 *
 *  This function is used for both transmitting and receiving data. If the I2C
 *  is configured in ::I2C_MODE_CALLBACK mode, it is possible to chain transactions
 *  together and receive a callback when all transactions are done.
 *  When active I2C transactions exist, the device might enter idle, not standby.
 *
 *  @pre    I2CCC26XX_open() has to be called first.
 *          Calling context: Hwi and Swi (only if using ::I2C_MODE_CALLBACK), Task
 *
 *  @param handle An I2C_Handle returned by I2C_open()
 *
 *  @param transaction Pointer to a I2C transaction object
 *
 *  @return TRUE on successful transfer.
 *          FALSE on an error, such as a I2C bus fault.
 *
 *  @note  The generic I2C API should be used when accessing the I2CCC26XX.
 *
 *  @sa    I2CCC26XX_open(), I2C_transfer()
 */
bool I2CCC26XX_transfer(I2C_Handle handle,
                        I2C_Transaction *transaction)
{
    bool                        ret = false;
    UInt                        key;
    I2CCC26XX_Object            *object;
    I2CCC26XX_HWAttrsV1 const  *hwAttrs;
    int                         status;

    /* Get the pointer to the object and hwAttrs */
    object = handle->object;
    hwAttrs = handle->hwAttrs;

    /* Check if anything needs to be written or read */
    if ((!transaction->writeCount) && (!transaction->readCount)) {
        /* Nothing to write or read */
        return (ret);
    }

    if (object->transferMode == I2C_MODE_CALLBACK) {
        /* Check if a transfer is in progress */
        key = Hwi_disable();
        if (object->headPtr) {
            /* Transfer in progress */

            /*
             * Update the message pointed by the tailPtr to point to the next
             * message in the queue
             */
            object->tailPtr->nextPtr = transaction;

            /* Update the tailPtr to point to the last message */
            object->tailPtr = transaction;

            /* I2C is still being used */
            Hwi_restore(key);
            return (true);
        }
        else {
            /* Store the headPtr indicating I2C is in use */
            object->headPtr = transaction;
            object->tailPtr = transaction;
        }
        Hwi_restore(key);
    }

    /* Set standby disallow constraint. */
    Power_setConstraint(PowerCC26XX_SB_DISALLOW);

    /* Acquire the lock for this particular I2C handle */
    Semaphore_pend(Semaphore_handle(&(object->mutex)), BIOS_WAIT_FOREVER);

    /*
     * I2CCC26XX_primeTransfer is a longer process and
     * protection is needed from the I2C interrupt
     */

    Hwi_disableInterrupt(hwAttrs->intNum);
    status = I2CCC26XX_primeTransfer(handle, transaction);
    Hwi_enableInterrupt(hwAttrs->intNum);

    if (object->transferMode == I2C_MODE_BLOCKING) {
        if (status == I2C_STATUS_ERROR) {
            Log_print1(Diags_USER1,
                    "I2C:(%p) Bus busy, transaction not started",
                    hwAttrs->baseAddr);
            object->mode = I2CCC26XX_BUSBUSY_MODE;
            ret = false;

            /* Release standby disallow constraint. */
            Power_releaseConstraint(PowerCC26XX_SB_DISALLOW);
        }
        else {
            Log_print1(Diags_USER1,
                    "I2C:(%p) Pending on transferComplete semaphore",
                    hwAttrs->baseAddr);
            /*
             * Wait for the transfer to complete here.
             * It's OK to block from here because the I2C's Hwi will unblock
             * upon errors
             */
            //Semaphore_pend(Semaphore_handle(&(object->transferComplete)), BIOS_WAIT_FOREVER);
            Semaphore_pend(Semaphore_handle(&(object->transferComplete)), 100000);

            /* No need to release standby disallow constraint here - done in swi */

            Log_print1(Diags_USER1,
                    "I2C:(%p) Transaction completed",
                    hwAttrs->baseAddr);

            /* Hwi handle has posted a 'transferComplete' check for Errors */
            if (object->mode == I2CCC26XX_IDLE_MODE) {
                Log_print1(Diags_USER1,
                        "I2C:(%p) Transfer OK",
                        hwAttrs->baseAddr);
                ret = true;
            }
        }
    }
    else {
        /* Call back now if not able to start transfer */
        if (status == I2C_STATUS_ERROR) {
            object->mode = I2CCC26XX_BUSBUSY_MODE;
            Swi_post(Swi_handle(&(object->swi)));
            ret = false;
        }
        else {
            /* Return true if transaction is started */
            ret = true;
        }
    }

    /* Release the lock for this particular I2C handle */
    Semaphore_post(Semaphore_handle(&(object->mutex)));

    /* Return status */
    return (ret);
}

/*
 *  ======== I2CCC26XX_blockingCallback ========
 */
static void I2CCC26XX_blockingCallback(I2C_Handle handle,
                                       I2C_Transaction *msg,
                                       bool transferStatus)
{
    I2CCC26XX_Object        *object;

    Log_print1(Diags_USER1, "I2C:(%p) posting transferComplete semaphore",
               ((I2CCC26XX_HWAttrsV1 const  *)(handle->hwAttrs))->baseAddr);

    /* Get the pointer to the object */
    object = handle->object;

    /* Indicate transfer complete */
    Semaphore_post(Semaphore_handle(&(object->transferComplete)));
}

/*
 *  ======== I2CCC26XX_hwInit ========
 *  This functions initializes the I2C hardware module.
 *
 *  @pre    Function assumes that the I2C handle is pointing to a hardware
 *          module which has already been opened.
 */
static void I2CCC26XX_initHw(I2C_Handle handle) {
    I2CCC26XX_Object            *object;
    I2CCC26XX_HWAttrsV1 const  *hwAttrs;
    Types_FreqHz freq;

    /* Get the pointer to the object and hwAttrs */
    object = handle->object;
    hwAttrs = handle->hwAttrs;

    /* Set the I2C configuration */
    BIOS_getCpuFreq(&freq);
    I2CMasterInitExpClk(hwAttrs->baseAddr, freq.lo, bitRate[object->bitRate]);

    /* Clear any pending interrupts */
    I2CMasterIntClear(hwAttrs->baseAddr);

    /* Enable the I2C Master for operation */
    I2CMasterEnable(hwAttrs->baseAddr);

    /* Unmask I2C interrupts */
    I2CMasterIntEnable(hwAttrs->baseAddr);
}

/*
 *  ======== I2CCC26XX_initIO ========
 *  This functions initializes the I2C IOs.
 *
 *  @pre    Function assumes that the I2C handle is pointing to a hardware
 *          module which has already been opened.
 */
static int I2CCC26XX_initIO(I2C_Handle handle, uintptr_t pinCfg) {
    I2CCC26XX_Object            *object;
    I2CCC26XX_HWAttrsV1 const  *hwAttrs;
    I2CCC26XX_I2CPinCfg         i2cPins;
    PIN_Config                  i2cPinTable[3];
    uint32_t i=0;

    /* Get the pointer to the object and hwAttrs */
    object = handle->object;
    hwAttrs = handle->hwAttrs;

    /* If the pinCfg pointer is NULL, use hwAttrs pins */
    if (pinCfg == NULL) {
        i2cPins.pinSDA = hwAttrs->sdaPin;
        i2cPins.pinSCL = hwAttrs->sclPin;
    } else {
        i2cPins.pinSDA = ((I2CCC26XX_I2CPinCfg *)pinCfg)->pinSDA;
        i2cPins.pinSCL = ((I2CCC26XX_I2CPinCfg *)pinCfg)->pinSCL;
    }

    /* Handle error */
    if(i2cPins.pinSDA == PIN_UNASSIGNED || i2cPins.pinSCL == PIN_UNASSIGNED) {
        return I2C_STATUS_ERROR;
    }
    /* Configure I2C pins SDA and SCL*/
    i2cPinTable[i++] = i2cPins.pinSDA | PIN_INPUT_EN | PIN_PULLUP | PIN_OPENDRAIN;
    i2cPinTable[i++] = i2cPins.pinSCL | PIN_INPUT_EN | PIN_PULLUP | PIN_OPENDRAIN;
    i2cPinTable[i++] = PIN_TERMINATE;
    /* Allocate pins*/
    object->hPin = PIN_open(&object->pinState, i2cPinTable);

    if (!object->hPin) {
        return I2C_STATUS_ERROR;
    }

    /* Set IO muxing for the UART pins */
    PINCC26XX_setMux(object->hPin, i2cPins.pinSDA, IOC_PORT_MCU_I2C_MSSDA);
    PINCC26XX_setMux(object->hPin, i2cPins.pinSCL, IOC_PORT_MCU_I2C_MSSCL);
    return I2C_STATUS_SUCCESS;
}

/*
 *  ======== i2cPostNotify ========
 *  This functions is called to notify the I2C driver of an ongoing transition
 *  out of sleep mode.
 *
 *  @pre    Function assumes that the I2C handle (clientArg) is pointing to a
 *          hardware module which has already been opened.
 */
static int i2cPostNotify(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg)
{
    /* Return value */
    int res;

    /* reconfigure the hardware if returning from sleep*/
    if (eventType == PowerCC26XX_AWAKE_STANDBY) {
         I2CCC26XX_initHw((I2C_Handle)clientArg);
    }

    res = Power_NOTIFYDONE;
    return res;
}
