/*==================================================================================================
*   Project              : RTD AUTOSAR 4.7
*   Platform             : CORTEXM
*   Peripheral           : LPI2C
*   Dependencies         : none
*
*   Autosar Version      : 4.7.0
*   Autosar Revision     : ASR_REL_4_7_REV_0000
*   Autosar Conf.Variant :
*   SW Version           : 4.0.0
*   Build Version        : S32K3_RTD_4_0_0_P19_D2403_ASR_REL_4_7_REV_0000_20240315
*
*  Copyright 2020 - 2024 NXP
*
*   NXP Confidential. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms. By expressly
*   accepting such terms or by downloading, installing, activating and/or otherwise
*   using the software, you are agreeing that you have read, and that you agree to
*   comply with and are bound by, such license terms. If you do not agree to be
*   bound by the applicable license terms, then you may not retain, install,
*   activate or otherwise use the software.
==================================================================================================*/

/**
*   @file
*
*   @addtogroup I2C_DRIVER_CONFIGURATION I2c Driver Configurations
*   @{
*/

#ifndef CDD_I2C_CFG_H
#define CDD_I2C_CFG_H

#ifdef __cplusplus
extern "C"
{
#endif


/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "CDD_I2c_VS_0_PBcfg.h"

/*==================================================================================================
*                              SOURCE FILE VERSION INFORMATION
==================================================================================================*/
#define I2C_VENDOR_ID_CFG                    43
#define I2C_MODULE_ID_CFG                    255
#define I2C_AR_RELEASE_MAJOR_VERSION_CFG     4
#define I2C_AR_RELEASE_MINOR_VERSION_CFG     7
#define I2C_AR_RELEASE_REVISION_VERSION_CFG  0
#define I2C_SW_MAJOR_VERSION_CFG             4
#define I2C_SW_MINOR_VERSION_CFG             0
#define I2C_SW_PATCH_VERSION_CFG             0

/*==================================================================================================
*                                     FILE VERSION CHECKS
==================================================================================================*/

/* Check if current file and CDD_I2c_VS_0_PBcfg.h header file are of the same module */
#if (I2C_MODULE_ID_CFG != I2C_VS_0_PBCFG_MODULE_ID)
    #error "CDD_I2c_Cfg.h and CDD_I2c_VS_0_PBcfg.h have different module ids"
#endif
/* Check if current file and CDD_I2c_VS_0_PBcfg.h header file are of the same vendor */
#if (I2C_VENDOR_ID_CFG != I2C_VS_0_PBCFG_VENDOR_ID)
    #error "CDD_I2c_Cfg.h and CDD_I2c_VS_0_PBcfg.h have different vendor ids"
#endif
/* Check if current file and CDD_I2c_VS_0_PBcfg.h header file are of the same Autosar version */
#if ((I2C_AR_RELEASE_MAJOR_VERSION_CFG    != I2C_VS_0_PBCFG_AR_RELEASE_MAJOR_VERSION) || \
     (I2C_AR_RELEASE_MINOR_VERSION_CFG    != I2C_VS_0_PBCFG_AR_RELEASE_MINOR_VERSION) || \
     (I2C_AR_RELEASE_REVISION_VERSION_CFG != I2C_VS_0_PBCFG_AR_RELEASE_REVISION_VERSION) \
    )
    #error "AutoSar Version Numbers of CDD_I2c_Cfg.h and CDD_I2c_VS_0_PBcfg.h are different"
#endif
/* Check if current file and CDD_I2c_VS_0_PBcfg.h header file are of the same Software version */
#if ((I2C_SW_MAJOR_VERSION_CFG != I2C_VS_0_PBCFG_SW_MAJOR_VERSION) || \
     (I2C_SW_MINOR_VERSION_CFG != I2C_VS_0_PBCFG_SW_MINOR_VERSION) || \
     (I2C_SW_PATCH_VERSION_CFG != I2C_VS_0_PBCFG_SW_PATCH_VERSION)    \
    )
    #error "Software Version Numbers of CDD_I2c_Cfg.h and CDD_I2c_VS_0_PBcfg.h are different"
#endif

/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/






/*==================================================================================================
*                                              ENUMS
==================================================================================================*/


/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/


/*==================================================================================================
*                                  GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
*                                       FUNCTION PROTOTYPES
==================================================================================================*/


#ifdef __cplusplus
}
#endif

/** @} */

#endif /* CDD_I2C_CFG_H */

