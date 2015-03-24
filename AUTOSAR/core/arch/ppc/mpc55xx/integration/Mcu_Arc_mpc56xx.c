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

/* ----------------------------[includes]------------------------------------*/

#include <assert.h>
#include "Std_Types.h"
#include "Mcu.h"
#include "io.h"
#include "mpc55xx.h"
#include "Mcu_Arc.h"
#if defined(USE_FEE)
#include "Fee_Memory_Cfg.h"
#endif
#if defined(USE_DMA)
#include "Dma.h"
#endif
#include "asm_ppc.h"
#include "Os.h"
#include "EcuM.h"
#include "isr.h"

#include <string.h>

/*lint -e923 Cast in Freescale header file */
/* ----------------------------[private define]------------------------------*/
/* ----------------------------[private macro]-------------------------------*/

#define ME_RUN_PC0      0
#define ME_RUN_PC1      1

#define MODE_DRUN       3u
#define MODE_RUN0       4u
#define MODE_STANDBY    13u

/* ----------------------------[private typedef]-----------------------------*/
/* ----------------------------[private function prototypes]-----------------*/
/* ----------------------------[private variables]---------------------------*/

#if defined(USE_FLS)
/*lint -esym(752, EccErrReg) */
extern uint32 EccErrReg;
#endif


/* ----------------------------[private functions]---------------------------*/
/* ----------------------------[public functions]----------------------------*/



void Mcu_Arc_InitPre( const Mcu_ConfigType *configPtr ) {
	/*lint -e{920} General interface, the configPtr may be needed for other architectures */
    (void)configPtr;
#if defined(CFG_MPC5744P) || defined(CFG_E200Z4D) || defined(CFG_MPC5777M)
	Cache_EnableI();
#if !defined(USE_TTY_WINIDEA)
// cannot use debug terminal with D-cache enabled
	Cache_EnableD();
#endif
#endif
}

void Mcu_Arc_InitPost( const Mcu_ConfigType *configPtr ) {
    /*lint -e{920} General interface, the configPtr may be needed for other architectures */
	(void)configPtr;
}

#if defined(CFG_MPC5643L) || defined(CFG_MPC5645S) || defined(CFG_SPC56XL70) || defined(CFG_MPC5607B)
static void switchMode( uint32 mode ) {
    /* Mode Transition to enter RUN0 mode: */
     /* Enter RUN0 Mode & Key */
     ME.MCTL.R = 0x00005AF0 | (mode << 28uL); /*lint !e9027 Setting upper four bits, alternative does not make sense */
     /* Enter RUN0 Mode & Inverted Key */
     ME.MCTL.R = 0x0000A50F | (mode << 28uL); /*lint !e9027 Setting upper four bits, alternative does not make sense*/

#if !defined(CFG_SIMULATOR)
     /* Wait for mode transition to complete */
     while (ME.GS.B.S_MTRANS != 0u) {}
     /* Verify RUN0 is the current mode */
     while(ME.GS.B.S_CURRENTMODE != mode) {}
#endif
}
#endif


#if defined(CFG_MCU_ARC_LP)

static void enterStandby( void ) {
    /* ME_PCTL_x MUST be off */

    /* Configure STANDBY0 Mode for lowest consumption (only WUP Unit ON) */
    /* Please note, WKPU (Wakeup Unit) is always enabled */
    /* To generate an interrupt event triggered by a wakeup line, it is */
    /* necessary to enable WKPU */

    RGM.STDBY.B.BOOT_FROM_BKP_RAM = 1;
    ME.MER.B.STANDBY = 1;
#if defined(CFG_MPC560X)
    PCU.PCONF[2].B.STBY0 = 1;       /* Enable 32K RAM in stdby */
#elif defined(CFG_MPC5645S)
    /* PD0 is always on and that holds first 8K of SRAM */
#else
#error Not supported
#endif

    /* Turn off ALL ME_PCTL
     * All ME_PCTL point to LP config ME_LP_PC0 (LP_CFG=0)
     */
    ME.LPPC[0].R = 0;       /* Freeze peripherals */
    ME.RUNPC[0].R = 0xfe;
    ME.RUNPC[1].R = 0xfe;


    /* Errata e3247, must write FRZ to all CAN devices. How do I know
     * what ones to disable?
     * */
    CAN_0.MCR.B.FRZ = 1;
    CAN_1.MCR.B.FRZ = 1;
    CAN_2.MCR.B.FRZ = 1;
    CAN_0.MCR.B.MDIS = 1;
    CAN_1.MCR.B.MDIS = 1;
    CAN_2.MCR.B.MDIS = 1;

     switchMode(MODE_STANDBY);

    /* From "MC_ME Mode Diagram", page 136 in RM,  it seems as e return to DRUN after exit of STANDBY0 */
}
#endif


