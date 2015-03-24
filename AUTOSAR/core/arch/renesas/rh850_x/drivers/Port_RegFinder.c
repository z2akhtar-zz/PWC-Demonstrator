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

#include "Port_RegFinder.h"
#include "dr7f701503.dvf.h"
#if defined(USE_DET)
#include "Det.h"
#endif

typedef enum {
    PORTTYPE_P,
    PORTTYPE_JP,
    PORTTYPE_AP,
    PORTTYPE_IP,
    PORTTYPE_NOT_CONFIGURABLE
}ArcPort_PortType;

typedef struct {
    ArcPort_PortType    portType;
    uint8       portNr;
    uint8       bit;
} PinIdToPortTranslation;

PinIdToPortTranslation PinIdToPort[]={
        /*NOPIN*/ {PORTTYPE_NOT_CONFIGURABLE, 0, 0}, /*The PIN list starts at 1 */
        /* PIN1 */  { PORTTYPE_P, 10, 3 },
        /* PIN2 */  { PORTTYPE_P, 10, 4 },
        /* PIN3 */  { PORTTYPE_P, 10, 5 },
        /* PIN4 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN5 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN6 */  { PORTTYPE_P, 10, 15 },
        /* PIN7 */  { PORTTYPE_P, 11, 0 },
        /* PIN8 */  { PORTTYPE_P, 11, 8 },
        /* PIN9 */  { PORTTYPE_P, 11, 9 },
        /* PIN10 */  { PORTTYPE_P, 11, 10 },
        /* PIN11 */  { PORTTYPE_P, 11, 11 },
        /* PIN12 */  { PORTTYPE_P, 11, 12 },
        /* PIN13 */  { PORTTYPE_P, 11, 13 },
        /* PIN14 */  { PORTTYPE_P, 11, 14 },
        /* PIN15 */  { PORTTYPE_P, 12, 3 },
        /* PIN16 */  { PORTTYPE_P, 12, 4 },
        /* PIN17 */  { PORTTYPE_P, 12, 5 },
        /* PIN18 */  { PORTTYPE_P, 0, 0 },
        /* PIN19 */  { PORTTYPE_P, 0, 1 },
        /* PIN20 */  { PORTTYPE_P, 0, 2 },
        /* PIN21 */  { PORTTYPE_P, 0, 3 },
        /* PIN22 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN23 */  { PORTTYPE_P, 0, 4 },
        /* PIN24 */  { PORTTYPE_P, 0, 5 },
        /* PIN25 */  { PORTTYPE_P, 0, 6 },
        /* PIN26 */  { PORTTYPE_P, 0, 11 },
        /* PIN27 */  { PORTTYPE_P, 0, 12 },
        /* PIN28 */  { PORTTYPE_P, 0, 13 },
        /* PIN29 */  { PORTTYPE_P, 0, 14 },
        /* PIN30 */  { PORTTYPE_P, 1, 0 },
        /* PIN31 */  { PORTTYPE_P, 1, 1 },
        /* PIN32 */  { PORTTYPE_P, 1, 2 },
        /* PIN33 */  { PORTTYPE_P, 1, 3 },
        /* PIN34 */  { PORTTYPE_P, 1, 12 },
        /* PIN35 */  { PORTTYPE_P, 1, 13 },
        /* PIN36 */  { PORTTYPE_P, 2, 6 },
        /* PIN37 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN38 */  { PORTTYPE_P, 8, 2 },
        /* PIN39 */  { PORTTYPE_P, 8, 10 },
        /* PIN40 */  { PORTTYPE_P, 8, 11 },
        /* PIN41 */  { PORTTYPE_P, 8, 12 },
        /* PIN42 */  { PORTTYPE_JP, 0, 5 },
        /* PIN43 */  { PORTTYPE_JP, 0, 4 },
        /* PIN44 */  { PORTTYPE_JP, 0, 3 },
        /* PIN45 */  { PORTTYPE_JP, 0, 2 },
        /* PIN46 */  { PORTTYPE_JP, 0, 1 },
        /* PIN47 */  { PORTTYPE_JP, 0, 0 },
        /* PIN48 */  { PORTTYPE_P, 2, 1 },
        /* PIN49 */  { PORTTYPE_P, 2, 0 },
        /* PIN50 */  { PORTTYPE_P, 1, 11 },
        /* PIN51 */  { PORTTYPE_P, 1, 10 },
        /* PIN52 */  { PORTTYPE_P, 1, 9 },
        /* PIN53 */  { PORTTYPE_P, 1, 8 },
        /* PIN54 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN55 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN56 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN57 */  { PORTTYPE_IP, 0, 0 },
        /* PIN58 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN59 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN60 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN61 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN62 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN63 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN64 */  { PORTTYPE_P, 2, 3 },
        /* PIN65 */  { PORTTYPE_P, 2, 2 },
        /* PIN66 */  { PORTTYPE_JP, 0, 6 },
        /* PIN67 */  { PORTTYPE_P, 0, 10 },
        /* PIN68 */  { PORTTYPE_P, 0, 9 },
        /* PIN69 */  { PORTTYPE_P, 0, 8 },
        /* PIN70 */  { PORTTYPE_P, 0, 7 },
        /* PIN71 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN72 */  { PORTTYPE_P, 1, 7 },
        /* PIN73 */  { PORTTYPE_P, 1, 6 },
        /* PIN74 */  { PORTTYPE_P, 1, 5 },
        /* PIN75 */  { PORTTYPE_P, 1, 4 },
        /* PIN76 */  { PORTTYPE_P, 2, 4 },
        /* PIN77 */  { PORTTYPE_P, 2, 5 },
        /* PIN78 */  { PORTTYPE_P, 1, 14 },
        /* PIN79 */  { PORTTYPE_P, 1, 15 },
        /* PIN80 */  { PORTTYPE_P, 8, 0 },
        /* PIN81 */  { PORTTYPE_P, 8, 1 },
        /* PIN82 */  { PORTTYPE_P, 8, 3 },
        /* PIN83 */  { PORTTYPE_P, 8, 4 },
        /* PIN84 */  { PORTTYPE_P, 8, 5 },
        /* PIN85 */  { PORTTYPE_P, 8, 6 },
        /* PIN86 */  { PORTTYPE_P, 8, 7 },
        /* PIN87 */  { PORTTYPE_P, 8, 8 },
        /* PIN88 */  { PORTTYPE_P, 8, 9 },
        /* PIN89 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN90 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN91 */  { PORTTYPE_AP, 0, 15 },
        /* PIN92 */  { PORTTYPE_AP, 0, 14 },
        /* PIN93 */  { PORTTYPE_AP, 0, 13 },
        /* PIN94 */  { PORTTYPE_AP, 0, 12 },
        /* PIN95 */  { PORTTYPE_AP, 0, 11 },
        /* PIN96 */  { PORTTYPE_AP, 0, 10 },
        /* PIN97 */  { PORTTYPE_AP, 0, 9 },
        /* PIN98 */  { PORTTYPE_AP, 0, 8 },
        /* PIN99 */  { PORTTYPE_AP, 0, 7 },
        /* PIN100 */  { PORTTYPE_AP, 0, 6 },
        /* PIN101 */  { PORTTYPE_AP, 0, 5 },
        /* PIN102 */  { PORTTYPE_AP, 0, 4 },
        /* PIN103 */  { PORTTYPE_AP, 0, 3 },
        /* PIN104 */  { PORTTYPE_AP, 0, 2 },
        /* PIN105 */  { PORTTYPE_AP, 0, 1 },
        /* PIN106 */  { PORTTYPE_AP, 0, 0 },
        /* PIN107 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN108 */  { PORTTYPE_P, 9, 0 },
        /* PIN109 */  { PORTTYPE_P, 9, 1 },
        /* PIN110 */  { PORTTYPE_P, 9, 2 },
        /* PIN111 */  { PORTTYPE_P, 9, 3 },
        /* PIN112 */  { PORTTYPE_P, 9, 4 },
        /* PIN113 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN114 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN115 */  { PORTTYPE_P, 20, 3 },
        /* PIN116 */  { PORTTYPE_P, 20, 2 },
        /* PIN117 */  { PORTTYPE_P, 20, 1 },
        /* PIN118 */  { PORTTYPE_P, 20, 0 },
        /* PIN119 */  { PORTTYPE_P, 20, 5 },
        /* PIN120 */  { PORTTYPE_P, 20, 4 },
        /* PIN121 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN122 */  { PORTTYPE_AP, 1, 11 },
        /* PIN123 */  { PORTTYPE_AP, 1, 10 },
        /* PIN124 */  { PORTTYPE_AP, 1, 9 },
        /* PIN125 */  { PORTTYPE_AP, 1, 8 },
        /* PIN126 */  { PORTTYPE_AP, 1, 7 },
        /* PIN127 */  { PORTTYPE_AP, 1, 6 },
        /* PIN128 */  { PORTTYPE_AP, 1, 5 },
        /* PIN129 */  { PORTTYPE_AP, 1, 4 },
        /* PIN130 */  { PORTTYPE_AP, 1, 3 },
        /* PIN131 */  { PORTTYPE_AP, 1, 2 },
        /* PIN132 */  { PORTTYPE_AP, 1, 1 },
        /* PIN133 */  { PORTTYPE_AP, 1, 0 },
        /* PIN134 */  { PORTTYPE_AP, 1, 15 },
        /* PIN135 */  { PORTTYPE_AP, 1, 14 },
        /* PIN136 */  { PORTTYPE_AP, 1, 13 },
        /* PIN137 */  { PORTTYPE_AP, 1, 12 },
        /* PIN138 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN139 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN140 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN141 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN142 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN143 */  { PORTTYPE_P, 18, 0 },
        /* PIN144 */  { PORTTYPE_P, 18, 1 },
        /* PIN145 */  { PORTTYPE_P, 18, 2 },
        /* PIN146 */  { PORTTYPE_P, 18, 3 },
        /* PIN147 */  { PORTTYPE_P, 18, 4 },
        /* PIN148 */  { PORTTYPE_P, 18, 5 },
        /* PIN149 */  { PORTTYPE_P, 18, 6 },
        /* PIN150 */  { PORTTYPE_P, 18, 7 },
        /* PIN151 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN152 */  { PORTTYPE_P, 10, 6 },
        /* PIN153 */  { PORTTYPE_P, 10, 7 },
        /* PIN154 */  { PORTTYPE_P, 10, 8 },
        /* PIN155 */  { PORTTYPE_P, 10, 9 },
        /* PIN156 */  { PORTTYPE_P, 10, 10 },
        /* PIN157 */  { PORTTYPE_P, 10, 11 },
        /* PIN158 */  { PORTTYPE_P, 10, 12 },
        /* PIN159 */  { PORTTYPE_P, 10, 13 },
        /* PIN160 */  { PORTTYPE_P, 10, 14 },
        /* PIN161 */  { PORTTYPE_P, 11, 1 },
        /* PIN162 */  { PORTTYPE_P, 11, 2 },
        /* PIN163 */  { PORTTYPE_P, 11, 3 },
        /* PIN164 */  { PORTTYPE_P, 11, 4 },
        /* PIN165 */  { PORTTYPE_P, 11, 5 },
        /* PIN166 */  { PORTTYPE_P, 11, 6 },
        /* PIN167 */  { PORTTYPE_P, 11, 7 },
        /* PIN168 */  { PORTTYPE_P, 11, 15 },
        /* PIN169 */  { PORTTYPE_P, 12, 0 },
        /* PIN170 */  { PORTTYPE_P, 12, 1 },
        /* PIN171 */  { PORTTYPE_P, 12, 2 },
        /* PIN172 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN173 */  { PORTTYPE_NOT_CONFIGURABLE, 0, 0 },
        /* PIN174 */  { PORTTYPE_P, 10, 0 },
        /* PIN175 */  { PORTTYPE_P, 10, 1 },
        /* PIN176 */  { PORTTYPE_P, 10, 2 }
};

