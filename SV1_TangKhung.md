# TÀI LIỆU MODULE SINH VIÊN 1: TẦNG KHUNG & ĐỒNG BỘ
## 1. Nhiệm vụ đảm nhiệm
- **Đóng khung (Framing):** Gắn cờ `0x7E` (SOF/EOF) ở đầu và cuối để phân định ranh giới gói tin.
- **Mã hóa Byte Stuffing (COBS):** Mã hóa toàn bộ dữ liệu bên trong gói để đảm bảo không có bất kỳ byte nào vô tình trùng với cờ `0x7E`.
- **Bảo vệ toàn vẹn (CRC16):** Cài đặt thuật toán CRC16-CCITT chuẩn viễn thông để phát hiện lỗi sai lệch bit.
- **Bóc tách FSM tự đồng bộ:** Thiết kế Máy Trạng Thái (Finite State Machine) đọc từng byte nhận được từ UART, tự động lọc vứt rác bên ngoài khung, giải mã COBS, kiểm tra CRC và tự động đồng bộ (Resync) lại khi nhiễu.

## 2. Source Code Thư Viện (Trích xuất từ `reliable_protocol.c`)

```c
#include <stdint.h>
#include <stdbool.h>

// Định nghĩa cờ và mã hóa COBS
#define CO_NIEM_PHONG  0x7E
#define TUI_HOA_TRANG  0x7D
#define MA_BUP_BE_DO   0x5E  // Thay cho 0x7E
#define MA_MON_DO_XANH 0x5D  // Thay cho 0x7D

// Định nghĩa trạng thái Máy bóc tách (FSM)
typedef enum {
    TRANG_THAI_NGOAI_KHUNG = 0,
    TRANG_THAI_TRONG_KHUNG = 1,
    TRANG_THAI_HOA_TRANG   = 2
} TrangThaiNhan_t;

typedef enum {
    KETQUA_CHUA_XONG = 0,
    KETQUA_OK = 1,
    KETQUA_LOI_CRC = 2
} TrangThaiBocTach_t;

// Cấu trúc cái rổ đựng thư (Đã bóc tách thành công)
typedef struct {
    uint8_t loai_thu;
    uint8_t so_thu_tu;
    uint8_t do_dai;
    uint8_t ruot_thu[255];
} RoDungThu_t;


// ==========================================================
// HÀM 1: TÍNH TOÁN MÃ CRC16 (CCITT-FALSE)
// ==========================================================
uint16_t GiaoThuc_TinhCRC16(const uint8_t* du_lieu, uint16_t do_dai_du_lieu) {
    uint16_t crc = 0xFFFF; // Gia tri khoi tao
    for (uint16_t i = 0; i < do_dai_du_lieu; i++) {
        crc ^= (uint16_t)du_lieu[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021; // Đa thức CCITT
            else crc <<= 1;
        }
    }
    return crc;
}

// ==========================================================
// HÀM 2: ĐÓNG GÓI, CHÈN CRC & BYTE STUFFING COBS
// ==========================================================
void GiaoThuc_DongGoiThu(const RoDungThu_t* thu_vao, uint8_t* phong_bi_xuat, uint16_t* kich_thuoc_phong_bi) {
    uint16_t idx = 0;
    phong_bi_xuat[idx++] = CO_NIEM_PHONG; 
    
    uint8_t thung_tam[260];
    thung_tam[0] = thu_vao->loai_thu;
    thung_tam[1] = thu_vao->so_thu_tu;
    thung_tam[2] = thu_vao->do_dai;
    for (uint8_t i = 0; i < thu_vao->do_dai; i++) {
        thung_tam[3 + i] = thu_vao->ruot_thu[i];
    }
    
    uint16_t chieu_dai_truoc_crc = 3 + thu_vao->do_dai;
    uint16_t crc16 = GiaoThuc_TinhCRC16(thung_tam, chieu_dai_truoc_crc);
    thung_tam[chieu_dai_truoc_crc]     = (uint8_t)(crc16 >> 8); 
    thung_tam[chieu_dai_truoc_crc + 1] = (uint8_t)(crc16 & 0xFF);
    
    uint16_t tong_so_do_vat = chieu_dai_truoc_crc + 2;
    
    for (uint16_t i = 0; i < tong_so_do_vat; i++) {
        uint8_t mon_do = thung_tam[i];
        if (mon_do == CO_NIEM_PHONG) { 
            phong_bi_xuat[idx++] = TUI_HOA_TRANG;  
            phong_bi_xuat[idx++] = MA_BUP_BE_DO;   
        } 
        else if (mon_do == TUI_HOA_TRANG) { 
            phong_bi_xuat[idx++] = TUI_HOA_TRANG;  
            phong_bi_xuat[idx++] = MA_MON_DO_XANH; 
        } 
        else phong_bi_xuat[idx++] = mon_do; 
    }
    phong_bi_xuat[idx++] = CO_NIEM_PHONG;
    *kich_thuoc_phong_bi = idx;
}

// ==========================================================
// HÀM 3: BỘ MÁY TÁCH KHUNG TỰ ĐỒNG BỘ FSM
// ==========================================================
static TrangThaiNhan_t state = TRANG_THAI_NGOAI_KHUNG;
static uint8_t bo_dem_tam[260]; 
static uint16_t ngon_tay_tam = 0;

TrangThaiBocTach_t GiaoThuc_BocTachTungChu(uint8_t chu_cai_nhan, RoDungThu_t* ro_dung_ket_qua) {
    if (chu_cai_nhan == CO_NIEM_PHONG) {
        if (state == TRANG_THAI_TRONG_KHUNG && ngon_tay_tam >= 5) {
            uint8_t loai_doc = bo_dem_tam[0];
            uint8_t seq_doc = bo_dem_tam[1];
            uint8_t do_dai_doc = bo_dem_tam[2];
            uint8_t crc_msb = bo_dem_tam[ngon_tay_tam - 2];
            uint8_t crc_lsb = bo_dem_tam[ngon_tay_tam - 1];
            uint16_t crc_doc = ((uint16_t)crc_msb << 8) | crc_lsb;
            
            if (do_dai_doc == (ngon_tay_tam - 5)) {
                uint16_t crc_tu_tinh = GiaoThuc_TinhCRC16(bo_dem_tam, ngon_tay_tam - 2);
                if (crc_tu_tinh == crc_doc) {
                    ro_dung_ket_qua->loai_thu = loai_doc;
                    ro_dung_ket_qua->so_thu_tu = seq_doc;
                    ro_dung_ket_qua->do_dai = do_dai_doc;
                    for(uint8_t i = 0; i < do_dai_doc; i++) ro_dung_ket_qua->ruot_thu[i] = bo_dem_tam[3 + i];
                    state = TRANG_THAI_NGOAI_KHUNG; ngon_tay_tam = 0; 
                    return KETQUA_OK;       
                } else {
                    ro_dung_ket_qua->loai_thu = loai_doc; 
                    ro_dung_ket_qua->so_thu_tu = seq_doc;
                    state = TRANG_THAI_NGOAI_KHUNG; ngon_tay_tam = 0; 
                    return KETQUA_LOI_CRC;
                }
            }
        }
        state = TRANG_THAI_TRONG_KHUNG; ngon_tay_tam = 0; 
        return KETQUA_CHUA_XONG;
    }      
    
    switch (state) {
        case TRANG_THAI_TRONG_KHUNG:
            if (chu_cai_nhan == TUI_HOA_TRANG) state = TRANG_THAI_HOA_TRANG;
            else if (ngon_tay_tam < 260) bo_dem_tam[ngon_tay_tam++] = chu_cai_nhan;
            break;
            
        case TRANG_THAI_HOA_TRANG:
            if (chu_cai_nhan == MA_BUP_BE_DO) {
                if (ngon_tay_tam < 260) bo_dem_tam[ngon_tay_tam++] = CO_NIEM_PHONG;
            } else if (chu_cai_nhan == MA_MON_DO_XANH) {
                if (ngon_tay_tam < 260) bo_dem_tam[ngon_tay_tam++] = TUI_HOA_TRANG;
            }
            state = TRANG_THAI_TRONG_KHUNG;
            break;
            
        case TRANG_THAI_NGOAI_KHUNG:
        default:
            break;
    }
    return KETQUA_CHUA_XONG;
}
```