void Mcu_Arc_InitClockPre( const Mcu_ClockSettingConfigType *clockSettingsPtr )
{
#if defined(CFG_MPC5604B) || defined(CFG_MPC5606B)
    // Write pll parameters.
    CGM.FMPLL_CR.B.IDF = clockSettingsPtr->Pll1;
    CGM.FMPLL_CR.B.NDIV = clockSettingsPtr->Pll2;
    CGM.FMPLL_CR.B.ODF = clockSettingsPtr->Pll3;

    /* RUN0 cfg: 16MHzIRCON,OSC0ON,PLL0ON,syclk=PLL0 */
    ME.RUN[0].R = 0x001F0074uL;
    /* Peri. Cfg. 1 settings: only run in RUN0 mode */
    ME.RUNPC[1].R = 0x00000010uL;
    /* MPC56xxB/S: select ME.RUNPC[1] */
    ME.PCTL[69].R = 0x01;  //WKUP
    ME.PCTL[68].R = 0x01u; //SIUL control
    ME.PCTL[91].R = 0x01u; //RTC/API control
    ME.PCTL[92].R = 0x01u; //PIT_RTI control
    ME.PCTL[72].R = 0x01u; //eMIOS0 control
    ME.PCTL[73].R = 0x01u; //eMIOS1 control
    ME.PCTL[16].R = 0x01u; //FlexCAN0 control
    ME.PCTL[17].R = 0x01u; //FlexCAN1 control
    ME.PCTL[18].R = 0x01u; //FlexCAN2 control
    ME.PCTL[19].R = 0x01u; //FlexCAN3 control
    ME.PCTL[20].R = 0x01u; //FlexCAN4 control
    ME.PCTL[21].R = 0x01u; //FlexCAN5 control
    ME.PCTL[4].R = 0x01u;  /* MPC56xxB/P/S DSPI0  */
    ME.PCTL[5].R = 0x01u;  /* MPC56xxB/P/S DSPI1:  */
    ME.PCTL[32].R = 0x01u; //ADC0 control
#if defined(CFG_MPC5606B)
    ME.PCTL[33].R = 0x01u; //ADC1 control
#endif
    ME.PCTL[23].R = 0x01u; //DMAMUX control
    ME.PCTL[48].R = 0x01u; /* MPC56xxB/P/S LINFlex 0  */
    ME.PCTL[49].R = 0x01u; /* MPC56xxB/P/S LINFlex 1  */
    ME.PCTL[50].R = 0x01u; /* MPC56xxB/P/S LINFlex 2  */
    ME.PCTL[51].R = 0x01u; /* MPC56xxB/P/S LINFlex 3  */
    /* Mode Transition to enter RUN0 mode: */
    /* Enter RUN0 Mode & Key */
    ME.MCTL.R = 0x40005AF0uL;
    /* Enter RUN0 Mode & Inverted Key */
    ME.MCTL.R = 0x4000A50FuL;

    /* Wait for mode transition to complete */
    while (ME.GS.B.S_MTRANS != 0u) {}
    /* Verify RUN0 is the current mode */
    while(ME.GS.B.S_CURRENTMODE != 4u) {}

    CGM.SC_DC[0].R = 0x80u; /* MPC56xxB/S: Enable peri set 1 sysclk divided by 1 */
    CGM.SC_DC[1].R = 0x80u; /* MPC56xxB/S: Enable peri set 2 sysclk divided by 1 */
    CGM.SC_DC[2].R = 0x80u; /* MPC56xxB/S: Enable peri set 3 sysclk divided by 1 */

#elif defined(CFG_MPC5606S)
    // Write pll parameters.
    CGM.FMPLL[0].CR.B.IDF = clockSettingsPtr->Pll1;
    CGM.FMPLL[0].CR.B.NDIV = clockSettingsPtr->Pll2;
    CGM.FMPLL[0].CR.B.ODF = clockSettingsPtr->Pll3;

    /* RUN0 cfg: 16MHzIRCON,OSC0ON,PLL0ON,syclk=PLL0 */
    ME.RUN[0].R = 0x001F0074;
    /* Peri. Cfg. 1 settings: only run in RUN0 mode */
    ME.RUNPC[1].R = 0x00000010;
    /* MPC56xxB/S: select ME.RUNPC[1] */
    ME.PCTL[68].R = 0x01; //SIUL control
    ME.PCTL[91].R = 0x01; //RTC/API control
    ME.PCTL[92].R = 0x01; //PIT_RTI control
    ME.PCTL[72].R = 0x01; //eMIOS0 control
    ME.PCTL[73].R = 0x01; //eMIOS1 control
    ME.PCTL[16].R = 0x01; //FlexCAN0 control
    ME.PCTL[17].R = 0x01; //FlexCAN1 control
    ME.PCTL[4].R = 0x01;  /* MPC56xxB/P/S DSPI0  */
    ME.PCTL[5].R = 0x01;  /* MPC56xxB/P/S DSPI1:  */
    ME.PCTL[32].R = 0x01; //ADC0 control
    ME.PCTL[23].R = 0x01; //DMAMUX control
    ME.PCTL[48].R = 0x01; /* MPC56xxB/P/S LINFlex  */
    ME.PCTL[49].R = 0x01; /* MPC56xxB/P/S LINFlex  */
    /* Mode Transition to enter RUN0 mode: */
    /* Enter RUN0 Mode & Key */
    ME.MCTL.R = 0x40005AF0;
    /* Enter RUN0 Mode & Inverted Key */
    ME.MCTL.R = 0x4000A50F;

    /* Wait for mode transition to complete */
    while (ME.GS.B.S_MTRANS) {}
    /* Verify RUN0 is the current mode */
    while(ME.GS.B.S_CURRENTMODE != 4) {}

    CGM.SC_DC[0].R = 0x80; /* MPC56xxB/S: Enable peri set 1 sysclk divided by 1 */
    CGM.SC_DC[1].R = 0x80; /* MPC56xxB/S: Enable peri set 2 sysclk divided by 1 */
    CGM.SC_DC[2].R = 0x80; /* MPC56xxB/S: Enable peri set 3 sysclk divided by 1 */

#elif defined(CFG_MPC5604P)
    // Write pll parameters.
    CGM.FMPLL[0].CR.B.IDF = clockSettingsPtr->Pll1;
    CGM.FMPLL[0].CR.B.NDIV = clockSettingsPtr->Pll2;
    CGM.FMPLL[0].CR.B.ODF = clockSettingsPtr->Pll3;
    // PLL1 must be higher than 120MHz for PWM to work */
    CGM.FMPLL[1].CR.B.IDF = clockSettingsPtr->Pll1_1;
    CGM.FMPLL[1].CR.B.NDIV = clockSettingsPtr->Pll2_1;
    CGM.FMPLL[1].CR.B.ODF = clockSettingsPtr->Pll3_1;

    /* RUN0 cfg: 16MHzIRCON,OSC0ON,PLL0ON, PLL1ON,syclk=PLL0 */
  	ME.RUN[0].R = 0x001F00F4;
    /* Peri. Cfg. 1 settings: only run in RUN0 mode */
    ME.RUNPC[1].R = 0x00000010;

    /* MPC56xxB/S: select ME.RUNPC[1] */
    ME.PCTL[68].R = 0x01; //SIUL control
    ME.PCTL[92].R = 0x01; //PIT_RTI control
    ME.PCTL[41].R = 0x01; //flexpwm0 control
    ME.PCTL[16].R = 0x01; //FlexCAN0 control
    ME.PCTL[26].R = 0x01; //FlexCAN1(SafetyPort) control
    ME.PCTL[4].R = 0x01;  /* MPC56xxB/P/S DSPI0  */
    ME.PCTL[5].R = 0x01;  /* MPC56xxB/P/S DSPI1:  */
    ME.PCTL[6].R = 0x01;  /* MPC56xxB/P/S DSPI2  */
    ME.PCTL[7].R = 0x01;  /* MPC56xxB/P/S DSPI3:  */
    ME.PCTL[32].R = 0x01; //ADC0 control
    ME.PCTL[33].R = 0x01; //ADC1 control
    ME.PCTL[48].R = 0x01; /* MPC56xxB/P/S LINFlex  */
    ME.PCTL[49].R = 0x01; /* MPC56xxB/P/S LINFlex  */
    /* Mode Transition to enter RUN0 mode: */
    /* Enter RUN0 Mode & Key */
    ME.MCTL.R = 0x40005AF0;
    /* Enter RUN0 Mode & Inverted Key */
    ME.MCTL.R = 0x4000A50F;

    /* Wait for mode transition to complete */
    while (ME.GS.B.S_MTRANS) {}
    /* Verify RUN0 is the current mode */
    while(ME.GS.B.S_CURRENTMODE != 4) {}

    /* Pwm, adc, etimer clock */
    CGM.AC0SC.R = 0x05000000uL;  /* MPC56xxP: Select FMPLL1 for aux clk 0  */
    CGM.AC0DC.R = 0x80000000uL;  /* MPC56xxP: Enable aux clk 0 div by 1 */

    /* Safety port clock */
    CGM.AC2SC.R = 0x04000000uL;  /* MPC56xxP: Select FMPLL0 for aux clk 2  */
    CGM.AC2DC.R = 0x80000000uL;  /* MPC56xxP: Enable aux clk 2 div by 1 */
#elif defined(CFG_MPC5744P)
    // enable run pheriperal 0 in all modes
    MC_ME.RUN_PC[0].R = 0xFFFFFFFFuL;
    if(SIUL2.MIDR.B.PARTNUM == 0x5744 && SIUL2.MIDR.B.MAJOR_MASK == 1 && SIUL2.MIDR.B.MINOR_MASK == 0) {
        // always issue a short reset on external reset without any self test. This is a workaround for errata e7218 on rev2.0 mask
        // of the mpc5744P (marked 0N65H)
        MC_RGM.FESS.B.SS_EXR = 1u;
    }
    // setup clock source to xosc for PLL0
    MC_CGM.AC3_SC.B.SELCTL = 1u;
    // setup clock source tp PHI1 for PLL1
    MC_CGM.AC4_SC.B.SELCTL = 3u;

#elif defined(CFG_MPC5643L) || defined(CFG_SPC56XL70)

    /* For some reason we are in SAFE mode from start
     * -> Need to go through DRUN to get to RUN 0
     * */
    uint32 runPC = ME_RUN_PC1;
    uint32 mode = MODE_DRUN;

    /* pll1 - IDF,  pll2 - NDIV , pll3 - odf */
   assert(clockSettingsPtr->Pll1 <= 15);
   assert(clockSettingsPtr->Pll2 >=32 && clockSettingsPtr->Pll2 <= 96);
   assert(clockSettingsPtr->Pll3 <= 3);

   uint32  extal = clockSettingsPtr->McuClockReferencePointFrequency;

   PLLD0.CR.B.NDIV = clockSettingsPtr->Pll2;
   PLLD0.CR.B.IDF = clockSettingsPtr->Pll1;
   PLLD0.CR.B.ODF = clockSettingsPtr->Pll3;
   PLLD0.CR.B.EN_PLL_SW = 1;                    /* Enable progressive clock switching */

   /* Fvco = CLKIN * NDIV / IDF */
#if 0
   /* Specification ambigous, vcoFreq not currently used */
   uint32 vcoFreq = extal * clockSettingsPtr->Pll2 / (clockSettingsPtr->Pll1+1);
   assert(vcoFreq >= 256000000 && vcoFreq <= 512000000);
#endif

   /* Calulation
    * phi = xosc * ldf / (idf * odf )
    *
    */
   uint32 sysFreq = ( (extal)*(clockSettingsPtr->Pll2) / ((clockSettingsPtr->Pll1+1)*(2<<(clockSettingsPtr->Pll3))) );
   assert(sysFreq <= 120000000);


   uint32 val = ME.RUN0_MC.R;

   ME.RUN0_MC.R = ( val |= 0x30000);     /* FLAON=3 Normal mode */
   ME.RUN0_MC.R = ( val |= 0x20); /* XOSC on */
   ME.RUN0_MC.R = ( val |= 0x40); /* PLL0 on */
   ME.RUN0_MC.R = ( val |= 0x04); /* SysClk = 4; */

   /* Enable RUN0 mode in Peri config 1 */
   ME.RUN_PC1.R = 0x00000010;

   // setup clock source to xosc for PLL0
   CGM.AC3_SC.B.SELCTL = 1;

   /* Setup sources for aux clocks */
   CGM.AC0_SC.B.SELCTL = 4; // use system FMPLL as source for Aux clock 0 (motor control clock, SWG clock)
#if 0
   CGM.AC1_SC.B.SELCTL = 4; // use system FMPLL as source for Aux clock 1 (Flexray clock)
#endif
   CGM.AC2_SC.B.SELCTL = 4; // use system FMPLL as source for Aux clock 2 (FlexCAN clock)

   runPC = ME_RUN_PC1;
   mode = MODE_RUN0;

   ME.PCTL[92].R = runPC; //PIT_RTI control

   ME.PCTL[4].R = runPC; //Dspi0 control
   ME.PCTL[5].R = runPC; //Dspi1 control
   ME.PCTL[6].R = runPC; //Dspi2 control

   ME.PCTL[16].R = runPC; //FlexCAN0 control
   ME.PCTL[17].R = runPC; //FlexCAN1 control
#if defined(CFG_SPC56XL70)
   ME.PCTL[18].R = runPC; //FlexCAN2 control
#endif
   ME.PCTL[32].R = 0x01; //ADC0 control
   ME.PCTL[33].R = 0x01; //ADC1 control
   ME.PCTL[41].R = 0x01; //FlexPWM0 control
   ME.PCTL[42].R = 0x01; //FlexPWM1 control
   ME.PCTL[48].R = 0x01; //LINFlex0 control
   ME.PCTL[49].R = 0x01; //LINFlex0 control

   switchMode(mode);
#elif defined(CFG_MPC5645S)
   uint8 runPC = ME_RUN_PC1;
   uint32 mode = MODE_DRUN;

   /* pll1 - IDF,  pll2 - NDIV , pll3 - odf */
     assert(clockSettingsPtr->Pll1 <= 15);
     assert((clockSettingsPtr->Pll2 >=32) && (clockSettingsPtr->Pll2 <= 96));
     assert(clockSettingsPtr->Pll3 <= 3);

#if 0
     uint32  extal = clockSettingsPtr->McuClockReferencePointFrequency;

      /* Specification ambigous, vcoFreq not currently used */
      /* Fvco = CLKIN * NDIV / IDF */
      uint32 vcoFreq = extal * clockSettingsPtr->Pll2 / clockSettingsPtr->Pll1;
      assert(vcoFreq >= 256000000 && vcoFreq <= 512000000);
#endif
      // Write pll parameters.
      CGM.FMPLL[0].CR.B.IDF = clockSettingsPtr->Pll1;
      CGM.FMPLL[0].CR.B.NDIV = clockSettingsPtr->Pll2;
      CGM.FMPLL[0].CR.B.ODF = clockSettingsPtr->Pll3;


      /* Turn of things for RUN0 mode */
      /*lint -e{970,24,40,10,63,446,1058,1514,9018) Seems lint can't handle typeof */
      const typeof(ME.RUN[0]) val = {
              .B.PDO=1,
              .B.MVRON=1,
              .B.FLAON=3,
              .B.FXOSCON=1,
              .B.FMPLL0ON=1,
              .B.FIRCON=1,
              .B.SYSCLK=4      /* System FMPLL as clock */
      };

      /*lint -e{10,40) Seems lint can't handle typeof */
      ME.RUN[0].R = val.R;

      /* Enable RUN0 mode in Peri config 1 */
      /*lint -e{10,40) Seems lint can't handle typeof */
      ME.RUNPC[1].R = 0x00000010;

      runPC = ME_RUN_PC1;
      mode = MODE_RUN0;

      ME.PCTL[16].R = runPC; //FlexCAN0 control
      ME.PCTL[17].R = runPC; //FlexCAN1 control
      ME.PCTL[18].R = runPC; //FlexCAN2 control

      ME.PCTL[23].R = runPC; //DMA-CH MUX
      ME.PCTL[32].R = runPC; //ADC0 control

      ME.PCTL[48].R = runPC; //LINFlex0 control
      ME.PCTL[49].R = runPC; //LINFlex1 control
      ME.PCTL[50].R = runPC; //LINFlex2 control
      ME.PCTL[51].R = runPC; //LINFlex3 control

      ME.PCTL[5].R = runPC;  // DSPI1 Control
      ME.PCTL[66].R = runPC; // CFLASH0 Control
      ME.PCTL[68].R = runPC; // SIUL control

      ME.PCTL[72].R = runPC; // eMIOS0
      ME.PCTL[73].R = runPC; // eMIOS1

      ME.PCTL[91].R = runPC; //RTC
      ME.PCTL[92].R = runPC; //PIT_RTI control


     switchMode(mode);

#endif
     /*lint -e{920} General interface, the clockSettingsPtr may be needed for other architectures */
    (void)clockSettingsPtr; // get rid of compiler warning

}