typedef struct {
    volatile uint16  *P;
    volatile uint16  *AP;
    volatile uint8   *JP;
    volatile uint16  *IP;
} PortTypes;

PortTypes regs[_NO_OF_REGISTERSELECTION_] = {
        /*PMC    */{  &PMC0,       0,  &JPMC0,       0},
        /*PIPC   */{ &PIPC0,       0,       0,       0},
        /*PM     */{   &PM0,   &APM0,   &JPM0,       0},
        /*PIBC   */{ &PIBC0, &APIBC0, &JPIBC0, &IPIBC0},
        /*PFC    */{  &PFC0,       0,  &JPFC0,       0},
        /*PFCE   */{ &PFCE0,       0,       0,       0},
        /*PFCAE  */{&PFCAE0,       0,       0,       0},
        /*PBDC   */{ &PBDC0, &APBDC0, &JPBDC0,       0},
        /*PPR    */{  &PPR0,  &APPR0,  &JPPR0,  &IPPR0},
        /*P      */{    &P0,    &AP0,    &JP0,       0},
        /*PU     */{   &PU0,       0,   &JPU0,       0},
        /*PD     */{   &PD0,       0,   &JPD0,       0},
        /*PDSC   */{ (volatile uint16 *)0xFFC14600,       0,       0,       0},
        /*PODC   */{ (volatile uint16 *)0xFFC14500,       0, (volatile uint8 *)0xFFC20450,       0},
        /*PIS    */{  &PIS0,       0,       0,       0}
};

