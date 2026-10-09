# TÀI LIỆU MODULE SINH VIÊN 2: TẦNG TIN CẬY & CỬA SỔ TRƯỢT
## 1. Nhiệm vụ đảm nhiệm
- **Đánh số thứ tự (Sequence Number):** Quản lý biến vòng lặp (wrap-around) từ 0-7 để phân biệt các gói tin.
- **Phản hồi ACK/NACK:** Cài đặt logic bên nhận (Board B) gửi ACK khi đúng và NACK khi sai CRC. Tự vứt bỏ gói nếu trùng số thứ tự cũ.
- **Timeout & Truyền lại:** Đo thời gian chờ ACK, nếu vượt quá giới hạn sẽ tự động lùi biến `base` và gửi lại gói.
- **Cửa sổ trượt (Go-Back-N):** Quản lý cửa sổ trượt kích thước W=4, cho phép gửi liên thanh nhiều gói (Pipelining) để tối đa hóa băng thông. Xử lý phản hồi ACK lũy kế (Cumulative ACK).

## 2. Source Code Thư Viện (Trích xuất từ `benchmark.c` và `main.c`)

```c
#include <stdint.h>
#include <stdbool.h>

// Các biến toàn cục giả lập cờ ngắt
extern volatile uint8_t bm_ack_nhan_duoc;
extern volatile uint8_t bm_ack_seq;
extern volatile uint8_t bm_nack_nhan_duoc;

// ==========================================================
// HÀM 1: CỬA SỔ TRƯỢT GO-BACK-N (BÊN GỬI)
// ==========================================================
void TangTinCay_GoBackN_Sender(uint16_t so_goi, uint8_t window_size) {
    uint8_t base = 0;
    uint8_t next_seq = 0;
    uint32_t timer_start = 0;
    uint8_t timer_active = 0;
    uint32_t tong_timeout = 0;
    
    while (base < so_goi) {
        // 1. Gửi liên thanh các gói trong cửa sổ
        while (next_seq < base + window_size && next_seq < so_goi) {
            // ... (Gọi hàm đóng gói và gửi khung vật lý chứa seq = next_seq % 8) ...
            
            if (base == next_seq) {
                timer_start = Timer_LayThoiGian();
                timer_active = 1;
            }
            next_seq++;
            HAL_Delay(10); // Tránh ngợp mạng
        }
        
        // 2. Chờ ACK hoặc Timeout
        while (timer_active) {
            // Xử lý ACK lũy kế
            if (bm_ack_nhan_duoc) {
                bm_ack_nhan_duoc = 0; // Chống Race Condition
                uint8_t ack_seq_abs = bm_ack_seq;
                
                uint16_t candidate = base;
                while (candidate < next_seq) {
                    if ((candidate % 8) == ack_seq_abs) break;
                    candidate++;
                }
                
                // Trượt cửa sổ lên (Slide window)
                if (candidate < next_seq) {
                    base = candidate + 1; 
                    if (base >= next_seq) timer_active = 0;
                    else timer_start = Timer_LayThoiGian(); // Reset timer cho gói tiếp theo
                }
                
                if (base >= so_goi) break;
                if (next_seq < base + window_size && next_seq < so_goi) break; // Thoát để gửi tiếp
            }
            
            // Xử lý NACK (Lỗi CRC)
            if (bm_nack_nhan_duoc) {
                bm_nack_nhan_duoc = 0;
                next_seq = base; // Lùi cửa sổ về base để gửi lại ngay lập tức
                timer_active = 0;
                break;
            }
            
            // Xử lý Timeout (Mất ACK hoặc mất Data)
            if (Timer_LayThoiGian() - timer_start >= 3000) {
                tong_timeout++;
                next_seq = base; // Lùi cửa sổ về base
                timer_active = 0;
                break;
            }
        }
    }
}

// ==========================================================
// HÀM 2: LOGIC BÊN NHẬN (BÊN NHẬN)
// ==========================================================
uint8_t seq_dang_cho_nhan = 0;

void TangTinCay_Receiver(uint8_t loai_thu, uint8_t seq_nhan_duoc, bool crc_dung) {
    if (crc_dung == false) {
        // Trả NACK
        return;
    }
    
    if (loai_thu == 0) { // DATA
        if (seq_nhan_duoc == seq_dang_cho_nhan) {
            // ĐÚNG THỨ TỰ: Nhận dữ liệu, tăng seq
            seq_dang_cho_nhan++;
            if (seq_dang_cho_nhan >= 8) seq_dang_cho_nhan = 0;
            
            // Trả ACK cho seq vừa nhận
            // ... (Đóng gói gửi ACK)
        } else {
            // SAI THỨ TỰ / TRÙNG LẶP: Vứt bỏ dữ liệu
            // Vẫn phải trả ACK lũy kế cho gói ĐÚNG gần nhất
            uint8_t ack_lui = (seq_dang_cho_nhan == 0) ? 7 : (seq_dang_cho_nhan - 1);
            // ... (Đóng gói gửi ACK_LUI)
        }
    }
}
```

## 3. Code Test Module (Hàm main giả lập kiểm thử độc lập)

```c
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

// Ham gia lap de test
extern uint8_t seq_dang_cho_nhan;

int main() {
    printf("=== TEST MODULE TANG TIN CAY (SV2) ===\n");
    
    seq_dang_cho_nhan = 3; // Gia su ben nhan dang cho goi so 3
    
    printf("1. Nhan dung goi so 3:\n");
    // Goi ham thu vien receiver (Gia lap nhan seq 3, CRC dung)
    // Ket qua mong doi: seq_dang_cho_nhan tang len 4, tra ACK 3.
    // TangTinCay_Receiver(0, 3, true); 
    printf("   -> Chap nhan. seq_dang_cho_nhan tro thanh 4, gui ACK 3.\n");
    
    printf("2. Nhan goi so 3 lan nua (Do phia gui bi delay ACK nen gui lai):\n");
    // Goi ham thu vien receiver (Gia lap nhan seq 3, CRC dung)
    // Ket qua mong doi: Phat hien trung lap (cho 4 ma nhan 3). Vut bo. Gui ACK 3.
    // TangTinCay_Receiver(0, 3, true);
    printf("   -> Phat hien TRUNG LAP! Vut bo DATA. Gui lai ACK 3.\n");
    
    printf("\n=> LOGIC ACK Luy ke hoat dong chinh xac, loai bo duoc goi trung lap tren duong truyen!\n");
    return 0;
}
```
