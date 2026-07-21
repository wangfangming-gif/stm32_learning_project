#include "../BSP/LCD/segment_lcd_ui.h"
#include "../BSP/HT16220/ht16220.h"
#include <string.h>

#define LCD_BLINK_TICKS       50U
#define LCD_AUTO_RETURN_TICKS 1000U
#define POINT(seg1, com1) ((uint16_t)((((uint16_t)(seg1) - 1U) << 3) | ((uint16_t)(com1) - 1U)))
#define GLYPH_BIT(x) (1UL << (x))

/*
 * 用户没有指定“探杆长度”右下角缩写，当前使用PL（Probe Length）。
 * 如需修改，只改这里即可。
 */
#define LCD_PROBE_LENGTH_TEXT "PL"

typedef enum
{
  G_A = 0, G_B, G_C, G_D, G_E, G_F, G_G, G_H,
  G_I, G_J, G_K, G_L, G_M, G_N, G_O, G_P
} GlyphSeg_t;

typedef struct
{
  uint16_t a, b, c, d, e, f, g;
} SevenSegMap_t;

typedef struct
{
  uint16_t point[16];
} AlphaMap_t;

static const SevenSegMap_t top_digit[5] =
{
  {POINT(15,7),POINT(15,6),POINT(15,3),POINT(15,1),POINT(15,2),POINT(15,5),POINT(15,4)},
  {POINT(16,7),POINT(16,6),POINT(16,3),POINT(16,1),POINT(16,2),POINT(16,5),POINT(16,4)},
  {POINT(17,7),POINT(17,6),POINT(17,3),POINT(17,1),POINT(17,2),POINT(17,5),POINT(17,4)},
  {POINT(18,7),POINT(18,6),POINT(18,3),POINT(18,1),POINT(18,2),POINT(18,5),POINT(18,4)},
  {POINT(19,7),POINT(19,6),POINT(19,3),POINT(19,1),POINT(19,2),POINT(19,5),POINT(19,4)}
};

static const SevenSegMap_t temp_digit[2] =
{
  {POINT(20,7),POINT(20,6),POINT(20,3),POINT(20,1),POINT(20,2),POINT(20,5),POINT(20,4)},
  {POINT(21,7),POINT(21,6),POINT(21,3),POINT(21,1),POINT(21,2),POINT(21,5),POINT(21,4)}
};

static const AlphaMap_t alpha_digit[5] =
{
  {{POINT(1,1),POINT(2,1),POINT(2,4),POINT(2,8),POINT(1,8),POINT(1,7),POINT(1,5),POINT(1,3),POINT(1,4),POINT(2,5),POINT(1,2),POINT(2,2),POINT(2,3),POINT(2,7),POINT(2,6),POINT(1,6)}},
  {{POINT(3,1),POINT(4,1),POINT(4,4),POINT(4,8),POINT(3,8),POINT(3,7),POINT(3,5),POINT(3,3),POINT(3,4),POINT(4,5),POINT(3,2),POINT(4,2),POINT(4,3),POINT(4,7),POINT(4,6),POINT(3,6)}},
  {{POINT(5,1),POINT(6,1),POINT(6,4),POINT(6,8),POINT(5,8),POINT(5,7),POINT(5,5),POINT(5,3),POINT(5,4),POINT(6,5),POINT(5,2),POINT(6,2),POINT(6,3),POINT(6,7),POINT(6,6),POINT(5,6)}},
  {{POINT(7,1),POINT(8,1),POINT(8,4),POINT(8,8),POINT(7,8),POINT(7,7),POINT(7,5),POINT(7,3),POINT(7,4),POINT(8,5),POINT(7,2),POINT(8,2),POINT(8,3),POINT(8,7),POINT(8,6),POINT(7,6)}},
  {{POINT(9,1),POINT(10,1),POINT(10,4),POINT(10,8),POINT(9,8),POINT(9,7),POINT(9,5),POINT(9,3),POINT(9,4),POINT(10,5),POINT(9,2),POINT(10,2),POINT(10,3),POINT(10,7),POINT(10,6),POINT(9,6)}}
};

#define P_TEMP_MINUS POINT(20,8)
#define P_TEMP_ICON  POINT(13,1)
#define P_ALPHA_DP3  POINT(11,8)
#define P_ALPHA_DP4  POINT(11,7)
#define P_ALPHA_DP5  POINT(11,6)
#define P_ALPHA_DP6  POINT(11,5)
#define P_TOP_DP9    POINT(11,1)

