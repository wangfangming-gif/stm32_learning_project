#include "../BSP/LCD/segment_lcd_ui.h"
#include "../BSP/HT16220/ht16220.h"
#include <string.h>

/* 闪烁翻转周期：50 × 10ms = 500ms。 */
#define LCD_BLINK_TICKS       50U    /* 50×10ms=500ms */
#define LCD_AUTO_RETURN_TICKS 1000U  /* 1000×10ms=10s */
/*
 * 屏幕资料中的 SEG/COM 编号从 1 开始，本驱动内部从 0 开始。
 * POINT 将一个 SEG/COM 交点压缩成 16 位索引：高位保存 SEG，低 3 位保存 COM。
 */
#define POINT(seg1, com1) ((uint16_t)((((uint16_t)(seg1) - 1U) << 3) | ((uint16_t)(com1) - 1U)))
#define GLYPH_BIT(x) (1UL << (x))

/* 16段字符的逻辑段名，对应屏幕资料中的A~P */
typedef enum
{
  G_A = 0, G_B, G_C, G_D, G_E, G_F, G_G, G_H,
  G_I, G_J, G_K, G_L, G_M, G_N, G_O, G_P
} GlyphSeg_t;

/* 一个七段数字每根逻辑段对应的物理 SEG/COM 点。 */
typedef struct
{
  uint16_t a, b, c, d, e, f, g;
} SevenSegMap_t;

/* 一个 16 段字符的 16 根逻辑段与物理点映射。 */
typedef struct
{
  uint16_t point[16];
} AlphaMap_t;

/* 上方5位数字：从左到右分别是屏幕标号12、11、10、9、8 */
static const SevenSegMap_t top_digit[5] =
{
  {POINT(15,7),POINT(15,6),POINT(15,3),POINT(15,1),POINT(15,2),POINT(15,5),POINT(15,4)},
  {POINT(16,7),POINT(16,6),POINT(16,3),POINT(16,1),POINT(16,2),POINT(16,5),POINT(16,4)},
  {POINT(17,7),POINT(17,6),POINT(17,3),POINT(17,1),POINT(17,2),POINT(17,5),POINT(17,4)},
  {POINT(18,7),POINT(18,6),POINT(18,3),POINT(18,1),POINT(18,2),POINT(18,5),POINT(18,4)},
  {POINT(19,7),POINT(19,6),POINT(19,3),POINT(19,1),POINT(19,2),POINT(19,5),POINT(19,4)}
};

/* 左下角温度两位：屏幕标号1、2 */
static const SevenSegMap_t temp_digit[2] =
{
  {POINT(20,7),POINT(20,6),POINT(20,3),POINT(20,1),POINT(20,2),POINT(20,5),POINT(20,4)},
  {POINT(21,7),POINT(21,6),POINT(21,3),POINT(21,1),POINT(21,2),POINT(21,5),POINT(21,4)}
};

/* 右下角5个16段字符：屏幕标号3~7 */
static const AlphaMap_t alpha_digit[5] =
{
  {{POINT(1,1),POINT(2,1),POINT(2,4),POINT(2,8),POINT(1,8),POINT(1,7),POINT(1,5),POINT(1,3),POINT(1,4),POINT(2,5),POINT(1,2),POINT(2,2),POINT(2,3),POINT(2,7),POINT(2,6),POINT(1,6)}},
  {{POINT(3,1),POINT(4,1),POINT(4,4),POINT(4,8),POINT(3,8),POINT(3,7),POINT(3,5),POINT(3,3),POINT(3,4),POINT(4,5),POINT(3,2),POINT(4,2),POINT(4,3),POINT(4,7),POINT(4,6),POINT(3,6)}},
  {{POINT(5,1),POINT(6,1),POINT(6,4),POINT(6,8),POINT(5,8),POINT(5,7),POINT(5,5),POINT(5,3),POINT(5,4),POINT(6,5),POINT(5,2),POINT(6,2),POINT(6,3),POINT(6,7),POINT(6,6),POINT(5,6)}},
  {{POINT(7,1),POINT(8,1),POINT(8,4),POINT(8,8),POINT(7,8),POINT(7,7),POINT(7,5),POINT(7,3),POINT(7,4),POINT(8,5),POINT(7,2),POINT(8,2),POINT(8,3),POINT(8,7),POINT(8,6),POINT(7,6)}},
  {{POINT(9,1),POINT(10,1),POINT(10,4),POINT(10,8),POINT(9,8),POINT(9,7),POINT(9,5),POINT(9,3),POINT(9,4),POINT(10,5),POINT(9,2),POINT(10,2),POINT(10,3),POINT(10,7),POINT(10,6),POINT(9,6)}}
};

