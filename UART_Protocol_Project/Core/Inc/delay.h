/* ============================================================================
 * TÊN FILE       : delay.h
 * PHỤ TRÁCH      : DÙNG CHUNG (SV1: Vũ Trường An, SV2: Đồng Quang Đại, SV3: Nguyễn Duy Khánh, SV4: Nguyễn Văn Hùng)
 * CHỨC NĂNG CHÍNH: Khai báo prototype hàm delay_ms.
 * CÁC API CHÍNH  : delay_ms()
 * ============================================================================ */
#ifndef DELAY_H
#define DELAY_H

// Ham delay bang vong lap phan mem, thay the HAL_Delay
// count: so mili-giay can delay
void delay_ms(unsigned int count);

#endif // DELAY_H

