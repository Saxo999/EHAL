
/* This file was automatically generated by nrfutil on 2016-12-09 (YY-MM-DD) at 10:25:52 */

#include "stdint.h"
#include "compiler_abstraction.h"

/* This file was generated with a throwaway private key, that is only inteded for a debug version of the DFU project.
  Please see https://github.com/NordicSemiconductor/pc-nrfutil/blob/master/README.md to generate a valid public key. */

#ifdef NRF_DFU_DEBUG_VERSION 

/** @brief Public key used to verify DFU images */
__ALIGN(4) const uint8_t pk[64] =
{
    0xe7, 0xea, 0x85, 0xfc, 0x97, 0xf3, 0x38, 0xb8, 0x33, 0x57, 0x7e, 0xab, 0xe7, 0xff, 0xc8, 0xc5, 0xd8, 0xd4, 0xdb, 0xa8, 0x2c, 0x5d, 0xaa, 0x10, 0x83, 0x6e, 0xc2, 0xc5, 0xaf, 0xe4, 0x1f, 0xdb, 
    0xf7, 0x57, 0x7a, 0x81, 0xd3, 0xe5, 0x74, 0x3d, 0xb8, 0xc4, 0x6c, 0x02, 0x2f, 0x7c, 0xd7, 0x13, 0x00, 0x0d, 0xb2, 0xb1, 0x2c, 0x4d, 0xd1, 0xc9, 0x31, 0xe8, 0x15, 0xb6, 0xf4, 0xa0, 0x8f, 0x9c
};

#else
#error "Debug public key not valid for production. Please see https://github.com/NordicSemiconductor/pc-nrfutil/blob/master/README.md to generate it"
#endif