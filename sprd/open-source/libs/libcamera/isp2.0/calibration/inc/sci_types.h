/******************************************************************************
 ** File Name:      sci_types.h                                               *
 ** Author:         jakle zhu                                                 *
 ** DATE:           10/22/2001                                                *
 ** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This header file contains general types and macros that   *
 **         		are of use to all modules.The values or definitions are   *
 ** 				dependent on the specified target.  T_WINNT specifies     *
 **					Windows NT based targets, otherwise the default is for    *
 **					ARM targets.                                              *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 10/22/2001     Jakle zhu     Create.                                      *
 ******************************************************************************/
#ifndef SCI_TYPES_H
#define SCI_TYPES_H

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    extern   "C"
    {
#endif

/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */

#ifndef NULL
#define NULL  0
#endif

#define SCI_NULL                                 0
#ifndef PNULL
#define PNULL                                    0
#endif
/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */
typedef unsigned char		BOOLEAN;
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned long int	uint32;
//typedef unsigned __int64	uint64;
typedef unsigned int		uint;

typedef signed char		int8;
typedef signed short		int16;
typedef signed long int		int32;

typedef unsigned char		boolean;
typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long int	uint32_t;
//typedef unsigned __int64	uint64_t;
typedef unsigned int		uint_t;

typedef signed char		int8_t;
typedef signed short		int16_t;
typedef signed long int		int32_t;

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    }
#endif

#endif  /* SCI_TYPES_H */