#if defined(CFG_MPC5777M)
// freescale provided code for setting upp clocks, de appnote
//http://cache.freescale.com/files/microcontrollers/doc/app_note/AN4812.pdf?fpsp=1&WT_TYPE=Application%20Notes&WT_VENDOR=FREESCALE&WT_FILE_FORMAT=pdf&WT_ASSET=Documentation&Parent_nodeId=1351098580720722626559&Parent_pageType=product
/*!****************************************************************************/
/*! MPC5777M Matterhorn PLL and Clock Configuration */
/*! Configure the Mode and Clock Tree */
/*! Note: For MPC5777M cut 1, code must run from Core 2 local Memory, */
/*! it cannot run from System RAM */
/*!****************************************************************************/
void MC_MODE_INIT(void){
    //int i;
    /*! 1 Clear any faults */
    /*! Clear faults | MC_RGM.DES, MC_RGM.FES, and MC_ME.ME */
    MC_RGM.DES.R = 0xFFFF;
    MC_RGM.FES.R = 0xFFFF;
    MC_ME.ME.R = 0x000005FF;
    /*! 2 Set up peripheral run modes */
    /*! Enable the modes Required | MC_ME.ME */
    MC_ME.ME.R = 0x0000800F;
    /*! Add MC_ME.PCTL[x].R initializations here */
    /*! Set RUN Configuration Registers | MC_ME.RUN_PC[n] */
    MC_ME.RUN_PC[0].R=0x000000FE; /* Peripheral ON in every mode */
    MC_ME.RUN_PC[1].R=0x000000FE; /* Peripheral ON in every mode */
    MC_ME.RUN_PC[2].R=0x000000FE; /* Peripheral ON in every mode */
    MC_ME.RUN_PC[3].R=0x000000FE; /* Peripheral ON in every mode */
    MC_ME.RUN_PC[4].R=0x000000FE; /* Peripheral ON in every mode */
    MC_ME.RUN_PC[5].R=0x000000FE; /* Peripheral ON in every mode */
    MC_ME.RUN_PC[6].R=0x000000FE; /* Peripheral ON in every mode */
    MC_ME.RUN_PC[7].R=0x000000FE; /* Peripheral ON in every mode */
    /*! 3 Configure System Clock Dividers */
    /*! Configure System clock dividers */
    /*! Full speed Core 0 / 1 = 300 MHz. PLL1 = 600 MHz. */

    MC_CGM.SC_DIV_RC.R = 0x00000001uL; /* System clock divider ratios will */
    /* change with next update. */
    /* Not required for Cut 1. */
    MC_CGM.DIV_UPD_TYPE.R = 0x80000000uL; /* System clock divider ratios updated */
    /* on writing MC_CGM.DIV_UPD_TRIG.
    */ /* Not required for Cut 1. */
    MC_CGM.SC_DC2.R=0x800B0000uL; /*! PBRIDGE Clock Divide by 12 (50 MHz) */
    MC_CGM.SC_DC1.R=0x80050000uL; /*! SXBAR Clock Divide by 6 (100 MHz) */
    MC_CGM.SC_DC0.R=0x80020000uL; /*! FXBAR Clock Divide by 3 (200 MHz) */
    MC_CGM.SC_DC3.R=0x80010000uL; /*! Core0/1 Clock Divide by 2 (300 MHz) */
    MC_CGM.SC_DC4.R=0x80020000uL; /*! System Clock Divide by 3 (200 MHz) */
    MC_CGM.DIV_UPD_TRIG.R = 0xfeedfaceuL; /*! System clock divider ratio updates */
    /* triggered. Not required for Cut 1. */
    while (MC_CGM.DIV_UPD_STAT.B.SYS_UPD_STAT == 1u){} /*! Wait for System Clock */
    /* Div Update Status == 0. */
    /* Not required for Cut 1. */
    /*! 4 Configure System Clock Dividers */

    /*! Enable and configure Aux clocks */
    MC_CGM.AC0_SC.B.SELCTL=2u; // set PLL0 PHI for Aux Clock 0

    MC_CGM.AC0_DC0.R=0x80040000uL; // program Aux Clock 0 divider 0
    // peripheral clock -> Divide by = 4 + 1
    // 400 MHz/5 = 80 MHz

    MC_CGM.AC0_DC1.R=0x80180000uL; // program Aux Clock 0 divider 1
    // SDADC clock -> Divide by 24 + 1.
    // 400 MHz / 25 = 16 MHz
    MC_CGM.AC0_DC2.R=0x801B0000uL; // program Aux Clock 0 divider 2
    // SARADC clock -> Divide by 24 + 1
    // 400 MHz / 28 = 14.6 MHz
    MC_CGM.AC0_DC3.R=0x80030000uL; // program Aux Clock 0 divider 3
    // DSPI_CLK0 -> Divide by 3 + 1
    //400 MHz / 4 = 100 MHz
    MC_CGM.AC0_DC4.R=0x80030000uL; // program Aux Clock 0 divider 4
    // DSPI_CLK1/LIN_CLK -> Divide by 3 + 1
    // 400 MHz / 4 = 100 MHz
    MC_CGM.AC2_DC0.R=0x80090000uL; // program Aux Clock 2 divider 0
    // FlexRay -> Divide by 9 + 1
    // 400 MHz / 10 = 40 MHz
    MC_CGM.AC2_DC1.R=0x80090000uL; // program Aux Clock 2 divider 1
    // SENT -> Divide by 9 + 1
    // 400 MHz / 10 = 40 MHz
    MC_CGM.AC5_DC0.R=0x80090000uL; // program Aux Clock 5 divider 0
    // PSI5 -> Divide by 9 + 1
    // 400 MHz / 10 = 40 MHz
    MC_CGM.AC5_DC1.R=0x80090000uL; // program Aux Clock 5 divider 1
    // PSI5 -> Divide by 9 + 1
    // 400 MHz / 10 = 40 MHz
    MC_CGM.AC5_DC2.R=0x80090000uL; // program Aux Clock 5 divider 2
    // PSI5 -> Divide by 9 + 1
    // 400 MHz / 10 = 40 MHz
    /* CAN Clock Runs from XOSC by Default */
    MC_CGM.AC8_DC0.R=0x80070000uL; // program Aux Clock 8 divider 0
    // CAN Clock-> Divide by 8
    MC_CGM.AC9_SC.B.SELCTL=1; // Select XOSC for Aux Clock 9
    MC_CGM.AC9_DC0.R=0x80030000uL; // program Aux Clock 8 divider 0
    // RTI/PIT-> Divide by 4
	
	MC_CGM.AC10_SC.B.SELCTL=2u;   // Select PLL0 PHI for Aux Clock 10
    MC_CGM.AC10_DC0.R=0x800F0000uL; // program Aux Clock 10 divider 0	
    // ENET -> Divide by 15 + 1
    // 400 MHz / 16 = 25 MHz

    while ( MC_CGM.AC10_SS.B.SELSTAT != 2u);


    /* Ref_clk output for Phy if needed */
    MC_CGM.AC7_SC.B.SELCTL=2u;             /*slect PLL0*/
    MC_CGM.AC7_DC0.R = 0x800F0000uL;        /*400MHz/16  = 25MHz use sysclk1 op to PHY*/
    while ( MC_CGM.AC7_SS.B.SELSTAT != MC_CGM.AC7_SC.B.SELCTL);
    /*! Set the PRAMC Flow Through disable */
    /*! SRAM requires additional wait state */
    /*! Note: Do not change the FT_DIS bit while accessing System RAM.
    Relocate code programming the FT_DIS bit to another memory area
    (e.g. local Core memory). */
    /*! Also, set the FT_DIS bit after programming clock dividers and before
    setting PLLs and executing the Mode Entry change */
    PRAMC.PRCR1.B.FT_DIS = 1u; /*! Set Flow Through Disable. */

    /*! Step 5 --- CONFIGURE X0SC PLL0 PLL1 --- */
    /*! Route XOSC to the PLLs - IRC is default */
    MC_CGM.AC3_SC.B.SELCTL=1u; /*! Connect XOSC to PLL0 */
    MC_CGM.AC4_SC.B.SELCTL=1u; /*! Connect XOSC to PLL1 */

    /*! Configure PLL0 Dividers - 400 MHz from 40 MHz XOSC */
    PLLDIG.PLL0DV.B.RFDPHI = 1u;
    PLLDIG.PLL0DV.B.PREDIV = 4u;
    PLLDIG.PLL0DV.B.MFD = 40u;
    //! fPLL0_VCO = (fpll0_ref * 2 * MFD) / PREDIV
    //! fPLL0_VCO = (40 MHz * 2 * 40) / 4
    //! fPLL0_VCO = 800 MHz
    //!
    //! fPLL0_PHI = (fpll0_ref * 2 * MFD) / (PREDIV * RFDPHI * 2)
    //! fPLL0_PHI = (40 MHz * 2 * 40) / (4 * 1 * 2)
    //! fPLL0_PHI = 400 MHz

    /*! Put PLL0 into Normal mode */
    PLLDIG.PLL0CR.B.CLKCFG = 3u;
    /*! Configure PLL1 Dividers - 600 MHz from 40 MHz XOSC */
    PLLDIG.PLL1DV.B.RFDPHI = 1u;
    PLLDIG.PLL1DV.B.MFD = 30u;
    //! fPLL1_VCO = (fpll1_ref * MFD)
    //! fPLL1_VCO = (40 MHz * 30)
    //! fPLL1_VCO = 1200 MHz
    //!
    //! fPLL1_PHI = (fpll0_ref * MFD) / (RFDPHI * 2)
    //! fPLL1_PHI = (40 MHz * 30) / (1 * 2)
    //! fPLL1_PHI = 600 MHz

    /*! Put PLL1 into Normal mode */
    PLLDIG.PLL1CR.B.CLKCFG = 3u;
    /*! 6 CONFIGURE PROGRESSIVE CLOCK SWITCHING (PCS), Configure Progressive Clock Switching
    (PCS) to prevent glitches - 0.05 rate 70 steps. */
    MC_CGM.PCS_SDUR.R = 100u; /*! set Switch Duration */
    MC_ME.DRUN_MC.B.PWRLVL=3u; /*! Configure DRUN power level */
    /*! Configure PLL1 PCS switch | See RM section "Progressive system clock switching" */
    MC_CGM.PCS_DIVC4.B.INIT = 851u; /*! Set the Divider Change Initial Value */
    MC_CGM.PCS_DIVC4.B.RATE = 12u; /*! Set the Divider Change Rate */
    MC_CGM.PCS_DIVS4.R = 31671u; /*! Set the Divider Start Value. */
    MC_CGM.PCS_DIVE4.R = 31671u; /*! Set the Divider End Value */
    /* Configure PLL0 PCS switch (See RM section Progressive system clock switching) */
    MC_CGM.PCS_DIVC2.B.INIT = 851; /*! Set the Divider Change Initial Value */
    MC_CGM.PCS_DIVC2.B.RATE = 12; /*! Set the Divider Change Rate */
    MC_CGM.PCS_DIVS2.R = 31671; /*! Set the Divider Start Value */
    MC_CGM.PCS_DIVE2.R = 31671; /*! Set the Divider End Value */

#if 0
    /*! 7 ----- Initialize e200z Cores ----- */
    /* Enable cores if running from RAM and not using the BAF */
    /*! Enable Cores - Will start on next mode transition */
    /*! If core n is enabled, then */
    /*! - Set MC_ME.CADDR[n] to the code start address (see linker file) */
    /*! - Set MC_ME.CCTL[n] to enable core in all modes */
    MC_ME.CCTL0.R = 0x00FE;
    /* RAM addresses */
    MC_ME.CADDR1.R = 0x40010001; /* Set Core 0 Start Address */
    MC_ME.CCTL1.R = 0x00FE; /* Set modes in which Core 0 will run. */
    MC_ME.CADDR2.R = 0x40010001; /* Set Checker Core Start Address */
    MC_ME.CCTL2.R = 0x00FE; /* Set modes in which Checker Core will run.*/
    MC_ME.CADDR3.R = 0x40012001; /* Set Core 1 Start Address */
    MC_ME.CCTL3.R = 0x00FE; /* Set modes in which Core 1 will run. */

    MC_ME.CADDR4.R = 0x40012001; /* Set HSM Start Address */
    MC_ME.CCTL4.R = 0x00FE; /* Set modes in which HSM will run. */
#endif

    /* 8 ----- Perform Mode Entry change ----- */
    /* Set the System Clock. Enable XOSC and PLLs - PLL1 is sysclk, PWRLVL = 3. */
    MC_ME.DRUN_MC.R = 0x301300F4;
    /* Execute mode change: */
    /* Re-enter the DRUN mode to start cores, clock tree dividers, PCS, and PLL1 */
    MC_ME.MCTL.R = 0x30005AF0; /*! Write Mode and Key */
    MC_ME.MCTL.R = 0x3000A50F; /*! Write Mode and Key inverted */
    while(MC_ME.GS.B.S_MTRANS == 1){} /*! Wait for mode entry complete */
    while(MC_ME.GS.B.S_CURRENTMODE != 0x3u){} /*! Check DRUN mode entered */


    while(MC_ME.GS.B.S_XOSC != 0x1u){} /*! Check if external oscillator is not stable */


}
#endif

