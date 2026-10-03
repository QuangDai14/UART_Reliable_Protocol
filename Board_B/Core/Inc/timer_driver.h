#ifndef TIMER_DRIVER_H
#define TIMER_DRIVER_H

#include <stdint.h>

// 1. Ham khoi tao dong ho SysTick (1 mili-giay keu chuong 1 lan)
void Timer_KhoiTao(void);

// 2. Ham hoi gio (Lay so mili-giay da troi qua tu luc cam dien)
uint32_t Timer_LayThoiGian(void);

#endif // TIMER_DRIVER_H