/* S点来自用户提供的PIN/COM/SEG映射表 */
#define P_TEMP_MINUS POINT(20,8) /* S1 */
#define P_TEMP_ICON  POINT(13,1) /* S2，温度℃标志 */
#define P_ALPHA_DP3  POINT(11,8) /* S3，字符3后小数点 */
#define P_ALPHA_DP4  POINT(11,7) /* S4 */
#define P_ALPHA_DP5  POINT(11,6) /* S5 */
#define P_ALPHA_DP6  POINT(11,5) /* S6，字符6后小数点 */
#define P_TOP_DP9    POINT(11,1) /* S10，顶部数字9后小数点 */

/*
 * 顶部弧形进度条，共20格。
 * 数组顺序按照屏幕视觉位置从左到右排列：S41 -> S22。
 * 后续只需要调用 LCD_UI_SetProgressPercent() 即可动态刷新。
 */
static const uint16_t progress_bar[20] =
{
  POINT(22,1), POINT(22,2), POINT(22,3), POINT(22,4),
  POINT(22,5), POINT(22,6), POINT(22,7), POINT(22,8),
  POINT(23,1), POINT(23,2), POINT(23,3), POINT(23,4),
  POINT(23,5), POINT(23,6), POINT(23,7), POINT(23,8),
  POINT(24,1), POINT(24,2), POINT(24,3), POINT(24,4)
};

/*
 * 界面运行状态。
 * ui_data：业务数据
 * ui_page：当前页面
 * ui_editing：是否处于设置模式
 * ui_selected_digit：阈值设置时当前编辑位，0=个位，4=万位
 */
static LCD_UI_Data_t ui_data;
static LCD_Page_t ui_page;
static bool ui_editing;
static uint8_t ui_selected_digit; /* 0=个位，1=十位...4=万位 */
/*
 * 以下变量由 10ms 中断和 main 同时访问，因此中断相关变量使用 volatile。
 * 中断只置 pending 标志，真正重绘和 GPIO 通信都在 LCD_UI_Task 中完成。
 */
static volatile uint16_t ui_blink_tick;
static volatile bool ui_blink_toggle_pending;
static volatile uint16_t ui_inactive_tick;
static volatile bool ui_return_main_pending;
static bool ui_blink_visible;
static bool ui_render_pending;

/* 把压缩后的物理点索引还原成 SEG/COM，并写入 HT16220 缓存。 */
static void set_point(uint16_t point, bool on)
{
  HT16220_SetPoint((uint8_t)(point >> 3), (uint8_t)(point & 0x07U), on);
}

/*
 * 将数字或少量字母转换为标准七段码。
 * bit0~bit6 依次对应 a~g 段。
 */
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
    case 'N': return 0x37U; /* 七段数码管对大写N的近似字形，比小写n更饱满 */
    case 'n': return 0x54U;
    case '-': return 0x40U;
    default: return 0x00U;
  }
}

/*
 * 在指定七段位置显示一个字符。
 * visible=false 时会熄灭该字符，用于闪烁效果。
 */
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

/*
 * 将字符转换为 16 段字模。
 * 字模和具体玻璃段形状有关，若 W/O/F 等显示不理想，优先修改本函数。
 */
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
    case '5': return TOP | MID | BOT | UL | LR;
    case '6': return TOP | MID | BOT | UL | LL | LR;
    case '7': return TOP | UR | LR;
    case '8': return TOP | MID | BOT | UL | UR | LL | LR;
    case '9': return TOP | MID | BOT | UL | UR | LR;
    case 'A': return TOP | MID | UL | UR | LL | LR;
    case 'F': return TOP | MID | UL | LL;
    case 'H': return MID | UL | UR | LL | LR;
    case 'W': return UL | UR | LL | LR | GLYPH_BIT(G_P) | GLYPH_BIT(G_N);
    case 'N': return UL | UR | LL | LR | GLYPH_BIT(G_K) | GLYPH_BIT(G_N);
    case '-': return MID;
    default: return 0;
  }
}

