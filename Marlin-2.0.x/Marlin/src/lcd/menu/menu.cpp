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

#include "../../inc/MarlinConfigPre.h"

#if HAS_LCD_MENU

#include "menu.h"
#include "../../module/planner.h"
#include "../../module/motion.h"
#include "../../module/printcounter.h"
#include "../../gcode/queue.h"

#if HAS_BUZZER
  #include "../../libs/buzzer.h"
#endif

#if ENABLED(EEPROM_SETTINGS)
  #include "../../module/configuration_store.h"
#endif

#if WATCH_HOTENDS || WATCH_BED
  #include "../../module/temperature.h"
#endif

#if ENABLED(BABYSTEP_ZPROBE_OFFSET)
  #include "../../module/probe.h"
#endif

#if EITHER(ENABLE_LEVELING_FADE_HEIGHT, AUTO_BED_LEVELING_UBL)
  #include "../../feature/bedlevel/bedlevel.h"
#endif

////////////////////////////////////////////
///////////// Global Variables /////////////
////////////////////////////////////////////

// Menu Navigation
int8_t encoderTopLine, encoderLine, screen_items;

typedef struct {
  screenFunc_t menu_function;
  uint32_t encoder_position;
  int8_t top_line, items;
} menuPosition;
menuPosition screen_history[6];
uint8_t screen_history_depth = 0;

uint8_t MenuItemBase::itemIndex;  // Index number for draw and action
chimera_t editable;               // Value Editing

// Menu Edit Items
PGM_P        MenuEditItemBase::editLabel;
void*        MenuEditItemBase::editValue;
int32_t      MenuEditItemBase::minEditValue,
             MenuEditItemBase::maxEditValue;
screenFunc_t MenuEditItemBase::callbackFunc;
bool         MenuEditItemBase::liveEdit;

// Prevent recursion into screen handlers
bool no_reentry = false;

////////////////////////////////////////////
//////// Menu Navigation & History /////////
////////////////////////////////////////////

void MarlinUI::return_to_status() { goto_screen(status_screen); }

void MarlinUI::save_previous_screen() {
  if (screen_history_depth < COUNT(screen_history))
    screen_history[screen_history_depth++] = { currentScreen, encoderPosition, encoderTopLine, screen_items };
}

void MarlinUI::_goto_previous_screen(
  #if ENABLED(TURBO_BACK_MENU_ITEM)
    const bool is_back/*=false*/
  #endif
) {
  #if DISABLED(TURBO_BACK_MENU_ITEM)
    constexpr bool is_back = false;
  #endif
  if (screen_history_depth > 0) {
    menuPosition &sh = screen_history[--screen_history_depth];
    goto_screen(sh.menu_function,
      is_back ? 0 : sh.encoder_position,
      is_back ? 0 : sh.top_line,
      sh.items
    );
  }
  else
    return_to_status();
}

////////////////////////////////////////////
/////////// Common Menu Actions ////////////
////////////////////////////////////////////

void MenuItem_gcode::action(PGM_P const, PGM_P const pgcode) { queue.inject_P(pgcode); }

////////////////////////////////////////////
/////////// Menu Editing Actions ///////////
////////////////////////////////////////////

/**
 * Functions for editing single values
 *
 * The "DEFINE_MENU_EDIT_ITEM" macro generates the classes needed to edit a numerical value.
 *
 * The prerequisite is that in the header the type was already declared:
 *
 *   DEFINE_MENU_EDIT_ITEM_TYPE(int3, int16_t, i16tostr3rj, 1)
 *
 * For example, DEFINE_MENU_EDIT_ITEM(int3) expands into:
 *
 *   template class TMenuEditItem<MenuEditItemInfo_int3>
 *
 * You can then use one of the menu macros to present the edit interface:
 *   EDIT_ITEM(int3, MSG_SPEED, &feedrate_percentage, 10, 999)
 *
 * This expands into a more primitive menu item:
 *  _MENU_ITEM_P(int3, false, GET_TEXT(MSG_SPEED), &feedrate_percentage, 10, 999)
 *
 * ...which calls:
 *       MenuItem_int3::action(plabel, &feedrate_percentage, 10, 999)
 *       MenuItem_int3::draw(encoderLine == _thisItemNr, _lcdLineNr, plabel, &feedrate_percentage, 10, 999)
 */
