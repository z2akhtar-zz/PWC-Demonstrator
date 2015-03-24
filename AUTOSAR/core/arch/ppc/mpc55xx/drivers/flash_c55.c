/*-------------------------------- Arctic Core ------------------------------
 * Copyright (C) 2013, ArcCore AB, Sweden, www.arccore.com.
 * Contact: <contact@arccore.com>
 * 
 * You may ONLY use this file:
 * 1)if you have a valid commercial ArcCore license and then in accordance with  
 * the terms contained in the written license agreement between you and ArcCore, 
 * or alternatively
 * 2)if you follow the terms found in GNU General Public License version 2 as 
 * published by the Free Software Foundation and appearing in the file 
 * LICENSE.GPL included in the packaging of this file or here 
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>
 *-------------------------------- Arctic Core -----------------------------*/

/*
 * flash_h7f_c90.c
 *
 *  Created on: 29 aug 2011
 *      Author: mahi
 *
 * Interface for the low level flash written by freescale (flash_ll_h7f_c90.c )
 *
 * This file aims to support support all mpc55xx as well as mpc56xx.
 */

/** @tagSettings DEFAULT_ARCHITECTURE=NOT_USED */

/* ----------------------------[includes]------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "typedefs.h"
#include "io.h"
#include "flash.h"
#include "Fls_Cfg.h"

#define USE_DEBUG_PRINTF
#include "debug.h"

#define ASSERT(_x)  assert(_x)

/* ----------------------------[private define]------------------------------*/

/* Module Configuration Register */
#define C55FL_MCR_DONE               0x00000400   /* State Machine Status */
#define C55FL_MCR_PEG                0x00000200   /* Program/Erase Good */
#define C55FL_MCR_PGM                0x00000010   /* Program */
#define C55FL_MCR_ERS                0x00000004   /* Erase */
#define C55FL_MCR_EHV                0x00000001   /* Enable High Voltage */

#define C55FL_MCR             0x0000       /* Module Configuration Register */
#define C55FL_LML             0x0010       /* Low/Mid Address Space Block Locking Register */
#define C55FL_HBL             0x0014       /* High Address Space Block Locking Register */
#define C55FL_LBL             0x0018       /* Large Address Space Block Locking Register */
#define C55FL_LMS             0x0038       /* Low/Mid Address Space Block Select Register */
#define C55FL_HBS             0x003C       /* High Address Space Block Select Register */
#define C55FL_LBS             0x0040       /* Large Address Space Block Select Register */

#if 0
#define C55FL_SLL             0x000C       /* Secondary Low/Mid Address Space Block Locking Register */

#define NVLML_LME           (1u<<31)

#define PASSWORD_LOW_MID  			0xA1A11111UL
#define PASSWORD_HIGH				0xB2B22222UL
#define PASSWORD_SECONDARY_LOW_MID	0xC3C33333UL
#endif
/* ----------------------------[private macro]-------------------------------*/

/* Check if two ranges overlap (_a0->_a1 is first range ) */
#define OVERLAP(_a0,_a1, _b0, _b1 ) ( ( ((_a0) <= (_b0)) && ((_b0) <= (_a1)) ) || \
									  ( ((_b0) <= (_a0)) && ((_a0) <= (_b1)) ) )

/* ----------------------------[private typedef]-----------------------------*/
/* ----------------------------[private function prototypes]-----------------*/
/* ----------------------------[private variables]---------------------------*/

/* ----------------------------[private functions]---------------------------*/
/* ----------------------------[public functions]----------------------------*/

/**
 * Convert address to strange block format that freescale likes.
 *
 * @param addr
 * @param size
 * @param fb
 * @return
 */
static bool getAffectedBlocks( const FlashType *bPtr, uintptr_t addr, size_t size,	uint32 (*fb)[ADDR_SPACE_CNT] ) {
	uint16 addrSpace;
	bool anyAffected = false;


	memset(fb, 0, sizeof(*fb) );

	/* Check if sector range overlaps */
	for (int sector = 0; sector < bPtr->sectCnt; sector++)
	{
		if (OVERLAP( addr,addr+size-1,
				bPtr->sectAddr[sector],bPtr->sectAddr[sector+1]-1))
		{
			addrSpace = bPtr->addrSpace[sector];
			(*fb)[ADDR_SPACE_GET(addrSpace)] |= (1 << ADDR_SPACE_GET_SECTOR(addrSpace));
			anyAffected = true;
		}
	}
	return anyAffected;
}


/**
 * Setup the flash
 */

void Flash_Init(void) {

	/* IMPROVEMENT: Does freescale setup the platform flash controller with sane
	 * values, or not?
	 */
}

