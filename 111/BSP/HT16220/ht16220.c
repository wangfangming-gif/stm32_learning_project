#include "../BSP/HT16220/ht16220.h"
#include <string.h>

/* HT16220 9 位命令码，具体格式见数据手册命令表。 */
#define HT16220_CMD_SYS_EN  0x002U
#define HT16220_CMD_LCD_ON  0x006U
#define HT16220_CMD_NORMAL  0x1C6U

/*
 * ht_ram：目标显示缓存，界面层只修改这里。
 * ht_sent_ram：记录已经发送到 HT16220 的内容。
 * 两者不同的 SEG 才会被真正发送，减少通信量。
 */
static uint8_t ht_ram[HT16220_SEG_COUNT];
static uint8_t ht_sent_ram[HT16220_SEG_COUNT];
static bool ht_sent_valid[HT16220_SEG_COUNT];
static uint8_t ht_flush_index;

/*
 * 向 HT16220 写入 1bit。
 * DATA 必须在 WR 上升沿前稳定，芯片在 WR 上升沿锁存数据。
 */
static void ht_write_bit(uint8_t bit)
{
  HT16220_PortWR(0);
  HT16220_PortDATA(bit ? 1U : 0U);
  HT16220_PortDelay();
  HT16220_PortWR(1);
  HT16220_PortDelay();
}

/*
 * 按最高位优先方式发送模式 ID、地址或命令。
 * 例如命令模式 ID 100、写模式 ID 101 都使用此函数。
 */
static void ht_write_bits_msb(uint16_t data, uint8_t bit_count)
{
  int8_t i;
  for(i = (int8_t)bit_count - 1; i >= 0; i--)
    ht_write_bit((uint8_t)((data >> i) & 0x01U));
}

/*
 * HT16220 的 RAM 数据位按照 D0、D1、D2、D3 顺序发送，
 * 因此 4bit 显示数据需要最低位优先发送。
 */
static void ht_write_nibble_lsb(uint8_t data)
{
  uint8_t i;
  for(i = 0; i < 4U; i++)
    ht_write_bit((uint8_t)((data >> i) & 0x01U));
}

/* 发送一条 9 位 HT16220 命令。 */
static void ht_send_command(uint16_t command)
{
  HT16220_PortCS(0);
  ht_write_bits_msb(0x04U, 3U); /* 命令模式ID：100 */
  ht_write_bits_msb(command & 0x01FFU, 9U);
  HT16220_PortCS(1);
  HT16220_PortDelay();
}

/*
 * 把一个 SEG 对应的 COM0~COM7 全部写入 HT16220。
 * HT16220 RAM 地址按 4bit 编址，所以一个 SEG 占两个地址：
 * 偶数地址保存 COM0~COM3，奇数地址保存 COM4~COM7。
 */
static void ht_write_seg_byte(uint8_t seg, uint8_t value)
{
  uint8_t address = (uint8_t)(seg << 1);
  HT16220_PortCS(0);
  ht_write_bits_msb(0x05U, 3U); /* 写模式ID：101 */
  ht_write_bits_msb(address, 6U);
  ht_write_nibble_lsb(value & 0x0FU);       /* COM0~COM3 */
  ht_write_nibble_lsb((value >> 4) & 0x0FU);/* COM4~COM7 */
  HT16220_PortCS(1);
  HT16220_PortDelay();
}

/* 初始化 GPIO 空闲电平、软件缓存和 HT16220 工作状态。 */
void HT16220_Init(void)
{
  HT16220_PortCS(1);
  HT16220_PortWR(1);
  HT16220_PortDATA(1);
  memset(ht_ram, 0, sizeof(ht_ram));
  memset(ht_sent_ram, 0, sizeof(ht_sent_ram));
  memset(ht_sent_valid, 0, sizeof(ht_sent_valid));
  ht_flush_index = 0;
  ht_send_command(HT16220_CMD_SYS_EN);
  ht_send_command(HT16220_CMD_NORMAL);
  ht_send_command(HT16220_CMD_LCD_ON);
}

/* 清空目标缓存；真正发送由 HT16220_FlushStep 分步完成。 */
void HT16220_Clear(void)
{
  memset(ht_ram, 0, sizeof(ht_ram));
}

/* 修改一个 SEG/COM 交点对应的缓存位。 */
void HT16220_SetPoint(uint8_t seg, uint8_t com, bool on)
{
  uint8_t mask;
  if(seg >= HT16220_SEG_COUNT || com >= 8U)
    return;
  mask = (uint8_t)(1U << com);
  if(on)
    ht_ram[seg] |= mask;
  else
    ht_ram[seg] &= (uint8_t)~mask;
}

/* 读取一个 SEG/COM 交点在目标缓存中的状态。 */
bool HT16220_GetPoint(uint8_t seg, uint8_t com)
{
  if(seg >= HT16220_SEG_COUNT || com >= 8U)
    return false;
  return (ht_ram[seg] & (uint8_t)(1U << com)) != 0U;
}

/* 直接覆盖某个 SEG 的 8 个 COM 位。 */
void HT16220_SetSegByte(uint8_t seg, uint8_t value)
{
  if(seg < HT16220_SEG_COUNT)
    ht_ram[seg] = value;
}

/* 读取某个 SEG 的 8 位缓存。 */
uint8_t HT16220_GetSegByte(uint8_t seg)
{
  if(seg >= HT16220_SEG_COUNT)
    return 0;
  return ht_ram[seg];
}

/*
 * 人为制造目标缓存和已发送缓存的差异，
 * 使后续 FlushStep 把整个屏幕重新发送一遍。
 */
void HT16220_RequestFullRefresh(void)
{
  memset(ht_sent_valid, 0, sizeof(ht_sent_valid));
  ht_flush_index = 0;
}

/*
 * 在 32 个 SEG 中寻找第一个发生变化的 SEG 并发送。
 * 一次最多发送一个，保证主循环调用时间可控。
 */
bool HT16220_FlushStep(void)
{
  uint8_t count;
  for(count = 0; count < HT16220_SEG_COUNT; count++)
  {
    uint8_t seg = ht_flush_index;
    ht_flush_index++;
    if(ht_flush_index >= HT16220_SEG_COUNT)
      ht_flush_index = 0;
    if(!ht_sent_valid[seg] || ht_ram[seg] != ht_sent_ram[seg])
    {
      ht_write_seg_byte(seg, ht_ram[seg]);
      ht_sent_ram[seg] = ht_ram[seg];
      ht_sent_valid[seg] = true;
      return true;
    }
  }
  return false;
}

/* 比较两个缓存，判断是否仍有尚未发送的显示变化。 */
bool HT16220_IsFlushDone(void)
{
  uint8_t i;
  for(i = 0; i < HT16220_SEG_COUNT; i++)
  {
    if(!ht_sent_valid[i] || ht_ram[i] != ht_sent_ram[i])
      return false;
  }
  return true;
}

