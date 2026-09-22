#ifndef APP_INTEGRATION_H
#define APP_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>

/* 
 * MODULE 4: APPLICATION & INTEGRATION
 * Tích hợp các module trên, định nghĩa ứng dụng cụ thể 
 * (VD: Đọc cảm biến giả lập và gửi đi).
 * Tác giả: Sinh viên 4
 */

// Hàm khởi tạo ứng dụng (Gọi tất cả các hàm init của các module khác)
void App_Init(void);

// Vòng lặp chính của ứng dụng (Thay thế cho code trong while(1) của main.c)
void App_Run(void);

// Hàm gửi dữ liệu từ Application (sẽ dùng Protocol_PackData và UART_SendRaw, kèm Timer)
bool App_SendReliableData(const uint8_t* data, uint8_t length);

// Callback xử lý dữ liệu khi nhận thành công một gói tin tin cậy
void App_OnValidDataReceived(const uint8_t* data, uint8_t length);

#endif // APP_INTEGRATION_H
