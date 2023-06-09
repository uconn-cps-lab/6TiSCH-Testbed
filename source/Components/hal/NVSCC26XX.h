/*
 * Copyright (c) 2015, Texas Instruments Incorporated
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
 *  ======== NVSCC26XX.h ========
 */

#ifndef ti_drivers_nvs_NVSCC26XX__include
#define ti_drivers_nvs_NVSCC26XX__include
#ifdef DeviceFamily_CC26X2

#include <stdint.h>
#include <stdbool.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @brief   Error status code returned by NVS_erase(), NVS_write().
 *
 *  This error status is returned if the system voltage is too low to safely
 *  perform the flash operation. Voltage must be 1.5V or greater.
 */
#define NVSCC26XX_STATUS_LOW_VOLTAGE    (NVS_STATUS_RESERVED - 1)

/*!
 *  @internal @brief NVS function pointer table
 *
 *  'NVSCC26XX_fxnTable' is a fully populated function pointer table
 *  that can be referenced in the NVS_config[] array entries.
 *
 *  Users can minimize their application code size by providing their
 *  own custom NVS function pointer table that contains only those APIs
 *  used by the application.
 *
 *  An example of a custom NVS function table is shown below:
 *  @code
 *  //
 *  // Since the application does not use the
 *  // NVS_control(), NVS_lock(), and NVS_unlock() APIs,
 *  // these APIs are removed from the function
 *  // pointer table and replaced with NULL
 *  //
 *  const NVS_FxnTable myNVS_fxnTable = {
 *      NVSCC26XX_close,
 *      NULL,     // remove NVSCC26XX_control(),
 *      NVSCC26XX_erase,
 *      NVSCC26XX_getAttrs,
 *      NVSCC26XX_init,
 *      NULL,     // remove NVSCC26XX_lock(),
 *      NVSCC26XX_open,
 *      NVSCC26XX_read,
 *      NULL,     // remove NVSCC26XX_unlock(),
 *      NVSCC26XX_write
 *  };
 *  @endcode
 */
extern const NVS_FxnTable NVSCC26XX_fxnTable;

