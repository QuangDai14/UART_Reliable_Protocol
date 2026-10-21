/* ============================================================================
 * TÊN FILE       : delay.c
 * PHỤ TRÁCH      : DÙNG CHUNG CẢ NHÓM (Hàm Delay Phần Mềm)
 * CHỨC NĂNG CHÍNH: Hàm delay bằng vòng lặp for thay thế HAL_Delay để chống xung đột ngắt SysTick trong các hàm nhạy cảm.
 * CÁC API CHÍNH  : delay_ms()
 * ============================================================================ */
#include "delay.h"

// ==============================================================================
// HAM DELAY BANG VONG LAP PHAN MEM (THAY THE HAL_Delay)
// Moi lan lap ~6000 vong tuong duong ~1ms o tan so 72MHz (STM32F103)
// ==============================================================================
void delay_ms(unsigned int count) {
    while (count--) {
        for (volatile unsigned int i = 0; i < 6000; i++);
    }
}