volatile uint16 * Port_Arc_getRegPointerFromPinId(Port_Arc_RegisterSelectionType regType, uint16 pinId, uint16 ModuleId, uint8 ApiId, uint8 ErrorId){
    volatile uint16 * retVal=0;
    if( regType < _NO_OF_REGISTERSELECTION_){
        switch (PinIdToPort[pinId].portType) {
            case PORTTYPE_P:
                retVal = (volatile uint16 *)(((volatile unsigned char *)regs[regType].P)+(PinIdToPort[pinId].portNr*4));
                break;
            case PORTTYPE_AP:
                retVal = (volatile uint16 *)(((volatile unsigned char *)regs[regType].AP)+(PinIdToPort[pinId].portNr*4));
                break;
            case PORTTYPE_JP:
                retVal = (volatile uint16 *)regs[regType].JP;
                break;
            case PORTTYPE_IP:
                retVal = (volatile uint16 *)regs[regType].IP;
                break;
            default:
#if defined(USE_DET)
                Det_ReportError(ModuleId, 0, ApiId, ErrorId );
#endif
                break;
        }
    }
    return retVal;
}

volatile uint16 * Port_Arc_getRegPointerFromPort(Port_Arc_RegisterSelectionType regType, Port_Arc_PortGroups portGroup, uint16 ModuleId, uint8 ApiId, uint8 ErrorId){
    volatile uint16 * retVal=0;

    static PinIdToPortTranslation Dio_PortTypeToRH850Port[]={
            { PORTTYPE_P, 0, 0 },
            { PORTTYPE_P, 1, 0 },
            { PORTTYPE_P, 2, 0 },
            { PORTTYPE_P, 8, 0 },
            { PORTTYPE_P, 9, 0 },
            { PORTTYPE_P, 10, 0 },
            { PORTTYPE_P, 11, 0 },
            { PORTTYPE_P, 12, 0 },
            { PORTTYPE_P, 18, 0 },
            { PORTTYPE_P, 20, 0 },
            { PORTTYPE_JP, 0, 0 },
            { PORTTYPE_AP, 0, 0 },
            { PORTTYPE_AP, 1, 0 },
            { PORTTYPE_IP, 0, 0 }
    };

    if( portGroup < _NR_OF_PORTGROUPS_){
        switch (Dio_PortTypeToRH850Port[portGroup].portType) {
            case PORTTYPE_P:
                retVal = (volatile uint16 *)(((volatile unsigned char *)regs[regType].P)+(Dio_PortTypeToRH850Port[portGroup].portNr*4));
                break;
            case PORTTYPE_AP:
                retVal = (volatile uint16 *)(((volatile unsigned char *)regs[regType].AP)+(Dio_PortTypeToRH850Port[portGroup].portNr*4));
                break;
            case PORTTYPE_JP:
                retVal = (volatile uint16 *)regs[regType].JP;
                break;
            case PORTTYPE_IP:
                retVal = (volatile uint16 *)regs[regType].IP;
                break;
            default:
#if defined(USE_DET)
                Det_ReportError(ModuleId, 0, ApiId, ErrorId );
#endif
                break;
        }
    }
    return retVal;
}

void Port_Arc_setPin(uint16 pinId, volatile uint16 *reg, uint8 flag) {

    uint16 regVal = (0x0001 << PinIdToPort[pinId].bit);
    if (flag > 0) {
        *reg |= regVal;
    }
    else {
        *reg &= ~regVal;
    }

}

uint8 Port_Arc_getPin(uint16 pinId, volatile uint16 *reg) {
    uint16 retVal = 0;
    if ((0x0001 << PinIdToPort[pinId].bit) & (*reg)) {
        retVal = 1;
    }
    return retVal;
}
