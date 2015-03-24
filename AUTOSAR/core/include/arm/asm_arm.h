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


#ifndef ASM_ARM_H_
#define ASM_ARM_H

#if defined(__GNUC__)
#define ASM_WORD(_x)	.word _x
#define ASM_EXTERN(_x)	.extern _x
#define ASM_GLOBAL(_x)  .global _x
#define ASM_TYPE(_x,_y)	.type	_x, _y
#define ASM_LABEL(_x)	_x:
#elif defined(__ARMCC_VERSION)
#define ASM_WORD(_x)	_x DCD 1
#define ASM_EXTERN(_x)	IMPORT _x
#define ASM_GLOBAL(_x)  GLOBAL _x
#define ASM_TYPE(_x,_y)
#define ASM_LABEL(_x)	_x
#endif


#if defined(_ASSEMBLER_)

/* Use as:
 * ASM_SECTION_TEXT(.text) - For normal .text or .text_vle
 */

#if defined(__GNUC__) || defined(__ghs__)
#  if defined(__ghs__) && defined(CFG_VLE)
#    define ASM_SECTION_TEXT(_x) 	.section .vletext,"vax"
#    define ASM_SECTION(_x)  		.section #_x,"vax"
#    define ASM_CODE_DIRECTIVE()    .vle
#  else
#  define ASM_SECTION_TEXT(_x) 	.section #_x,"ax"
#  define ASM_SECTION(_x)  		.section #_x,"ax"
#  endif
#elif defined(__CWCC__)
#  if defined(CFG_VLE)
#    define ASM_SECTION_TEXT(_x) .section _x,text_vle
#  else
#    define ASM_SECTION_TEXT(_x) .section _x,4,"rw"
#  endif
#  define ASM_SECTION(_x)		.section _x,4,"r"
#elif defined(__DCC__)
#  if defined(CFG_VLE)
#    define ASM_SECTION_TEXT(_x) .section _x,4,"x"
#  else
#    define ASM_SECTION_TEXT(_x) .section _x,4,"x"
#  endif
#  define ASM_SECTION(_x)		.section _x,4,"r"
#endif

#ifndef ASM_CODE_DIRECTIVE
#define ASM_CODE_DIRECTIVE()
#endif



#endif /* _ASSEMBLER_ */

#endif /*ASM_ARM_H_*/