void Flash_Lock(const FlashType *fPtr, uint32 op, Fls_AddressType from, uint32 size) {
	uint32 flashBlocks[ADDR_SPACE_CNT];
	int bank;
	const FlashType *bPtr;
	uint32 regAddr;
	uint32 lock;

	for (bank = 0; bank < FLASH_BANK_CNT; bank++) {
		bPtr = &fPtr[bank];

		getAffectedBlocks(bPtr, from, size, &flashBlocks);

		/* ---------- Low/Mid ---------- */
	    lock = (flashBlocks[ADDR_SPACE_LOW]<<16) | flashBlocks[ADDR_SPACE_MID];
	    if( lock != 0 ) {
			regAddr = bPtr->regBase + C55FL_LML;

#if 0
			/* Unlock LML (enable LME bit) */
			if ( (READ32(regAddr) & NVLML_LME) == 0 ) {
				WRITE32(regAddr,PASSWORD_LOW_MID);
			}
#endif
			/* lock/unlock */
			if( op & FLASH_OP_UNLOCK ) {
				WRITE32(regAddr,(~lock) & READ32(regAddr) );
			} else {
				WRITE32(regAddr,(lock) | READ32(regAddr) );
			}

#if 0
			regAddr = bPtr->regBase + C55FL_SLL;
			/* Unlock secondary, SLL (enable LME bit) */
			if ( (READ32(regAddr) & NVLML_LME) == 0 ) {
				WRITE32(regAddr,PASSWORD_SECONDARY_LOW_MID);
			}
			/* lock/unlock */
			if( op & FLASH_OP_UNLOCK ) {
				WRITE32(regAddr,(~lock) & READ32(regAddr) );
			} else {
				WRITE32(regAddr,(lock) | READ32(regAddr) );
			}
#endif

	    }

		/* ---------- high ----------*/
	    lock = flashBlocks[ADDR_SPACE_HIGH];
	    if( lock != 0 ) {
	    	regAddr = bPtr->regBase + C55FL_HBL;
#if 0
	    	/* Unlock LML (enable LME bit) */
			if ( (READ32(regAddr) & NVLML_LME) == 0 ) {
				WRITE32(regAddr,PASSWORD_HIGH);
			}
#endif
			/* lock/unlock */
			if( op & FLASH_OP_UNLOCK ) {
				WRITE32(regAddr,(~lock) & READ32(regAddr) );
			} else {
				WRITE32(regAddr,(lock) | READ32(regAddr) );
			}
	    }
        /* ---------- large ----------*/
        lock = flashBlocks[ADDR_SPACE_LARGE];
        if( lock != 0 ) {
            regAddr = bPtr->regBase + C55FL_LBL;
            /* lock/unlock */
            if( op & FLASH_OP_UNLOCK ) {
                WRITE32(regAddr,(~lock) & READ32(regAddr) );
            } else {
                WRITE32(regAddr,(lock) | READ32(regAddr) );
            }
        }
	}

}

/**
 *
 * @param fPtr
 * @param dest
 * @param size
 * @return
 */
uint32 Flash_Erase( const FlashType *fPtr, uintptr_t dest, uint32 size, flashCbType cb) {
	uint32 flashBlocks[ADDR_SPACE_CNT];
	const FlashType *bPtr;
	bool affected;
	(void)cb;

	/* FSL functions are for each bank, so loop over banks */
	for (int bank = 0; bank < FLASH_BANK_CNT; bank++) {
		bPtr = &fPtr[bank];

		affected = getAffectedBlocks(bPtr, dest, size, &flashBlocks);
		if( affected == false ) {
			continue;
		}

	    /* check the high voltage operation*/
	    if (READ32(bPtr->regBase) & (C55FL_MCR_ERS | C55FL_MCR_PGM))
	    {
	        /* if any P/E operation in progress, return error*/
	        return EE_ERROR_PE_OPT;
	    }
        /* Set MCR ERS bit*/
        SET32(bPtr->regBase, C55FL_MCR_ERS);

        /* prepare low enabled blocks*/
//        flashBlocks[0] &= 0x0000FFFF;
        flashBlocks[0] = flashBlocks[0] << 16;

        /* prepare middle enabled blocks*/
//        flashBlocks[1] &= 0xF;

        /* prepare high enabled blocks*/
 //       flashBlocks[2] &= 0x0FFFFFFF;

        /* write the block selection registers*/
        WRITE32 ((bPtr->regBase + C55FL_LMS), (flashBlocks[0] | flashBlocks[1]));
        WRITE32 ((bPtr->regBase + C55FL_HBS), flashBlocks[2]);
        WRITE32 ((bPtr->regBase + C55FL_LBS), flashBlocks[3]);

        /* Interlock write*/
        WRITE32(dest, 0xFFFFFFFF);

        /* Set MCR EHV bit*/
        SET32(bPtr->regBase, C55FL_MCR_EHV);
	}
	return EE_OK;
}


