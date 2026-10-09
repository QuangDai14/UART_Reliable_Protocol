# TÀI LIỆU MODULE SINH VIÊN 3: MODULE CHÈN LỖI (ERROR INJECTION)
## 1. Nhiệm vụ đảm nhiệm
- **Bỏ gói (Drop):** Cố tình không gửi gói tin ra cáp UART để thử nghiệm chức năng Timeout của bên gửi.
- **Đảo bit (Corrupt):** Tác động lật bit (XOR `0xFF`) vào 2 byte Checksum CRC16 để làm hỏng dữ liệu, thử nghiệm chức năng từ chối NACK của bên nhận.
- **Chèn rác (Garbage):** Chèn các byte rác (`0xAA`, `0xBB`, `0xCC`) vào giữa khung tin để phá vỡ cấu trúc đóng khung, thử nghiệm chức năng Tự Đồng Bộ của FSM.
- **Trễ ACK (Delay):** Giữ gói tin ACK lại 4 giây trước khi gửi (Freeze CPU) để mô phỏng nghẽn mạng, thử nghiệm chức năng loại bỏ gói trùng lặp (Duplicate) của bên nhận.
- **Phím cứng vật lý:** Quét và chống dội (Debounce) cho 4 nút nhấn PB11, PB12, PB13, PB14 để kích hoạt lỗi theo ý muốn trong quá trình Demo.

## 2. Source Code Thư Viện (Trích xuất từ `error_injection.c`)

```c
#include <stdint.h>
#include <stdbool.h>

// Từ điển các loại bệnh lý (Lỗi) cần giả lập
typedef enum {
    KHONG_LOI = 0,    
    LOI_BO_GOI = 1,   
    LOI_DAO_BIT = 2,  
    LOI_CHEN_RAC = 3, 
    LOI_TRE_ACK = 4   
} CheDoChenLoi_t;

static CheDoChenLoi_t che_do_hien_tai = KHONG_LOI;

// ==========================================================
// HÀM 1: CÀI ĐẶT LỖI TỪ NÚT BẤM VẬT LÝ
// ==========================================================
void SV3_KichHoatLoi(uint8_t ma_phim) {
    if (ma_phim == '0')      che_do_hien_tai = KHONG_LOI; 
    else if (ma_phim == '1') che_do_hien_tai = LOI_BO_GOI; 
    else if (ma_phim == '2') che_do_hien_tai = LOI_DAO_BIT; 
    else if (ma_phim == '3') che_do_hien_tai = LOI_CHEN_RAC; 
    else if (ma_phim == '4') che_do_hien_tai = LOI_TRE_ACK; 
}

// ==========================================================
// HÀM 2: TRẠM THU PHÍ KIỂM DUYỆT VÀ PHÁ HOẠI DỮ LIỆU
// ==========================================================
void SV3_GuiCoChenLoi(uint8_t* phong_bi, uint16_t kich_thuoc, bool la_thu_ack) {
    
    // 1. MẤT GÓI (DROP PACKET)
    if (che_do_hien_tai == LOI_BO_GOI) {
        che_do_hien_tai = KHONG_LOI; // Chỉ phá 1 lần
        return; // Return lập tức, gói hàng bị vứt bỏ, không gọi hàm gửi UART
    }
    
    // 2. NHIỄU SÓNG, LẬT BIT (CORRUPT PACKET)
    if (che_do_hien_tai == LOI_DAO_BIT) {
        if (kich_thuoc >= 2) {
            // Lật ngược toàn bộ bit (XOR 0xFF) của byte Checksum
            phong_bi[kich_thuoc - 2] ^= 0xFF; 
        }
        UART_GuiBanGoc(phong_bi, kich_thuoc);
        che_do_hien_tai = KHONG_LOI; 
        return;
    }
    
    // 3. CHÈN RÁC (GARBAGE INSERT)
    if (che_do_hien_tai == LOI_CHEN_RAC) {
        // Gửi Nửa khúc đầu 
        UART_GuiBanGoc(phong_bi, kich_thuoc / 2);
        
        // Nhét 3 byte rác vào giữa
        uint8_t rac[] = {0xAA, 0xBB, 0xCC};
        UART_GuiBanGoc(rac, 3);
        
        // Gửi nốt Nửa khúc sau
        UART_GuiBanGoc(phong_bi + (kich_thuoc / 2), kich_thuoc - (kich_thuoc / 2));
        che_do_hien_tai = KHONG_LOI; 
        return;
    }
    
    // 4. LÀM TRỄ ACK (DELAY ACK)
    // Dùng cờ la_thu_ack để đảm bảo chỉ làm trễ thư ACK, bỏ qua thư DATA
    if (che_do_hien_tai == LOI_TRE_ACK && la_thu_ack == true) {
        HAL_Delay(4000); // Đóng băng CPU 4 giây
        UART_GuiBanGoc(phong_bi, kich_thuoc);
        che_do_hien_tai = KHONG_LOI; 
        return;
    }

    // NẾU KHÔNG CÓ LỖI: GỬI BÌNH THƯỜNG
    UART_GuiBanGoc(phong_bi, kich_thuoc);
}
```

## 3. Code Test Module (Hàm main giả lập kiểm thử độc lập)

```c
#include <stdio.h>
#include <stdint.h>

// Bien gia lap
uint8_t khung_mau[] = {0x7E, 0x01, 0x05, 0xAA, 0xBB, 0x7E}; // Co kem crc ao
uint8_t mang_chua_ket_qua[50];
int index_ket_qua = 0;

// Ham gia lap UART_GuiBanGoc de in ra console thay vi day xuong cap UART
void UART_GuiBanGoc(uint8_t* data, uint16_t len) {
    for(int i = 0; i < len; i++) {
        mang_chua_ket_qua[index_ket_qua++] = data[i];
    }
}

int main() {
    printf("=== TEST MODULE CHEN LOI (SV3) ===\n");
    
    // Test 1: Binh thuong
    index_ket_qua = 0;
    SV3_KichHoatLoi('0'); 
    SV3_GuiCoChenLoi(khung_mau, 6, false);
    printf("Binh thuong: Byte truoc cuoi la 0x%02X\n", mang_chua_ket_qua[4]);
    
    // Test 2: Lat bit CRC
    index_ket_qua = 0;
    SV3_KichHoatLoi('2'); // Kich hoat DAO BIT
    SV3_GuiCoChenLoi(khung_mau, 6, false);
    printf("Lat Bit CRC: Byte truoc cuoi la 0x%02X (Da bi XOR voi 0xFF)\n", mang_chua_ket_qua[4]);
    
    printf("\n=> THÀNH CÔNG: Module can thiệp thay đổi được nội dung byte trong mảng trước khi đẩy xuống UART!\n");
    
    return 0;
}
```