void MenuEditItemBase::edit_screen(strfunc_t strfunc, loadfunc_t loadfunc) {
  #if ENABLED(TOUCH_BUTTONS)
    ui.repeat_delay = BUTTON_DELAY_EDIT;
  #endif
  if (int32_t(ui.encoderPosition) < 0) ui.encoderPosition = 0;
  if (int32_t(ui.encoderPosition) > maxEditValue) ui.encoderPosition = maxEditValue;
  if (ui.should_draw())
    draw_edit_screen(strfunc(ui.encoderPosition + minEditValue));
  if (ui.lcd_clicked || (liveEdit && ui.should_draw())) {
    if (editValue != nullptr) loadfunc(editValue, ui.encoderPosition + minEditValue);
    if (callbackFunc && (liveEdit || ui.lcd_clicked)) (*callbackFunc)();
    if (ui.use_click()) ui.goto_previous_screen();
  }
}

void MenuEditItemBase::goto_edit_screen(
  PGM_P const el,         // Edit label
  void * const ev,        // Edit value pointer
  const int32_t minv,     // Encoder minimum
  const int32_t maxv,     // Encoder maximum
  const uint16_t ep,      // Initial encoder value
  const screenFunc_t cs,  // MenuItem_type::draw_edit_screen => MenuEditItemBase::edit()
  const screenFunc_t cb,  // Callback after edit
  const bool le           // Flag to call cb() during editing
) {
  ui.screen_changed = true;
  ui.save_previous_screen();
  ui.refresh();
  editLabel = el;
  editValue = ev;
  minEditValue = minv;
  maxEditValue = maxv;
  ui.encoderPosition = ep;
  ui.currentScreen = cs;
  callbackFunc = cb;
  liveEdit = le;
}

// TODO: Remove these but test build size with and without
#define DEFINE_MENU_EDIT_ITEM(NAME) template class TMenuEditItem<MenuEditItemInfo_##NAME>

DEFINE_MENU_EDIT_ITEM(percent);     // 100%       right-justified
DEFINE_MENU_EDIT_ITEM(int3);        // 123, -12   right-justified
DEFINE_MENU_EDIT_ITEM(int4);        // 1234, -123 right-justified
DEFINE_MENU_EDIT_ITEM(int8);        // 123, -12   right-justified
DEFINE_MENU_EDIT_ITEM(uint8);       // 123        right-justified
DEFINE_MENU_EDIT_ITEM(uint16_3);    // 123        right-justified
DEFINE_MENU_EDIT_ITEM(uint16_4);    // 1234       right-justified
DEFINE_MENU_EDIT_ITEM(uint16_5);    // 12345      right-justified
DEFINE_MENU_EDIT_ITEM(float3);      // 123        right-justified
DEFINE_MENU_EDIT_ITEM(float42_52);  // _2.34, 12.34, -2.34 or 123.45, -23.45
DEFINE_MENU_EDIT_ITEM(float43);     // 1.234
DEFINE_MENU_EDIT_ITEM(float5);      // 12345      right-justified
DEFINE_MENU_EDIT_ITEM(float5_25);   // 12345      right-justified (25 increment)
DEFINE_MENU_EDIT_ITEM(float51);     // 1234.5     right-justified
DEFINE_MENU_EDIT_ITEM(float41sign); // +123.4
DEFINE_MENU_EDIT_ITEM(float51sign); // +1234.5
DEFINE_MENU_EDIT_ITEM(float52sign); // +123.45
DEFINE_MENU_EDIT_ITEM(long5);       // 12345      right-justified
DEFINE_MENU_EDIT_ITEM(long5_25);    // 12345      right-justified (25 increment)