/*!
 *  @brief      NVSCC26XX hardware attributes
 *
 *  The NVSCC26XX hardware attributes define hardware specific settings
 *  for a NVS driver instance.
 *
 *  \note Care must be taken to ensure that the linker does not place application
 *  content (such as .text or .const) in the flash regions defined by the
 *  this hardware attributes structure.
 *
 *  For CCS and IAR tools, defining and reserving flash memory regions can
 *  be done entirely within the Board.c file. For GCC, additional content is
 *  required in the application's linker script to achieve the same
 *  result.
 *
 *  The example below defines a char array \p flashBuf. Preprocessor logic is
 *  used so that this example will work with either the TI, IAR or GCC tools.
 *  For the TI and IAR tools, pragmas are used to place \p flashBuf at the
 *  flash location specified by #NVSCC26XX_HWAttrs.regionBase.
 *
 *  For the GCC tool, the \p flashBuf array is placed into a named linker output
 *  section, \p .nvs. This section is defined in the application's linker
 *  script. The section placement command is carefully chosen to only RESERVE
 *  space for the \p flashBuf array, and not to actually initialize it during
 *  the application load process, thus preserving the content of flash.
 *
 *  Regardless of tool chain, the \p flashBuf array in the example below is
 *  placed at the \p NVS_REGIONS_BASE address and has an overall size of
 *  \p REGIONSIZE bytes. Theoretically, the memory reserved by \p flashBuf can
 *  be divided into four separate regions, each having a size of \p SECTORSIZE
 *  bytes. Each region must always be aligned to the flash sector size,
 *  \p SECTORSIZE. This example below shows two regions defined.
 *
 *  An array of two #NVSCC26XX_HWAttrs structures is defined. Each index
 *  of this structure defines a region of on-chip flash memory. Both regions
 *  utilize memory reserved by the \p flashBuf array. The two regions do not
 *  overlap or share the same physical memory locations. The two regions do
 *  however exist adjacent to each other in physical memory. The first
 *  region is defined as starting at the \p NVS_REGIONS_BASE address and has a
 *  size equal to the flash sector size, as defined by \p SECTORSIZE. The second
 *  region is defined as starting at (NVS_REGIONS_BASE + SECTORSIZE), that is,
 *  the \p NVS_REGIONS_BASE address offset by \p SECTORSIZE bytes. The second region
 *  has a size equal to (3 * SECTORSIZE) bytes. These regions together fully
 *  occupy \p REGIONSIZE bytes of physical on-chip flash memory as reserved by
 *  the \p flashBuf array.
 *
 *  @code
 *  #define NVS_REGIONS_BASE 0x1B000
 *  #define SECTORSIZE       0x1000
 *  #define REGIONSIZE       (SECTORSIZE * 4)
 *
 *  //
 *  // Reserve flash sectors for NVS driver use
 *  // by placing an uninitialized byte array
 *  // at the desired flash address.
 *  //
 *  #if defined(__TI_COMPILER_VERSION__)
 *
 *  //
 *  //  Place uninitialized array at FLASH_REGION_BASE
 *  //
 *  #pragma LOCATION(flashBuf, FLASH_REGION_BASE);
 *  #pragma NOINIT(flashBuf);
 *  char flashBuf[REGIONSIZE];
 *
 *  #elif defined(__IAR_SYSTEMS_ICC__)
 *
 *  //
 *  //  Place uninitialized array at FLASH_REGION_BASE
 *  //
 *  __no_init char flashBuf[REGIONSIZE] @ FLASH_REGION_BASE;
 *
 *  #elif defined(__GNUC__)
 *
 *  //
 *  //  Place the flash buffers in the .nvs section created in the gcc linker file.
 *  //  The .nvs section enforces alignment on a sector boundary but may
 *  //  be placed anywhere in flash memory.  If desired the .nvs section can be set
 *  //  to a fixed address by changing the following in the gcc linker file:
 *  //
 *  //  .nvs (FIXED_FLASH_ADDR) (NOLOAD) : AT (FIXED_FLASH_ADDR) {
 *  //       *(.nvs)
 *  //  } > REGION_TEXT
 *  //
 *
 *  __attribute__ ((section (".nvs")))
 *  char flashBuf[REGIONSIZE];
 *
 *  #endif
 *
 *  NVSCC26XX_HWAttrs nvsCC26XXHWAttrs[2] = {
 *      //
 *      // region 0 is 1 flash sector in length.
 *      //
 *      {
 *          .regionBase = (void *)flashBuf,
 *          .regionSize = SECTORSIZE,
 *      },
 *      //
 *      // region 1 is 3 flash sectors in length.
 *      //
 *      {
 *          .regionBase = (void *)(flashBuf + SECTORSIZE),
 *          .regionSize = SECTORSIZE * 3,
 *      }
 *  };
 *  @endcode
 *
 *  Example GCC linker script file content. This example places an output
 *  section, \p .nvs, at the memory address \p 0x1B000. The \p NOLOAD directive
 *  is used so that this memory is not initialized during program load to the
 *  target.
 *
 *  @code
 *  MEMORY
 *  {
 *      FLASH (RX)      : ORIGIN = 0x00000000, LENGTH = 0x0001ffa8
 *      FLASH_CCFG (RX) : ORIGIN = 0x0001ffa8, LENGTH = 0x00000058
 *      SRAM (RWX)      : ORIGIN = 0x20000000, LENGTH = 0x00005000
 *  }
 *
 *  .nvs (0x1b000) (NOLOAD) : AT (0x1b000) {
 *      *(.nvs)
 *  } > REGION_TEXT
 *  @endcode
 *
 *  If the write "scoreboard" is enabled, three new fields are added to the
 *  NVSCC26XX_HWAttrs structure:
 *    * scoreboard - a buffer provided by the application where each byte
 *      represents how many times a page has been written to.  It is important
 *      that this buffer be large enough such that there is a byte for each
 *      page of memory in the NVS region.  For example:
 *        - 64k NVS region
 *        - 256 byte page size
 *        - 64k / 256 = 256; the scoreboard buffer must be 256 bytes in length
 *
 *    * scoreboardSize - number of bytes in the scoreboard.
 *
 *    * flashPageSize - number of bytes in a flash page (i.e. 128 or 256)
 */
typedef struct NVSCC26XX_HWAttrs {
    void        *regionBase;    /*!< The regionBase field specifies the base
                                     address of the on-chip flash memory to be
                                     managed. The regionBase must be aligned
                                     to the flash sector size. This memory
                                     cannot be shared and must be for exclusive
                                     use by one NVS driver instance. */

    size_t       regionSize;    /*!< The regionSize field specifies the
                                     overall size of the on-chip flash memory
                                     to be managed. The regionSize must be at
                                     least 1 flash sector size AND an integer
                                     multiple of the flash sector size. For most
                                     CC26XX/CC13XX devices, the flash sector
                                     size is 4096 bytes. The NVSCC26XX driver
                                     will determine the device's actual sector
                                     size by reading internal system
                                     configuration registers. */

#if defined(NVSCC26XX_INSTRUMENTED)
    uint8_t     *scoreboard;         /*!< Pointer to scoreboard */
    size_t       scoreboardSize;     /*!< Scoreboard size in bytes */
    uint32_t     flashPageSize;      /*!< Size of a memory page in bytes */
#endif
} NVSCC26XX_HWAttrs;

/*
 *  @brief      NVSCC26XX Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct NVSCC26XX_Object {
    bool        opened;             /* Has this region been opened */
} NVSCC26XX_Object;

/*!
 *  @cond NODOC
 *  NVSCC26XX driver public APIs
 */

extern void         NVSCC26XX_close(NVS_Handle handle);
extern int_fast16_t NVSCC26XX_control(NVS_Handle handle, uint_fast16_t cmd,
                        uintptr_t arg);
extern int_fast16_t NVSCC26XX_erase(NVS_Handle handle, size_t offset,
                        size_t size);
