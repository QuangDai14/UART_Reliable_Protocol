#ifndef APP_BOARD_A_H
#define APP_BOARD_A_H

#include <stdint.h>

// ====================================================================
// MODULE UNG DUNG BOARD A (Application Layer)
// Chua toan bo logic: Go-Back-N, Doc DHT11, Quet nut, OLED, Benchmark
// ====================================================================

// Khoi tao ung dung Board A (OLED, DHT11, in thong diep chao)
void App_BoardA_Init(void);

// Vong lap chinh cua Board A (goi trong while(1) cua main.c)
void App_BoardA_Loop(void);

// Ham xu ly ngat nhan UART (goi tu USART1_IRQHandler trong uart_driver.c)
void UART_HamNgatNhan(uint8_t chu_cai_nhan);

#endif // APP_BOARD_A_H