void MenuItem_bool::action(PGM_P const, bool * const ptr, screenFunc_t callback) {
  *ptr ^= true; ui.refresh();
  if (callback) (*callback)();
}

////////////////////////////////////////////
///////////////// Menu Tree ////////////////
////////////////////////////////////////////

#include "../../MarlinCore.h"

bool printer_busy() {
  return planner.movesplanned() || printingIsActive();
}

/**
 * General function to go directly to a screen
 */
void MarlinUI::goto_screen(screenFunc_t screen, const uint16_t encoder/*=0*/, const uint8_t top/*=0*/, const uint8_t items/*=0*/) {
  if (currentScreen != screen) {

    #if ENABLED(TOUCH_BUTTONS)
      repeat_delay = BUTTON_DELAY_MENU;
    #endif

    #if ENABLED(LCD_SET_PROGRESS_MANUALLY)
      progress_reset();
    #endif

    #if BOTH(DOUBLECLICK_FOR_Z_BABYSTEPPING, BABYSTEPPING)
      static millis_t doubleclick_expire_ms = 0;
      // Going to menu_main from status screen? Remember first click time.
      // Going back to status screen within a very short time? Go to Z babystepping.
      if (screen == menu_main) {
        if (on_status_screen())
          doubleclick_expire_ms = millis() + DOUBLECLICK_MAX_INTERVAL;
      }
      else if (screen == status_screen && currentScreen == menu_main && PENDING(millis(), doubleclick_expire_ms)) {

        #if ENABLED(BABYSTEP_WITHOUT_HOMING)
          constexpr bool can_babystep = true;
        #else
          const bool can_babystep = all_axes_known();
        #endif
        #if ENABLED(BABYSTEP_ALWAYS_AVAILABLE)
          constexpr bool should_babystep = true;
        #else
          const bool should_babystep = printer_busy();
        #endif

        if (should_babystep && can_babystep) {
          screen =
            #if ENABLED(BABYSTEP_ZPROBE_OFFSET)
              lcd_babystep_zoffset
            #else
              lcd_babystep_z
            #endif
          ;
        }
        #if ENABLED(MOVE_Z_WHEN_IDLE)
          else {
            move_menu_scale = MOVE_Z_IDLE_MULTIPLICATOR;
            screen = lcd_move_z;
          }
        #endif
      }
    #endif

    currentScreen = screen;
    encoderPosition = encoder;
    encoderTopLine = top;
    screen_items = items;
    if (screen == status_screen) {
      defer_status_screen(false);
      #if ENABLED(AUTO_BED_LEVELING_UBL)
        ubl.lcd_map_control = false;
      #endif
      screen_history_depth = 0;
    }

    clear_lcd();

    // Re-initialize custom characters that may be re-used
    #if HAS_CHARACTER_LCD
      if (true
        #if ENABLED(AUTO_BED_LEVELING_UBL)
          && !ubl.lcd_map_control
        #endif
      ) set_custom_characters(screen == status_screen ? CHARSET_INFO : CHARSET_MENU);
    #endif

    refresh(LCDVIEW_CALL_REDRAW_NEXT);
    screen_changed = true;
    #if HAS_GRAPHICAL_LCD
      drawing_screen = false;
    #endif

    #if HAS_LCD_MENU
      encoder_direction_normal();
    #endif

    set_selection(false);
  }
}

////////////////////////////////////////////
///////////// Manual Movement //////////////
////////////////////////////////////////////

//
// Display the synchronize screen until moves are
// finished, and don't return to the caller until
// done. ** This blocks the command queue! **
//
static PGM_P sync_message;