extern void         NVSCC26XX_getAttrs(NVS_Handle handle, NVS_Attrs *attrs);
extern void         NVSCC26XX_init();
extern int_fast16_t NVSCC26XX_lock(NVS_Handle handle, uint32_t timeout);
extern NVS_Handle   NVSCC26XX_open(uint_least8_t index, NVS_Params *params);
extern int_fast16_t NVSCC26XX_read(NVS_Handle handle, size_t offset,
                        void *buffer, size_t bufferSize);
extern void         NVSCC26XX_unlock(NVS_Handle handle);
extern int_fast16_t NVSCC26XX_write(NVS_Handle handle, size_t offset,
                        void *buffer, size_t bufferSize, uint_fast16_t flags);
/*! @endcond */

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
#else  //DeviceFamily_CC26X2
#include <stdint.h>
#include <stdbool.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @brief  Command to set the copy block for an NVS block.
 *
 *  Passing NVSCC26XX_CMD_SET_COPYBLOCK to NVS_control(), along with
 *  a block of memory, is used to set the copy block for an
 *  NVSCC26XX_HWAttrs structure.
 *  The copy block is used as scratch when writing to a flash block.  Since
 *  the block must be erased before writing to it, the data in the block
 *  that is outside of the region to be modified, must be preserved.  It
 *  will be copied into the copy block, along with the buffer of data
 *  passed to NVS_write().  The block is then erased and the copy block
 *  copied back to the block.  If the copy block is not known at compile
 *  time, for example, if it is allocated from heap memory, it can be set
 *  through NVS_control() using the command NVSCC26XX_CMD_SET_COPYBLOCK.
 *  The copy block is passed in the arg parameter of NVS_control().  The
 *  size of the copy block passed to NVS_control() must be at least
 *  as large as the block size, and it is up to the application to ensure
 *  this.
 *
 *  @sa NVSCC26XX_HWAttrs
 */
#define NVSCC26XX_CMD_SET_COPYBLOCK      NVS_CMD_RESERVED + 0

/*!
 *  @brief  Alignment error returned by NVSCC26XX_control().
 *
 *  This error is returned if the copy block passed to NVSCC26XX_control()
 *  is not aligned on a 4-byte boundary, or is NULL.
 *
 *  @sa NVSCC26XX_HWAttrs
 */
#define NVSCC26XX_STATUS_ECOPYBLOCK      (NVS_STATUS_RESERVED - 1)

/*!
 *  @brief      NVSCC26XX command structure for setting copy block.
 *
 *  This structure is used to hold the copy block information that is
 *  passed to NVS_control().  If copyBlock is a buffer in RAM, isRam
 *  should be set to TRUE.  If copyBlock is in Flash, set isRam to
 *  FALSE.
 */
typedef struct NVSCC26XX_CmdSetCopyBlockArgs
{
    void *copyBlock;     /*!< Address of the copy block */
    bool isRam;          /*!< TRUE if copyBlock is a RAM buffer */
} NVSCC26XX_CmdSetCopyBlockArgs;

/* NVS function table pointer */
extern const NVS_FxnTable NVSCC26XX_fxnTable;

/*!
 *  @brief      NVSCC26XX attributes
 *
 *  The block is the address of a region in flash of size blockSize bytes.
 *
 *  For CC26XX devices, the smallest erase page size is 4KB, so in most
 *  cases, blockSize should be set to 4KB for this device.  If the
 *  blockSize is less than the page size, care should be taken not to use
 *  the rest of the page.  A write to the block will cause the entire page
 *  to be erased!  A blockSize greater than the page size is not supported.
 *  The page size for the device can be obtained through NVS_getAttrs().
 *
 *  When the block is written to, a scratch region is needed to preserve
 *  the unmodified data in the block.  This scratch region, referred to as
 *  copyBlock, can be a page in flash or a buffer in RAM.  The application
 *  can set copyBlock in the HWAttrs directly, if it is known at compile
 *  time, or set copyBlock through NVS_control(), for example, if it is
 *  allocated on the heap.  The copyBlock can be shared accross multiple
 *  NVS instances.  It is up to the application to ensure that copyBlock
 *  is set before the first call to NVS_write().
 *  Using a blockSize less than the page size decreases RAM or heap only
 *  if copyBlock is not in flash.
 */
typedef struct NVSCC26XX_HWAttrs {
    void         *block;      /*!< Address of flash block to manage */
    size_t        blockSize;  /*!< The size of block */
    void         *copyBlock;  /*!< A RAM buffer or flash block to use for
                               *   scratch when writing to the block.
                               */
    bool          isRam;      /*!< TRUE if copyBlock is a RAM buffer */
} NVSCC26XX_HWAttrs;

/*
 *  @brief      NVSCC26XX Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct NVSCC26XX_Object {
    bool                 opened;   /* Has the obj been opened */
} NVSCC26XX_Object;

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
#endif  //DeviceFamily_CC26X2
/*@}*/
#endif /* ti_drivers_nvs_NVSCC26XX__include */