static const uint16_t progress_bar[20] =
{
  POINT(22,1),POINT(22,2),POINT(22,3),POINT(22,4),
  POINT(22,5),POINT(22,6),POINT(22,7),POINT(22,8),
  POINT(23,1),POINT(23,2),POINT(23,3),POINT(23,4),
  POINT(23,5),POINT(23,6),POINT(23,7),POINT(23,8),
  POINT(24,1),POINT(24,2),POINT(24,3),POINT(24,4)
};

static LCD_UI_Data_t ui_data;
static LCD_UI_Config_t ui_edit_backup;
static LCD_Page_t ui_page;
static bool ui_editing;
static uint8_t ui_selected_digit;
static volatile uint16_t ui_blink_tick;
static volatile bool ui_blink_toggle_pending;
static volatile uint16_t ui_inactive_tick;
static volatile bool ui_return_main_pending;
static bool ui_blink_visible;
static bool ui_render_pending;

static void set_point(uint16_t point, bool on)
{
  HT16220_SetPoint((uint8_t)(point >> 3), (uint8_t)(point & 0x07U), on);
}

static uint32_t limit_5_digits(uint32_t value)
{
  return value > 99999UL ? 99999UL : value;
}

static uint8_t seven_pattern(char ch)
{
  switch(ch)
  {
    case '0': case 'O': return 0x3FU;
    case '1': return 0x06U;
    case '2': return 0x5BU;
    case '3': return 0x4FU;
    case '4': return 0x66U;
    case '5': return 0x6DU;
    case '6': return 0x7DU;
    case '7': return 0x07U;
    case '8': return 0x7FU;
    case '9': return 0x6FU;
    case 'A': return 0x77U;
    case 'F': return 0x71U;
    case 'H': return 0x76U;
    case 'N': return 0x37U;
    case 'n': return 0x54U;
    case '-': return 0x40U;
    default: return 0x00U;
  }
}

static void seven_write(const SevenSegMap_t *map, char ch, bool visible)
{
  uint8_t p = visible ? seven_pattern(ch) : 0U;
  set_point(map->a, (p & 0x01U) != 0U);
  set_point(map->b, (p & 0x02U) != 0U);
  set_point(map->c, (p & 0x04U) != 0U);
  set_point(map->d, (p & 0x08U) != 0U);
  set_point(map->e, (p & 0x10U) != 0U);
  set_point(map->f, (p & 0x20U) != 0U);
  set_point(map->g, (p & 0x40U) != 0U);
}

static uint32_t alpha_pattern(char ch)
{
  const uint32_t TOP = GLYPH_BIT(G_A) | GLYPH_BIT(G_B);
  const uint32_t MID = GLYPH_BIT(G_I) | GLYPH_BIT(G_J);
  const uint32_t BOT = GLYPH_BIT(G_E) | GLYPH_BIT(G_F);
  const uint32_t UL = GLYPH_BIT(G_H);
  const uint32_t UR = GLYPH_BIT(G_C);
  const uint32_t LL = GLYPH_BIT(G_G);
  const uint32_t LR = GLYPH_BIT(G_D);
  switch(ch)
  {
    case '0': case 'O': return TOP | BOT | UL | UR | LL | LR;
    case '1': return UR | LR;
    case '2': return TOP | MID | BOT | UR | LL;
    case '3': return TOP | MID | BOT | UR | LR;
    case '4': return MID | UL | UR | LR;
    case '5': case 'S': return TOP | MID | BOT | UL | LR;
    case '6': return TOP | MID | BOT | UL | LL | LR;
    case '7': return TOP | UR | LR;
    case '8': return TOP | MID | BOT | UL | UR | LL | LR;
    case '9': return TOP | MID | BOT | UL | UR | LR;
    case 'A': return TOP | MID | UL | UR | LL | LR;
    case 'D': return MID | BOT | UR | LL | LR;
    case 'F': return TOP | MID | UL | LL;
    case 'H': return MID | UL | UR | LL | LR;
    case 'I': return TOP | BOT | GLYPH_BIT(G_L) | GLYPH_BIT(G_O);
    case 'L': return BOT | UL | LL;
    case 'N': return UL | UR | LL | LR | GLYPH_BIT(G_K) | GLYPH_BIT(G_N);
    case 'P': return TOP | MID | UL | UR | LL;
    case 'W': return UL | UR | LL | LR | GLYPH_BIT(G_P) | GLYPH_BIT(G_N);
    case '-': return MID;
    default: return 0UL;
  }
}