void MarlinUI::_synchronize() {
  if (should_draw()) MenuItem_static::draw(LCD_HEIGHT >= 4, sync_message);
  if (no_reentry) return;
  // Make this the current handler till all moves are done
  const screenFunc_t old_screen = currentScreen;
  goto_screen(_synchronize);
  no_reentry = true;
  planner.synchronize(); // idle() is called until moves complete
  no_reentry = false;
  goto_screen(old_screen);
}

// Display the synchronize screen with a custom message
// ** This blocks the command queue! **
void MarlinUI::synchronize(PGM_P const msg/*=nullptr*/) {
  sync_message = msg ?: GET_TEXT(MSG_MOVING);
  _synchronize();
}

/**
 * Scrolling for menus and other line-based screens
 *
 *   encoderLine is the position based on the encoder
 *   encoderTopLine is the top menu line to display
 *   _lcdLineNr is the index of the LCD line (e.g., 0-3)
 *   _menuLineNr is the menu item to draw and process
 *   _thisItemNr is the index of each MENU_ITEM or STATIC_ITEM
 *   screen_items is the total number of items in the menu (after one call)
 */
void scroll_screen(const uint8_t limit, const bool is_menu) {
  ui.encoder_direction_menus();
  ENCODER_RATE_MULTIPLY(false);
  if (int32_t(ui.encoderPosition) < 0) ui.encoderPosition = 0;
  if (ui.first_page) {
    encoderLine = ui.encoderPosition / (ENCODER_STEPS_PER_MENU_ITEM);
    ui.screen_changed = false;
  }
  if (screen_items > 0 && encoderLine >= screen_items - limit) {
    encoderLine = _MAX(0, screen_items - limit);
    ui.encoderPosition = encoderLine * (ENCODER_STEPS_PER_MENU_ITEM);
  }
  if (is_menu) {
    NOMORE(encoderTopLine, encoderLine);
    if (encoderLine >= encoderTopLine + LCD_HEIGHT)
      encoderTopLine = encoderLine - LCD_HEIGHT + 1;
  }
  else
    encoderTopLine = encoderLine;
}

#if HAS_BUZZER
  void MarlinUI::completion_feedback(const bool good/*=true*/) {
    if (good) {
      BUZZ(100, 659);
      BUZZ(100, 698);
    }
    else BUZZ(20, 440);
  }
#endif

#if HAS_LINE_TO_Z

  void line_to_z(const float &z) {
    current_position.z = z;
    line_to_current_position(manual_feedrate_mm_s.z);
  }

#endif

#if ENABLED(BABYSTEP_ZPROBE_OFFSET)

  #include "../../feature/babystep.h"

  void lcd_babystep_zoffset() {
    if (ui.use_click()) return ui.goto_previous_screen_no_defer();
    ui.defer_status_screen();
    const bool do_probe = DISABLED(BABYSTEP_HOTEND_Z_OFFSET) || active_extruder == 0;
    if (ui.encoderPosition) {
      const int16_t babystep_increment = int16_t(ui.encoderPosition) * (BABYSTEP_MULTIPLICATOR_Z);
      ui.encoderPosition = 0;

      const float diff = planner.steps_to_mm[Z_AXIS] * babystep_increment,
                  new_probe_offset = probe.offset.z + diff,
                  new_offs =
                    #if ENABLED(BABYSTEP_HOTEND_Z_OFFSET)
                      do_probe ? new_probe_offset : hotend_offset[active_extruder].z - diff
                    #else
                      new_probe_offset
                    #endif
                  ;
      if (WITHIN(new_offs, Z_PROBE_OFFSET_RANGE_MIN, Z_PROBE_OFFSET_RANGE_MAX)) {

        babystep.add_steps(Z_AXIS, babystep_increment);

        if (do_probe) probe.offset.z = new_offs;
        #if ENABLED(BABYSTEP_HOTEND_Z_OFFSET)
          else hotend_offset[active_extruder].z = new_offs;
        #endif

        ui.refresh(LCDVIEW_CALL_REDRAW_NEXT);
      }
    }
    if (ui.should_draw()) {
      #if ENABLED(BABYSTEP_HOTEND_Z_OFFSET)
        if (!do_probe)
<<<<<<< HEAD
          MenuEditItemBase::draw_edit_screen(GET_TEXT(MSG_HOTEND_OFFSET_Z), ftostr54sign(hotend_offset[active_extruder].z));
=======
          MenuEditItemBase::draw_edit_screen(GET_TEXT(MSG_HOTEND_OFFSET_Z), LCD_Z_OFFSET_FUNC(hotend_offset[active_extruder].z));
        else
      #endif
          MenuEditItemBase::draw_edit_screen(GET_TEXT(MSG_ZPROBE_ZOFFSET), LCD_Z_OFFSET_FUNC(probe_offset.z));

      #if ENABLED(BABYSTEP_ZPROBE_GFX_OVERLAY)
        if (do_probe) _lcd_zoffset_overlay_gfx(probe_offset.z);
>>>>>>> 2b7ac9ca62c71088824dd1eb57906e58d42de222
      #endif
      if (do_probe) {
        MenuEditItemBase::draw_edit_screen(GET_TEXT(MSG_ZPROBE_ZOFFSET), BABYSTEP_TO_STR(probe.offset.z));
        #if ENABLED(BABYSTEP_ZPROBE_GFX_OVERLAY)
          _lcd_zoffset_overlay_gfx(probe.offset.z);
        #endif
      }
    }
  }