/* 在右下角指定 16 段字符位置显示一个字符。 */
static void alpha_write(uint8_t position, char ch, bool visible)
{
  uint8_t i;
  uint32_t pattern;
  if(position >= 5U)
    return;
  pattern = visible ? alpha_pattern(ch) : 0U;
  for(i = 0; i < 16U; i++)
    set_point(alpha_digit[position].point[i], (pattern & GLYPH_BIT(i)) != 0U);
}

/*
 * 把 0~99999 转换成固定 5 位十进制字符，不省略前导零。
 * 例如 1234 转换为 “01234”。
 */
static void format_fixed5(uint32_t value, char out[5])
{
  int8_t i;
  if(value > 99999UL)
    value = 99999UL;
  for(i = 4; i >= 0; i--)
  {
    out[i] = (char)('0' + (value % 10UL));
    value /= 10UL;
  }
}

/*
 * 在顶部五位七段数字区显示 5 个字符。
 * hidden_position 为 0~4 时熄灭对应位，用于单个位闪烁；-1 表示全部显示。
 */
static void top_show_digits(const char text[5], int8_t hidden_position)
{
  uint8_t i;
  for(i = 0; i < 5U; i++)
    seven_write(&top_digit[i], text[i], (int8_t)i != hidden_position);
}

/*
 * 在顶部五位区域靠右显示字符串，例如 OFF 或 ON。
 * 整体 visible=false 时所有字符熄灭，用于整词闪烁。
 */
static void top_show_word(const char *word, bool visible)
{
  uint8_t len = (uint8_t)strlen(word);
  uint8_t i;
  for(i = 0; i < 5U; i++)
  {
    int8_t source = (int8_t)i - (int8_t)(5U - len);
    char ch = source >= 0 ? word[source] : ' ';
    seven_write(&top_digit[i], ch, visible);
  }
}

/* 右下角五个 16 段字符显示固定 5 位数字。 */
static void alpha_show_fixed5(uint32_t value)
{
  char text[5];
  uint8_t i;
  format_fixed5(value, text);
  for(i = 0; i < 5U; i++)
    alpha_write(i, text[i], true);
}

/* 在右下角 16 段区域靠右显示字符串，例如 WHA。 */
static void alpha_show_right(const char *word)
{
  uint8_t len = (uint8_t)strlen(word);
  uint8_t i;
  for(i = 0; i < 5U; i++)
  {
    int8_t source = (int8_t)i - (int8_t)(5U - len);
    alpha_write(i, source >= 0 ? word[source] : ' ', true);
  }
}

/*
 * 生成主界面缓存：
 * 顶部显示油高 xxxx.x；左下显示整数温度和温度标志；右下显示水高 xxxx.x。
 * oil_height_x10/water_height_x10 已经包含一位小数，因此直接格式化为 5 位数字，
 * 再点亮固定的小数点即可，例如 763 -> 00763 + 小数点 = 0076.3。
 */
static void render_main_page(void)
{
  char top[5];
  uint32_t oil = ui_data.oil_height_x10 < 0 ? 0UL : (uint32_t)ui_data.oil_height_x10;
  uint32_t water = ui_data.water_height_x10 < 0 ? 0UL : (uint32_t)ui_data.water_height_x10;
  int16_t temp = ui_data.temperature_x10;
  uint16_t temp_abs;
  if(oil > 99999UL) oil = 99999UL;
  if(water > 99999UL) water = 99999UL;
  format_fixed5(oil, top);
  top_show_digits(top, -1);
  set_point(P_TOP_DP9, true);
  alpha_show_fixed5(water);
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
  if(temp_abs > 99U) temp_abs = 99U;
  seven_write(&temp_digit[0], (char)('0' + temp_abs / 10U), true);
  seven_write(&temp_digit[1], (char)('0' + temp_abs % 10U), true);
  set_point(P_TEMP_ICON, true);
}

/*
 * 生成高油位报警阈值界面。
 * 顶部显示固定五位整数；右下显示 WHA；温度区域关闭。
 * 设置模式下根据 ui_selected_digit 让某一位闪烁。
 */