static void alpha_write(uint8_t position, char ch, bool visible)
{
  uint8_t i;
  uint32_t pattern;
  if(position >= 5U)
    return;
  pattern = visible ? alpha_pattern(ch) : 0UL;
  for(i = 0U; i < 16U; i++)
    set_point(alpha_digit[position].point[i], (pattern & GLYPH_BIT(i)) != 0UL);
}

static void format_fixed5(uint32_t value, char out[5])
{
  int8_t i;
  value = limit_5_digits(value);
  for(i = 4; i >= 0; i--)
  {
    out[i] = (char)('0' + (value % 10UL));
    value /= 10UL;
  }
}

/*
 * 主界面水高仍使用固定的小数点位置，但去掉无意义的前导零。
 * 例如：763 -> "  763" -> 显示76.3；5 -> "   05" -> 显示0.5。
 */
static void format_water_x10(uint32_t value, char out[5])
{
  uint8_t i;
  format_fixed5(value, out);
  for(i = 0U; i < 3U; i++)
  {
    if(out[i] == '0')
      out[i] = ' ';
    else
      break;
  }
}

static void top_show_digits(const char text[5], int8_t hidden_position)
{
  uint8_t i;
  for(i = 0U; i < 5U; i++)
    seven_write(&top_digit[i], text[i], (int8_t)i != hidden_position);
}

static void top_show_word(const char *word, bool visible)
{
  uint8_t len = (uint8_t)strlen(word);
  uint8_t i;
  for(i = 0U; i < 5U; i++)
  {
    int8_t source = (int8_t)i - (int8_t)(5U - len);
    seven_write(&top_digit[i], source >= 0 ? word[source] : ' ', visible);
  }
}

static void alpha_show_text(const char text[5])
{
  uint8_t i;
  for(i = 0U; i < 5U; i++)
    alpha_write(i, text[i], true);
}

static void alpha_show_water_x10(uint32_t value)
{
  char text[5];
  format_water_x10(value, text);
  alpha_show_text(text);
}

static void alpha_show_right(const char *word)
{
  uint8_t len = (uint8_t)strlen(word);
  uint8_t i;
  for(i = 0U; i < 5U; i++)
  {
    int8_t source = (int8_t)i - (int8_t)(5U - len);
    alpha_write(i, source >= 0 ? word[source] : ' ', true);
  }
}

static void hide_temperature_area(void)
{
  seven_write(&temp_digit[0], ' ', true);
  seven_write(&temp_digit[1], ' ', true);
  set_point(P_TEMP_MINUS, false);
  set_point(P_TEMP_ICON, false);
}

static void clear_alpha_decimal_points(void)
{
  set_point(P_ALPHA_DP3, false);
  set_point(P_ALPHA_DP4, false);
  set_point(P_ALPHA_DP5, false);
  set_point(P_ALPHA_DP6, false);
}

static bool page_is_numeric(LCD_Page_t page)
{
  switch(page)
  {
    case LCD_PAGE_HIGH_OIL_ALARM:
    case LCD_PAGE_HIGH_OIL_WARNING:
    case LCD_PAGE_LOW_OIL_ALARM:
    case LCD_PAGE_LOW_OIL_WARNING:
    case LCD_PAGE_HIGH_WATER_ALARM:
    case LCD_PAGE_WATER_SHIELD_HEIGHT:
    case LCD_PAGE_DEVICE_ADDRESS:
    case LCD_PAGE_PROBE_LENGTH:
    case LCD_PAGE_OIL_COMPENSATION:
    case LCD_PAGE_WATER_COMPENSATION:
      return true;
    default:
      return false;
  }
}

