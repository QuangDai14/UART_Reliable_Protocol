#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/* 
 * MODULE 1: UART DRIVER
 * Phụ trách giao tiếp phần cứng UART ở mức thấp.
 * Tác giả: Sinh viên 1
 */

// Hàm khởi tạo UART (có thể dùng chung với cấu hình của CubeMX)
void UART_Init(void);

// Hàm gửi một mảng byte thô qua UART
// Trả về true nếu gửi thành công
bool UART_SendRaw(const uint8_t* data, uint16_t length);

// Hàm được gọi khi có ngắt nhận (Rx Interrupt) xảy ra.
// Hàm này sẽ đẩy byte nhận được sang cho State Machine của Module 2 xử lý.
void UART_RxCallback(uint8_t rxByte);

#endif // UART_DRIVER_H
