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

#include "../../inc/MarlinConfig.h"

#if HAS_PID_HEATING

#include "../gcode.h"
#include "../../module/temperature.h"

#if ENABLED(EXTENSIBLE_UI)
<<<<<<< HEAD:Marlin-2.0.x/Marlin/src/gcode/temp/M303.cpp
  #include "../../lcd/extui/ui_api.h"
=======
  #include "../../lcd/extensible_ui/ui_api.h"
>>>>>>> 2b7ac9ca62c71088824dd1eb57906e58d42de222:Marlin-2.0.x/Marlin/src/gcode/temperature/M303.cpp
#endif

/**
 * M303: PID relay autotune
 *
 *  S<temperature>  Set the target temperature. (Default: 150C / 70C)
 *  E<extruder>     Extruder number to tune, or -1 for the bed. (Default: E0)
 *  C<cycles>       Number of times to repeat the procedure. (Minimum: 3, Default: 5)
 *  U<bool>         Flag to apply the result to the current PID values
 *
 * With PID_DEBUG:
 *  D               Toggle PID debugging and EXIT without further action.
 */

#if ENABLED(PID_DEBUG)
  bool pid_debug_flag = 0;
#endif

void GcodeSuite::M303() {

  #if ENABLED(PID_DEBUG)
    if (parser.seen('D')) {
      pid_debug_flag = !pid_debug_flag;
      SERIAL_ECHO_START();
      SERIAL_ECHOPGM("PID Debug ");
      serialprintln_onoff(pid_debug_flag);
      return;
    }
  #endif

  #if ENABLED(PIDTEMPBED)
    #define SI H_BED
  #else
    #define SI H_E0
  #endif
  #if ENABLED(PIDTEMP)
    #define EI HOTENDS - 1
  #else
    #define EI H_BED
  #endif
  const heater_ind_t e = (heater_ind_t)parser.intval('E');
  if (!WITHIN(e, SI, EI)) {
<<<<<<< HEAD:Marlin-2.0.x/Marlin/src/gcode/temp/M303.cpp
    SERIAL_ECHOLNPGM(STR_PID_BAD_EXTRUDER_NUM);
    #if ENABLED(EXTENSIBLE_UI)
      ExtUI::onPidTuning(ExtUI::result_t::PID_BAD_EXTRUDER_NUM);
=======
    SERIAL_ECHOLNPGM(MSG_PID_BAD_EXTRUDER_NUM);
    #if ENABLED(EXTENSIBLE_UI)
      ExtUI::OnPidTuning(ExtUI::result_t::PID_BAD_EXTRUDER_NUM);
>>>>>>> 2b7ac9ca62c71088824dd1eb57906e58d42de222:Marlin-2.0.x/Marlin/src/gcode/temperature/M303.cpp
    #endif
    return;
  }

  const int c = parser.intval('C', 5);
  const bool u = parser.boolval('U');
  const int16_t temp = parser.celsiusval('S', e < 0 ? 70 : 150);

  #if DISABLED(BUSY_WHILE_HEATING)
    KEEPALIVE_STATE(NOT_BUSY);
  #endif

  thermalManager.PID_autotune(temp, e, c, u);
}

#endif // HAS_PID_HEATING
