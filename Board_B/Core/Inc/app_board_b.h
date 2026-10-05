#ifndef APP_BOARD_B_H
#define APP_BOARD_B_H

#include <stdint.h>

// ====================================================================
// MODULE UNG DUNG BOARD B (BEN NHAN)
// Chua toan bo logic: Nhan DATA, tra ACK/NACK, OLED thong ke
// ====================================================================

// Khoi tao ung dung Board B (OLED, in thong diep chao)
void App_BoardB_Init(void);

// Vong lap chinh cua Board B (goi trong while(1) cua main.c)
void App_BoardB_Loop(void);

// Ham xu ly ngat nhan UART (goi tu USART1_IRQHandler trong uart_driver.c)
void UART_HamNgatNhan(uint8_t chu_cai_nhan);

#endif // APP_BOARD_B_H
