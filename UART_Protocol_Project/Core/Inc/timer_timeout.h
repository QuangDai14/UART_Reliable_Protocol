#ifndef TIMER_TIMEOUT_H
#define TIMER_TIMEOUT_H

#include <stdint.h>
#include <stdbool.h>

/* 
 * MODULE 3: TIMER & TIMEOUT MANAGER
 * Quản lý thời gian, timeout và quyết định việc truyền lại (Retransmission)
 * TUYỆT ĐỐI KHÔNG dùng HAL_Delay. Sử dụng ngắt SysTick hoặc Timer.
 * Tác giả: Sinh viên 3
 */

#define TIMEOUT_DURATION_MS 1000 // Chờ 1 giây để nhận ACK
#define MAX_RETRANSMIT_TRIES 3   // Gửi lại tối đa 3 lần

// Cấu trúc quản lý một Timer mềm (Software Timer)
typedef struct {
    uint32_t start_time_ms;
    uint32_t timeout_ms;
    bool is_active;
} SoftTimer_t;

// Khởi tạo Timer phần cứng bằng thanh ghi
void Timer_HardwareInit(void);

// Lấy thời gian hệ thống (bằng mili-giây)
uint32_t Timer_GetSystemTime(void);

// Bắt đầu một bộ đếm thời gian
void Timer_Start(SoftTimer_t* timer, uint32_t duration_ms);

// Dừng bộ đếm thời gian
void Timer_Stop(SoftTimer_t* timer);

// Kiểm tra xem timer đã hết hạn chưa (hàm này được gọi liên tục trong while 1)
// Trả về true nếu hết thời gian
bool Timer_IsExpired(SoftTimer_t* timer);

#endif // TIMER_TIMEOUT_H
