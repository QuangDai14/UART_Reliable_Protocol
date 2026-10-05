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