static uint32_t *page_value_pointer(LCD_Page_t page)
{
  switch(page)
  {
    case LCD_PAGE_HIGH_OIL_ALARM: return &ui_data.config.high_oil_alarm;
    case LCD_PAGE_HIGH_OIL_WARNING: return &ui_data.config.high_oil_warning;
    case LCD_PAGE_LOW_OIL_ALARM: return &ui_data.config.low_oil_alarm;
    case LCD_PAGE_LOW_OIL_WARNING: return &ui_data.config.low_oil_warning;
    case LCD_PAGE_HIGH_WATER_ALARM: return &ui_data.config.high_water_alarm;
    case LCD_PAGE_WATER_SHIELD_HEIGHT: return &ui_data.config.water_shield_height;
    case LCD_PAGE_DEVICE_ADDRESS: return &ui_data.config.device_address;
    case LCD_PAGE_PROBE_LENGTH: return &ui_data.config.probe_length;
    case LCD_PAGE_OIL_COMPENSATION: return &ui_data.config.oil_compensation;
    case LCD_PAGE_WATER_COMPENSATION: return &ui_data.config.water_compensation;
    default: return NULL;
  }
}

static const char *page_right_text(LCD_Page_t page)
{
  switch(page)
  {
//    case LCD_PAGE_HIGH_OIL_ALARM: return "OHA";
//    case LCD_PAGE_HIGH_OIL_WARNING: return "OHW";
//    case LCD_PAGE_LOW_OIL_ALARM: return "OLA";
//    case LCD_PAGE_LOW_OIL_WARNING: return "OLW";
//    case LCD_PAGE_HIGH_WATER_ALARM: return "HHA";
//    case LCD_PAGE_WATER_SHIELD_HEIGHT: return "SH";
//    case LCD_PAGE_DEVICE_ADDRESS: return "ID";
//    case LCD_PAGE_PROBE_LENGTH: return LCD_PROBE_LENGTH_TEXT;
//    case LCD_PAGE_OIL_COMPENSATION: return "OS";
//    case LCD_PAGE_WATER_COMPENSATION: return "HS";
    case LCD_PAGE_HIGH_OIL_ALARM: return "11";
    case LCD_PAGE_HIGH_OIL_WARNING: return "10";
    case LCD_PAGE_LOW_OIL_ALARM: return "09";
    case LCD_PAGE_LOW_OIL_WARNING: return "08";
    case LCD_PAGE_HIGH_WATER_ALARM: return "07";
    case LCD_PAGE_WATER_SHIELD_HEIGHT: return "05";
    case LCD_PAGE_DEVICE_ADDRESS: return "04";
    case LCD_PAGE_PROBE_LENGTH: return "03";
    case LCD_PAGE_OIL_COMPENSATION: return "02";
    case LCD_PAGE_WATER_COMPENSATION: return "01";
    default: return "";
  }
}

static void render_main_page(void)
{
  char top[5];
  uint32_t oil = ui_data.oil_height_x10 < 0 ? 0UL : (uint32_t)ui_data.oil_height_x10;
  uint32_t water = ui_data.water_height_x10 < 0 ? 0UL : (uint32_t)ui_data.water_height_x10;
  int16_t temp = ui_data.temperature_x10;
  uint16_t temp_abs;

  oil = limit_5_digits(oil);
  water = limit_5_digits(water);

  format_fixed5(oil, top);
  top_show_digits(top, -1);
  set_point(P_TOP_DP9, true);

  alpha_show_water_x10(water);
  set_point(P_ALPHA_DP6, true);
  set_point(P_ALPHA_DP3, false);
  set_point(P_ALPHA_DP4, false);
  set_point(P_ALPHA_DP5, false);

  if(temp < 0)
  {
    temp_abs = (uint16_t)(-temp);
    set_point(P_TEMP_MINUS, true);
  }
  else
  {
    temp_abs = (uint16_t)temp;
    set_point(P_TEMP_MINUS, false);
  }

  temp_abs = (uint16_t)((temp_abs + 5U) / 10U);
  if(temp_abs > 99U)
    temp_abs = 99U;

  seven_write(&temp_digit[0], (char)('0' + temp_abs / 10U), true);
  seven_write(&temp_digit[1], (char)('0' + temp_abs % 10U), true);
  set_point(P_TEMP_ICON, true);
}

static void render_numeric_page(void)
{
  char top[5];
  int8_t hidden = -1;
  uint32_t *value = page_value_pointer(ui_page);

  if(value == NULL)
    return;

  format_fixed5(*value, top);
  if(ui_editing && !ui_blink_visible)
    hidden = (int8_t)(4U - ui_selected_digit);

  top_show_digits(top, hidden);
  set_point(P_TOP_DP9, false);
  hide_temperature_area();
  clear_alpha_decimal_points();
  alpha_show_right(page_right_text(ui_page));
}

