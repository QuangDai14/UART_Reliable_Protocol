/* ============================================================================
 * TÊN FILE       : delay.h
 * PHỤ TRÁCH      : DÙNG CHUNG CẢ NHÓM (Hàm Delay Phần Mềm)
 * CHỨC NĂNG CHÍNH: Khai báo prototype hàm delay_ms.
 * CÁC API CHÍNH  : delay_ms()
 * ============================================================================ */
#ifndef DELAY_H
#define DELAY_H

// Ham delay bang vong lap phan mem, thay the HAL_Delay
// count: so mili-giay can delay
void delay_ms(unsigned int count);

#endif // DELAY_H