/**
 *
 * @param to     Flash address to write. Updated to next address to write for each call
 * @param from   RAM address to read from. Updated with next address to read from for each call
 * @param size   The size to program. Updated for each for each call.
 * @return
 */
uint32 Flash_ProgramPageStart( const FlashType *fPtr, uint32 *to, uint32 * from, uint32 * size, flashCbType cb) {
    const FlashType *bPtr;
    bool affected;
    (void)cb;
    uint32 bytesToWrite;                    /* bytes to write in this operation */

    /* Check double word alignment */
    ASSERT((*size % FLASH_PAGE_SIZE) == 0 && (*to % FLASH_PAGE_SIZE) == 0);

    /* loop over banks */
    for (int bank = 0; bank < FLASH_BANK_CNT; bank++) {
        bPtr = &fPtr[bank];

        affected = OVERLAP( *to,
        		*to + *size - 1,
        		bPtr->sectAddr[0],
        		bPtr->sectAddr[0] + bPtr->bankSize-1);
        if( affected == false ) {
            /* This bank was not affected */
            continue;
        }

        /* Anything to program?*/
        if ( *size == 0 )
        {
            return (EE_OK);
        }

        /* check the high voltage operation*/
        if (READ32(bPtr->regBase) & (C55FL_MCR_ERS | C55FL_MCR_PGM))
        {
            /* if any P/E operation in progress, return error*/
            return (EE_INFO_HVOP_INPROGRESS);
        }

        /* calculate the size to be programmed within the page boundary*/
        bytesToWrite = *to;
        bytesToWrite = ((bytesToWrite / FLS_FLASH_WRITE_BUFFER_SIZE) + 1) * FLS_FLASH_WRITE_BUFFER_SIZE - bytesToWrite;
        if(*size < bytesToWrite) {
            bytesToWrite = *size;
            *size = 0;
        } else {
            *size -= bytesToWrite;
        }

        /* Set MCR PGM bit*/
        SET32(bPtr->regBase, C55FL_MCR_PGM);

        /* Program data */
        for (uint32 i = 0; i < bytesToWrite; i+=sizeof(uint32))
        {
            // read data bytewise to handle unaligned read buffer
            uint32 buf;
            memcpy(&buf, (uint8*)*from, sizeof(uint32));
            *from+=sizeof(uint32);
            /* Programming interlock write*/
            WRITE32(*to, buf);
            *to += sizeof(uint32);
        }

        /* Set MCR EHV bit*/
        SET32(bPtr->regBase, C55FL_MCR_EHV);
    }

    return EE_OK;
}

/**
 * Check status of programming/erase
 *
 * @param fPtr
 * @param to
 * @return
 */
uint32 Flash_CheckStatus( const FlashType *fPtr, const uint32 *to, uint32 size ) {
    const FlashType *bPtr;
    bool affected;
    uint32 rv = EE_OK;
    uintptr_t flAddr = (uintptr_t)to;

    for (int bank = 0; bank < FLASH_BANK_CNT && rv == E_OK; bank++) {
        bPtr = &fPtr[bank];

        /* We only need to figure out what bank is used, note that multiple banks
         * must be handled at the same time here */
        affected = OVERLAP(flAddr,flAddr+size-1,bPtr->sectAddr[0],bPtr->sectAddr[0] + bPtr->bankSize-1);
        if( affected == false ) {
            /* This bank was not affected */
            continue;
        }

        /* Check if MCR DONE is set*/
        if (READ32(bPtr->regBase) & C55FL_MCR_DONE)
        {
            /* check the operation status*/
            if(!(READ32(bPtr->regBase) & C55FL_MCR_PEG))
            {
                /* high voltage operation failed*/
                rv = EE_ERROR_PE_OPT;
            }

            /* end the high voltage operation*/
            CLEAR32(bPtr->regBase, C55FL_MCR_EHV);

            /* check for program operation*/
            if (READ32(bPtr->regBase) & C55FL_MCR_PGM)
            {
                /* finish the program operation*/
                CLEAR32(bPtr->regBase, C55FL_MCR_PGM);
            }
            else
            {
                /* finish the erase operation*/
                CLEAR32(bPtr->regBase, C55FL_MCR_ERS);
            }

        } else {
            rv = EE_INFO_HVOP_INPROGRESS;
        }
    }

    return rv;
}


