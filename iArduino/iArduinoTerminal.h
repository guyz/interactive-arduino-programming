// A C interpreter for Arduino
// Copyright(c) 2012 Noriaki Mitsunaga.  All rights reserved.
//
// This is a free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// See file LICENSE.txt for further informations on licensing terms.
//

#ifndef __IARDUINO_TERMINAL_H__
#define __IARDUINO_TERMINAL_H__

#define IAR_REPORT_PROTOVER  0x0
#define IAR_REPORT_VARIABLES 0x1
#define IAR_REPORT_OTHERS    0x4
#define IAR_REPORT_PROG_LIST 0xd
#define IAR_REPORT_PROG_POS  0xe
#define IAR_REPORT_TIME      0xf
#define IAR_SET_PINMODE   0x10
#define IAR_SET_DIGITAL   0x11
#define IAR_SET_PWM       0x12
#define IAR_SET_SERVO     0x13
#define IAR_ATTACH_SERVO  0x14

#define IAR_STX           0x7f  // Start of command or response
   // Protocol version
#define IAR_PROTOVER_MEGA328v1      0x0
#define IAR_PROTOVER_LEONARDv1      0x1

#endif