void Mcu_Arc_InitClockPost( const Mcu_ClockSettingConfigType *clockSettingsPtr )
{
#if defined(CFG_MPC5777M)
    uint32  extal = Mcu_Arc_GetClockReferencePointFrequency();
    while(extal != 40000000){} // code is assuming 40MHz crystal
    // call function provided by freescale to setup clocks to max freq with 40MHz crystal
    MC_MODE_INIT();

#elif defined(CFG_MPC5744P)
    // setup peripheral bridge clock divider to generate max 50MHz
    uint32 sysFreq = Mcu_Arc_GetSystemClock();
    typeof(MC_CGM.SC_DC0.B) scDc0 = {
        .DE = 1,
        .DIV = 1 | ((sysFreq + 50000000 - 1) / 50000000 - 1), // max 50MHz, divider must be odd
    };
    MC_CGM.SC_DC0.B = scDc0; // register can only be accessed through full register write
#endif
#if defined(CFG_MPC5744P)// || defined(CFG_MPC5777M)
    // start xosc: configure run mode to have xosc enabled
    MC_ME.DRUN_MC.B.XOSCON = 1;
    // configure the new run mode
    MC_ME.DRUN_MC.B.PLL0ON = 1;
    // and select PLL0 as clock source
    MC_ME.DRUN_MC.B.SYSCLK = 2;
    // enter run mode
    typeof(MC_ME.MCTL.B) mode = {.KEY = 0x5AF0, .TARGET_MODE = 3};
    MC_ME.MCTL.B = mode;
    mode.KEY ^= 0xFFFF;
    MC_ME.MCTL.B = mode;
    // wait for mode transition to complete
    while(MC_ME.GS.B.S_MTRANS == 1){}
    // confirm mode change
    if(MC_ME.GS.B.S_CURRENTMODE != mode.TARGET_MODE) {
        // do something smart
    }
#if defined(USE_ADC) && defined(CFG_MPC5744P)
    // setup clock to ADC
    typeof(MC_CGM.AC0_DC2.B) ac0Dc2 = {
        .DE = 1,
        .DIV = (sysFreq + 80000000 - 1) / 80000000 - 1, // max 80MHz,
    };
    MC_CGM.AC0_DC2.B = ac0Dc2; // register can only be accessed through full register write
#elif defined(USE_ADC) && defined(CFG_MPC5777M)
    // setup clock to SARADC
    typeof(MC_CGM.AC0_DC2.B) ac0Dc2 = {
        .DE = 1,
        .DIV = (sysFreq + 16000000 - 1) / 16000000 - 1, // max 80MHz,
    };
    MC_CGM.AC0_DC2.B = ac0Dc2; // register can only be accessed through full register write
    // setup clock to SDADC
    typeof(MC_CGM.AC0_DC1.B) ac0Dc1 = {
        .DE = 1,
        .DIV = (sysFreq + 16000000 - 1) / 16000000 - 1, // max 80MHz,
    };
    MC_CGM.AC0_DC1.B = ac0Dc1; // register can only be accessed through full register write
#endif

#if defined(USE_PWM) && defined(CFG_MPC5744P)
    // setup MOTC clock
    typeof(MC_CGM.AC0_DC0.B) ac0Dc0 = {
        .DE = 1,
        .DIV = (sysFreq + 160000000 - 1) / 160000000 - 1, // max 160MHz,
    };
    MC_CGM.AC0_DC0.B = ac0Dc0; // register can only be accessed through full register write
#endif

#if defined(USE_ETH)
    // setup ENET clock
    typeof(MC_CGM.AC10_DC0.B) ac10Dc0 = {
        .DE = 1,
        .DIV = (sysFreq + 50000000 - 1) / 50000000 - 1, // max 50MHz,
    };
    MC_CGM.AC10_DC0.B = ac10Dc0; // register can only be accessed through full register write
    MC_CGM.AC10_SC.B.SELCTL = 2; // use PHI as source
    typeof(MC_CGM.AC11_DC0.B) ac11Dc0 = {
        .DE = 1,
        .DIV = (sysFreq + 50000000 - 1) / 50000000 - 1, // max 50MHz,
    };
    MC_CGM.AC11_DC0.B = ac11Dc0; // register can only be accessed through full register write
    MC_CGM.AC11_SC.B.SELCTL = 2; // use PHI as source
#endif

    MC_CGM.AC0_SC.B.SELCTL = 2; // use PHI as source

#elif defined(CFG_MPC5643L) || defined(CFG_SPC56XL70)
    uint32 sysFreq = Mcu_Arc_GetSystemClock();

    //Setup perihperal set 0 clock
    typeof(CGM.SC_DC0.B) sc0Dc2 = {
        .DE = 1,
        .DIV = (sysFreq + 120000000 - 1) / 120000000 - 1, // max 120MHz
    };
    CGM.SC_DC0.B = sc0Dc2; // register can only be accessed through full register write

#if defined(USE_PWM)
    // setup MOTC clock
    typeof(CGM.AC0_DC0.B) ac0Dc0 = {
        .DE = 1,
        .DIV = (sysFreq + 120000000 - 1) / 120000000 - 1, // max 120MHz
    };
    CGM.AC0_DC0.B = ac0Dc0; // register can only be accessed through full register write
#endif

#if defined(USE_CAN)
    // setup FlexCAN clock
    typeof(CGM.AC2_DC0.B) ac2Dc0 = {
        .DE = 1,
        .DIV = (sysFreq + 60000000 - 1) / 60000000 - 1, // max 60MHz
    };
    CGM.AC2_DC0.B = ac2Dc0; // register can only be accessed through full register write
#endif
#elif defined(CFG_MPC5645S)
#if defined(USE_PWM)
    // setup Aux clock 1 (Emios1)
    /*lint -e{970} Lint can't handle typeof */
    typeof(CGM.AC1_DC.B) ac1Dc0 = {
        .DE0 = 1,
        .DIV0 = 0,
    };
    CGM.AC1_DC.B = ac1Dc0; // register can only be accessed through full register write
    CGM.AC1_SC.B.SELCTL = 3; // use system FMPLL/2 as source for Aux clock 1

    // setup Aux clock 1 (Emios2)
    /*lint -e{970} Lint can't handle typeof */
    typeof(CGM.AC2_DC.B) ac2Dc = {
        .DE0 = 1,
        .DIV0 = 0,
    };
    CGM.AC2_DC.B = ac2Dc; // register can only be accessed through full register write
    CGM.AC2_SC.B.SELCTL = 3; // use system FMPLL/2 as source for Aux clock 2
#endif
#endif
    /*lint -e{920} General interface, the clockSettingsPtr may be needed for other architectures */
    (void)clockSettingsPtr;
}