static void render_alarm_enable_page(void)
{
  bool visible = !(ui_editing && !ui_blink_visible);

  top_show_word(ui_data.config.alarm_enable != 0UL ? "ON" : "OFF", visible);
  set_point(P_TOP_DP9, false);
  hide_temperature_area();
  clear_alpha_decimal_points();
  alpha_show_right("06");
}

static void render_progress_bar(void)
{
  uint8_t i;
  uint8_t active_count;
  uint8_t percent = ui_data.progress_percent;

  if(percent > 100U)
    percent = 100U;

  if(percent == 0U)
    active_count = 0U;
  else
    active_count = (uint8_t)(((uint16_t)percent * 20U + 99U) / 100U);

  for(i = 0U; i < 20U; i++)
    set_point(progress_bar[i], i < active_count);
}

static void render_page(void)
{
  HT16220_Clear();

  if(ui_page == LCD_PAGE_MAIN)
    render_main_page();
  else if(ui_page == LCD_PAGE_ALARM_ENABLE)
    render_alarm_enable_page();
  else if(page_is_numeric(ui_page))
    render_numeric_page();

  render_progress_bar();
}

static void page_next(void)
{
  ui_page = (LCD_Page_t)((ui_page + 1U) % LCD_PAGE_COUNT);
  ui_render_pending = true;
}

static void page_prev(void)
{
  if(ui_page == LCD_PAGE_MAIN)
    ui_page = (LCD_Page_t)(LCD_PAGE_COUNT - 1U);
  else
    ui_page = (LCD_Page_t)(ui_page - 1U);
  ui_render_pending = true;
}

static void increment_selected_digit(void)
{
  static const uint32_t power10[5] = {1UL,10UL,100UL,1000UL,10000UL};
  uint32_t *value = page_value_pointer(ui_page);
  uint32_t base;
  uint32_t digit;

  if(value == NULL)
    return;

  if(ui_selected_digit >= 5U)
    ui_selected_digit = 0U;

  base = power10[ui_selected_digit];
  digit = (*value / base) % 10UL;

  if(digit >= 9UL)
    *value -= 9UL * base;
  else
    *value += base;

  *value = limit_5_digits(*value);
}

static void sanitize_config(LCD_UI_Config_t *config)
{
  config->high_oil_alarm = limit_5_digits(config->high_oil_alarm);
  config->high_oil_warning = limit_5_digits(config->high_oil_warning);
  config->low_oil_alarm = limit_5_digits(config->low_oil_alarm);
  config->low_oil_warning = limit_5_digits(config->low_oil_warning);
  config->high_water_alarm = limit_5_digits(config->high_water_alarm);
  config->alarm_enable = config->alarm_enable != 0UL ? 1UL : 0UL;
  config->water_shield_height = limit_5_digits(config->water_shield_height);
  config->device_address = limit_5_digits(config->device_address);
  config->probe_length = limit_5_digits(config->probe_length);
  config->oil_compensation = limit_5_digits(config->oil_compensation);
  config->water_compensation = limit_5_digits(config->water_compensation);
}

void LCD_UI_Init(void)
{
  memset(&ui_data, 0, sizeof(ui_data));
  memset(&ui_edit_backup, 0, sizeof(ui_edit_backup));

  ui_data.config.device_address = 1UL;
  ui_page = LCD_PAGE_MAIN;
  ui_editing = false;
  ui_selected_digit = 0U;
  ui_blink_tick = 0U;
  ui_blink_toggle_pending = false;
  ui_inactive_tick = 0U;
  ui_return_main_pending = false;
  ui_blink_visible = true;
  ui_render_pending = true;

  HT16220_Init();
}

void LCD_UI_Tick10ms(void)
{
  if(ui_page != LCD_PAGE_MAIN)
  {
    if(ui_inactive_tick < LCD_AUTO_RETURN_TICKS)
      ui_inactive_tick++;

    if(ui_inactive_tick >= LCD_AUTO_RETURN_TICKS)
    {
      ui_inactive_tick = 0U;
      ui_return_main_pending = true;
    }
  }
  else
  {
    ui_inactive_tick = 0U;
  }

  if(!ui_editing)
    return;

  ui_blink_tick++;

  if(ui_blink_tick >= LCD_BLINK_TICKS)
  {
    ui_blink_tick = 0U;
    ui_blink_toggle_pending = true;
  }
}

