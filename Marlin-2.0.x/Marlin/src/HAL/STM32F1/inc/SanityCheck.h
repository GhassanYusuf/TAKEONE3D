/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * Test STM32F1-specific configuration values for errors at compile-time.
 */

#if ENABLED(EMERGENCY_PARSER)
  #error "EMERGENCY_PARSER is not yet implemented for STM32F1. Disable EMERGENCY_PARSER to continue."
#endif

#if ENABLED(SDIO_SUPPORT) && DISABLED(SDSUPPORT)
  #error "SDIO_SUPPORT requires SDSUPPORT. Enable SDSUPPORT to continue."
#endif

#if ENABLED(FAST_PWM_FAN)
  #error "FAST_PWM_FAN is not yet implemented for this platform."
#endif

<<<<<<< HEAD:Marlin-2.0.x/Marlin/src/HAL/STM32F1/inc/SanityCheck.h
#if !defined(HAVE_SW_SERIAL) && HAS_TMC_SW_SERIAL
=======
#if !defined(HAVE_SW_SERIAL) && TMC_HAS_SW_SERIAL
>>>>>>> 2b7ac9ca62c71088824dd1eb57906e58d42de222:Marlin-2.0.x/Marlin/src/HAL/HAL_STM32F1/inc/SanityCheck.h
  #warning "With TMC2208/9 consider using SoftwareSerialM with HAVE_SW_SERIAL and appropriate SS_TIMER."
  #error "Missing SoftwareSerial implementation."
#endif