#if defined(CFG_MCU_ARC_LP)

/*lint -esym(9003, __LP_TEXT_ROM, __LP_TEXT_START, __LP_TEXT_END, context) Needs to be global */
extern sint8 __LP_TEXT_ROM[];
extern sint8 __LP_TEXT_START[];
extern sint8 __LP_TEXT_END[];

/* Context save area */
__balign(8) uint8 context[(32+5)*4];

void Mcu_Arc_SetModePre(Mcu_ModeType mcuMode) {

    Mcu_ModeType localMcuMode = mcuMode; /* Variable needed for MISRA compliance */

#if defined(CFG_ECUM_VIRTUAL_SOURCES)
    EcuM_WakeupSourceType pendWakeup;
#endif

    if (localMcuMode == McuConf_McuModeSettingConf_NORMAL) {
        localMcuMode = McuConf_McuModeSettingConf_RUN;
    }

    if (McuConf_McuModeSettingConf_SLEEP == localMcuMode) {

        /* Copy LP recovery routines to RAM */
        memcpy(__LP_TEXT_START, __LP_TEXT_ROM, __LP_TEXT_END - __LP_TEXT_START); /*lint !e946 !e947 !e732 Simpler than alternative */

        /* Go to sleep */
        if (Mcu_Arc_setjmp(context) == 0) {
            enterStandby();
        }

        /* Back from sleep!
         * Now running in DRUN and on FIRC (16Mhz)
         */

        /* Wdg OFF */
        SWT.SR.R = 0x0000c520UL; /* Write keys to clear soft lock bit */
        SWT.SR.R = 0x0000d928UL;
        SWT.CR.R = 0x8000010AUL;

#if defined(USE_ECUM_FIXED) || defined(USE_ECUM_FLEXIBLE)
        EcuM_CheckWakeup( 0x3fffffffUL );
#endif


        /* Setup exceptions and INTC again */
        Os_IsrInit();

    } /* MCU_MODE_SLEEP == localMcuMode */


}

void Mcu_Arc_SetModePost( Mcu_ModeType mcuMode)
{
    (void)mcuMode;
}
#endif