#endif // BABYSTEP_ZPROBE_OFFSET

#if ANY(AUTO_BED_LEVELING_UBL, PID_AUTOTUNE_MENU, ADVANCED_PAUSE_FEATURE)

  void lcd_enqueue_one_now(const char * const cmd) {
    no_reentry = true;
    queue.enqueue_one_now(cmd);
    no_reentry = false;
  }

  void lcd_enqueue_one_now_P(PGM_P const cmd) {
    no_reentry = true;
    queue.enqueue_now_P(cmd);
    no_reentry = false;
  }

#endif

#if ENABLED(EEPROM_SETTINGS)
  void lcd_store_settings() {
    const bool saved = settings.save();
    #if HAS_BUZZER
      ui.completion_feedback(saved);
    #endif
    UNUSED(saved);
  }
  void lcd_load_settings() {
    const bool loaded = settings.load();
    #if HAS_BUZZER
      ui.completion_feedback(loaded);
    #endif
    UNUSED(loaded);
  }
#endif

void _lcd_draw_homing() {
  constexpr uint8_t line = (LCD_HEIGHT - 1) / 2;
  if (ui.should_draw()) MenuItem_static::draw(line, GET_TEXT(MSG_LEVEL_BED_HOMING));
}

#if ENABLED(LCD_BED_LEVELING) || (HAS_LEVELING && DISABLED(SLIM_LCD_MENUS))
  #include "../../feature/bedlevel/bedlevel.h"
  void _lcd_toggle_bed_leveling() { set_bed_leveling_enabled(!planner.leveling_active); }
#endif

//
// Selection screen presents a prompt and two options
//
bool MarlinUI::selection; // = false
bool MarlinUI::update_selection() {
  encoder_direction_select();
  if (encoderPosition) {
    selection = int16_t(encoderPosition) > 0;
    encoderPosition = 0;
  }
  return selection;
}

void MenuItem_confirm::select_screen(PGM_P const yes, PGM_P const no, selectFunc_t yesFunc, selectFunc_t noFunc, PGM_P const pref, const char * const string/*=nullptr*/, PGM_P const suff/*=nullptr*/) {
  const bool ui_selection = ui.update_selection(), got_click = ui.use_click();
  if (got_click || ui.should_draw()) {
    draw_select_screen(yes, no, ui_selection, pref, string, suff);
    if (got_click) { ui_selection ? yesFunc() : noFunc(); }
  }
}

#endif // HAS_LCD_MENU