static void render_threshold_page(void)
{
  char top[5];
  int8_t hidden = -1;
  format_fixed5(ui_data.high_level_mm, top);
  if(ui_editing && !ui_blink_visible)
    hidden = (int8_t)(4U - ui_selected_digit);
  top_show_digits(top, hidden);
  set_point(P_TOP_DP9, false);
  alpha_show_right("WHA");
  seven_write(&temp_digit[0], ' ', true);
  seven_write(&temp_digit[1], ' ', true);
  set_point(P_TEMP_MINUS, false);
  set_point(P_TEMP_ICON, false);
}

/*
 * 生成高油位报警开关界面。
 * 顶部显示 ON/OFF，右下显示 WHA，设置模式下整个单词闪烁。
 */
static void render_enable_page(void)
{
  bool visible = !(ui_editing && !ui_blink_visible);
  top_show_word(ui_data.high_level_enable ? "ON" : "OFF", visible);
  set_point(P_TOP_DP9, false);
  alpha_show_right("WHA");
  seven_write(&temp_digit[0], ' ', true);
  seven_write(&temp_digit[1], ' ', true);
  set_point(P_TEMP_MINUS, false);
  set_point(P_TEMP_ICON, false);
}

/*
 * 绘制顶部20格进度条。
 * 0%=0格，100%=20格；非0百分比采用向上取整，避免小值完全不显示。
 */
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

/*
 * 重新生成一整屏“目标缓存”。
 * 这里只修改 RAM 缓存，不在此处一次性发送全部 32 个 SEG。
 */
static void render_page(void)
{
  HT16220_Clear();
  switch(ui_page)
  {
    case LCD_PAGE_MAIN: render_main_page(); break;
    case LCD_PAGE_HIGH_LEVEL_VALUE: render_threshold_page(); break;
    case LCD_PAGE_HIGH_LEVEL_ENABLE: render_enable_page(); break;
    default: break;
  }
  render_progress_bar();
}

/* 切换到下一页面，末页后回到主界面。 */
static void page_next(void)
{
  ui_page = (LCD_Page_t)((ui_page + 1U) % LCD_PAGE_COUNT);
  ui_render_pending = true;
}

/* 切换到上一页面，主界面向上后跳到最后一页。 */
static void page_prev(void)
{
  if(ui_page == LCD_PAGE_MAIN)
    ui_page = LCD_PAGE_HIGH_LEVEL_ENABLE;
  else
    ui_page = (LCD_Page_t)(ui_page - 1U);
  ui_render_pending = true;
}

/*
 * 当前选中位加 1，9 后回到 0，其他位保持不变。
 * 例如 01234，选中十位后：01234 -> 01244。
 */
static void threshold_increment_selected(void)
{
  static const uint32_t power10[5] = {1UL,10UL,100UL,1000UL,10000UL};
  uint32_t base;
  uint32_t digit;
  uint32_t value;
  if(ui_selected_digit >= 5U)
    ui_selected_digit = 0U;
  base = power10[ui_selected_digit];
  value = ui_data.high_level_mm;
  digit = (value / base) % 10UL;
  if(digit >= 9UL)
    value -= 9UL * base;
  else
    value += base;
  if(value > 99999UL)
    value = 99999UL;
  ui_data.high_level_mm = value;
}

/* 初始化全部 UI 状态，默认进入主界面。 */
void LCD_UI_Init(void)
{
  memset(&ui_data, 0, sizeof(ui_data));
  ui_data.high_level_mm = 1000UL;
  ui_data.progress_percent = 0U;
  ui_page = LCD_PAGE_MAIN;
  ui_editing = false;
  ui_selected_digit = 0;
  ui_blink_tick = 0;
  ui_blink_toggle_pending = false;
  ui_inactive_tick = 0U;
  ui_return_main_pending = false;
  ui_blink_visible = true;
  ui_render_pending = true;
  HT16220_Init();
}

/*
 * 10ms 定时入口。
 * 注意：中断里不调用 render_page、不操作 HT16220 GPIO，只产生闪烁事件。
 */
void LCD_UI_Tick10ms(void)
{
  /* 非主界面下统计无按键时间，满10秒后只置返回标志。 */
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

  /* 只有设置模式需要闪烁计时。 */
  if(!ui_editing)
    return;
  ui_blink_tick++;
  if(ui_blink_tick >= LCD_BLINK_TICKS)
  {
    ui_blink_tick = 0U;
    ui_blink_toggle_pending = true;
  }
}