void LCD_UI_Task(void)
{
  if(ui_return_main_pending)
  {
    ui_return_main_pending = false;

    /*
     * 设置中超时返回主界面时，放弃尚未按SET确认的修改。
     */
    if(ui_editing)
      ui_data.config = ui_edit_backup;

    ui_page = LCD_PAGE_MAIN;
    ui_editing = false;
    ui_selected_digit = 0U;
    ui_blink_tick = 0U;
    ui_blink_toggle_pending = false;
    ui_blink_visible = true;
    ui_render_pending = true;
  }

  if(ui_blink_toggle_pending)
  {
    ui_blink_toggle_pending = false;
    ui_blink_visible = !ui_blink_visible;
    ui_render_pending = true;
  }

  if(ui_render_pending)
  {
    ui_render_pending = false;
    render_page();
  }

  HT16220_FlushStep();
}

void LCD_UI_KeyEvent(LCD_KeyEvent_t key)
{
  ui_inactive_tick = 0U;
  ui_return_main_pending = false;

  if(!ui_editing)
  {
    if(key == LCD_KEY_UP)
      page_prev();
    else if(key == LCD_KEY_DOWN)
      page_next();
    else if(key == LCD_KEY_SET && ui_page != LCD_PAGE_MAIN)
    {
      ui_edit_backup = ui_data.config;
      ui_editing = true;
      ui_selected_digit = 0U;
      ui_blink_tick = 0U;
      ui_blink_visible = true;
      ui_render_pending = true;
    }
    return;
  }

  if(key == LCD_KEY_SET)
  {
    ui_editing = false;
    ui_blink_visible = true;
    ui_render_pending = true;
    LCD_UI_ConfigChangedCallback(&ui_data.config);
    return;
  }

  if(ui_page == LCD_PAGE_ALARM_ENABLE)
  {
    if(key == LCD_KEY_UP || key == LCD_KEY_DOWN)
      ui_data.config.alarm_enable = ui_data.config.alarm_enable == 0UL ? 1UL : 0UL;
  }
  else if(page_is_numeric(ui_page))
  {
    if(key == LCD_KEY_UP)
      increment_selected_digit();
    else if(key == LCD_KEY_DOWN)
      ui_selected_digit = (uint8_t)((ui_selected_digit + 1U) % 5U);
  }

  ui_blink_visible = true;
  ui_blink_tick = 0U;
  ui_render_pending = true;
}

void LCD_UI_SetMeasurements(int32_t oil_x10, int16_t temp_x10, int32_t water_x10)
{
  if(ui_data.oil_height_x10 == oil_x10 &&
     ui_data.temperature_x10 == temp_x10 &&
     ui_data.water_height_x10 == water_x10)
  {
    return;
  }

  ui_data.oil_height_x10 = oil_x10;
  ui_data.temperature_x10 = temp_x10;
  ui_data.water_height_x10 = water_x10;

  if(ui_page == LCD_PAGE_MAIN)
    ui_render_pending = true;
}

void LCD_UI_SetProgressPercent(uint8_t percent)
{
  if(percent > 100U)
    percent = 100U;

  if(ui_data.progress_percent == percent)
    return;

  ui_data.progress_percent = percent;
  ui_render_pending = true;
}

void LCD_UI_SetConfig(const LCD_UI_Config_t *config)
{
  if(config == NULL)
    return;

  ui_data.config = *config;
  sanitize_config(&ui_data.config);

  if(ui_page != LCD_PAGE_MAIN)
    ui_render_pending = true;
}

void LCD_UI_GetConfig(LCD_UI_Config_t *config)
{
  if(config != NULL)
    *config = ui_data.config;
}

void LCD_UI_SetHighLevelConfig(uint32_t threshold_mm, bool enable)
{
  ui_data.config.high_oil_alarm = limit_5_digits(threshold_mm);
  ui_data.config.alarm_enable = enable ? 1UL : 0UL;

  if(ui_page != LCD_PAGE_MAIN)
    ui_render_pending = true;
}

void LCD_UI_GetData(LCD_UI_Data_t *data)
{
  if(data != NULL)
    *data = ui_data;
}

LCD_Page_t LCD_UI_GetPage(void)
{
  return ui_page;
}

bool LCD_UI_IsEditing(void)
{
  return ui_editing;
}

#if defined(__GNUC__)
__attribute__((weak))
#endif
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
__weak
#endif
void LCD_UI_ConfigChangedCallback(const LCD_UI_Config_t *config)
{
  (void)config;
}
