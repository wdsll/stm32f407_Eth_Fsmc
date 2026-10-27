/* =============================================================================
 * Minimal sys/types.h stub for Keil MDK (ARM Compiler 6)
 *
 * Harmony 3's toolchain_specifics.h does  #include <sys/types.h>
 * The ARM C library shipped with armclang does not provide this POSIX header.
 * The Harmony sources include it but consume nothing from it, so an empty
 * stub is sufficient. Keil project adds  .\include  to the include path.
 * (XC32/MPLAB build keeps using its real sys/types.h - sources untouched.)
 * =============================================================================
 */
#ifndef KEIL_SYS_TYPES_H_STUB
#define KEIL_SYS_TYPES_H_STUB

#include <stddef.h>

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef long int ssize_t;
#endif

#endif /* KEIL_SYS_TYPES_H_STUB */