/*
 * 主循环任务：
 * 1. 消费中断产生的闪烁事件；
 * 2. 需要时重新生成显示缓存；
 * 3. 每次只发送一个发生变化的 SEG。
 */
void LCD_UI_Task(void)
{
  /* 真正的页面切换在主循环执行，避免10ms中断中操作显示状态和GPIO。 */
  if(ui_return_main_pending)
  {
    ui_return_main_pending = false;
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
  HT16220_FlushStep(); /* 一次只发一个SEG字节，限制单次main占用时间 */
}

/*
 * 处理已经完成消抖的按键事件。
 * 非设置状态：UP/DOWN 切页面，SET 在页面 2/3 进入设置。
 * 阈值设置：UP 当前位+1，DOWN 移动编辑位，SET 保存退出。
 * 开关设置：UP/DOWN 切换 ON/OFF，SET 保存退出。
 */
void LCD_UI_KeyEvent(LCD_KeyEvent_t key)
{
  /* 任意按键都重新开始10秒无操作计时。 */
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
      ui_editing = true;
      ui_selected_digit = 0;
      ui_blink_tick = 0;
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
    LCD_UI_ConfigChangedCallback(ui_data.high_level_mm, ui_data.high_level_enable);
  }
  else if(ui_page == LCD_PAGE_HIGH_LEVEL_VALUE)
  {
    if(key == LCD_KEY_UP)
      threshold_increment_selected();
    else if(key == LCD_KEY_DOWN)
      ui_selected_digit = (uint8_t)((ui_selected_digit + 1U) % 5U);
    ui_blink_visible = true;
    ui_blink_tick = 0;
    ui_render_pending = true;
  }
  else if(ui_page == LCD_PAGE_HIGH_LEVEL_ENABLE)
  {
    if(key == LCD_KEY_UP || key == LCD_KEY_DOWN)
      ui_data.high_level_enable = !ui_data.high_level_enable;
    ui_blink_visible = true;
    ui_blink_tick = 0;
    ui_render_pending = true;
  }
}

/*
 * 更新实时测量值。
 * 数据没有变化时直接返回，避免重复重绘。
 */
void LCD_UI_SetMeasurements(int32_t oil_x10, int16_t temp_x10, int32_t water_x10)
{
  if(ui_data.oil_height_x10 == oil_x10 && ui_data.temperature_x10 == temp_x10 && ui_data.water_height_x10 == water_x10)
    return;
  ui_data.oil_height_x10 = oil_x10;
  ui_data.temperature_x10 = temp_x10;
  ui_data.water_height_x10 = water_x10;
  if(ui_page == LCD_PAGE_MAIN)
    ui_render_pending = true;
}

/* 装载报警阈值和启用状态，并限制阈值最大为 99999。 */
void LCD_UI_SetHighLevelConfig(uint32_t threshold_mm, bool enable)
{
  if(threshold_mm > 99999UL)
    threshold_mm = 99999UL;
  ui_data.high_level_mm = threshold_mm;
  ui_data.high_level_enable = enable;
  if(ui_page != LCD_PAGE_MAIN)
    ui_render_pending = true;
}

/*
 * 设置顶部进度条百分比。
 * 数据没有变化时不重新绘制，避免无意义刷新。
 */
void LCD_UI_SetProgressPercent(uint8_t percent)
{
  if(percent > 100U)
    percent = 100U;
  if(ui_data.progress_percent == percent)
    return;
  ui_data.progress_percent = percent;
  ui_render_pending = true;
}

/* 向调用者复制当前数据，传入空指针时不处理。 */
void LCD_UI_GetData(LCD_UI_Data_t *data)
{
  if(data != 0)
    *data = ui_data;
}

/* 返回当前页面编号。 */
LCD_Page_t LCD_UI_GetPage(void)
{
  return ui_page;
}

/* 返回是否处于设置模式。 */
bool LCD_UI_IsEditing(void)
{
  return ui_editing;
}

/*
 * 默认弱回调。使用 GCC 时，用户可以在其他 .c 文件中定义同名函数覆盖它。
 * Keil/ARMCC 若不支持此写法，可删除这里的默认实现，直接在用户文件中实现。
 */
#if defined(__GNUC__)
__attribute__((weak))
#endif
void LCD_UI_ConfigChangedCallback(uint32_t threshold_mm, bool enable)
{
  (void)threshold_mm;
  (void)enable;
}