## 3. Code Test Module (Hàm main giả lập kiểm thử độc lập trên máy tính)

```c
#include <stdio.h>

int main() {
    printf("=== TEST MODULE TANG KHUNG (SV1) ===\n");
    
    // 1. Tao 1 goi tin DATA chua chu 'A' va co chua byte trung co 0x7E de test COBS
    RoDungThu_t thu_gui = {0, 5, 2, {0x7E, 'A'}}; 
    uint8_t mang_uart[50];
    uint16_t size_uart = 0;
    
    // Dong goi
    GiaoThuc_DongGoiThu(&thu_gui, mang_uart, &size_uart);
    printf("Da dong goi xong. Kich thuoc UART: %d bytes\n", size_uart);
    
    // 2. Gia lap ben Nhan, co chen them Vai Byte Rac (0xFF, 0xAA) vao giua qua trinh nhan
    RoDungThu_t thu_nhan;
    TrangThaiBocTach_t kq;
    
    printf("Day vao FSM: [Rac] ");
    GiaoThuc_BocTachTungChu(0xFF, &thu_nhan); // Rac
    GiaoThuc_BocTachTungChu(0xAA, &thu_nhan); // Rac
    
    printf("[Data Frame] ");
    for(int i = 0; i < size_uart; i++) {
        kq = GiaoThuc_BocTachTungChu(mang_uart[i], &thu_nhan);
    }
    
    if (kq == KETQUA_OK) {
        printf("\n=> BOC TACH THANH CONG! Loai: %d, Seq: %d. FSM tu loc rac hoan hao!\n", 
               thu_nhan.loai_thu, thu_nhan.so_thu_tu);
    }
    return 0;
}
```
