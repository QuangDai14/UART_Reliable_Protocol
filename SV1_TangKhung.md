# TÀI LIỆU MODULE SINH VIÊN 1: TẦNG KHUNG & ĐỒNG BỘ
## 1. Nhiệm vụ đảm nhiệm
- **Đóng khung (Framing):** Gắn cờ `0x7E` (SOF/EOF) ở đầu và cuối để phân định ranh giới gói tin.
- **Mã hóa Byte Stuffing (COBS):** Mã hóa toàn bộ dữ liệu bên trong gói để đảm bảo không có bất kỳ byte nào vô tình trùng với cờ `0x7E`.
- **Bảo vệ toàn vẹn (CRC16):** Cài đặt thuật toán CRC16-CCITT chuẩn viễn thông để phát hiện lỗi sai lệch bit.
- **Bóc tách FSM tự đồng bộ:** Thiết kế Máy Trạng Thái (Finite State Machine) đọc từng byte nhận được từ UART, tự động lọc vứt rác bên ngoài khung, giải mã COBS, kiểm tra CRC và tự động đồng bộ (Resync) lại khi nhiễu.

## 2. Source Code Thư Viện (Trích xuất từ `reliable_protocol.c`)

```c

    uint16_t chieu_dai_truoc_crc = 3 + thu_vao->do_dai;
    uint16_t crc16 = GiaoThuc_TinhCRC16(thung_tam, chieu_dai_truoc_crc);
    /* Thứ tự CRC trên đường truyền: byte cao trước, byte thấp sau. */
    thung_tam[chieu_dai_truoc_crc]     = (uint8_t)(crc16 >> 8);
    thung_tam[chieu_dai_truoc_crc + 1] = (uint8_t)(crc16 & 0xFF);

    uint16_t tong_so_do_vat = chieu_dai_truoc_crc + 2;
    /* Escape toàn bộ nội dung, bao gồm header và CRC, không chỉ payload. */
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
        else phong_bi_xuat[idx++] = mon_do; /* Byte thường được giữ nguyên. */
    }
    phong_bi_xuat[idx++] = CO_NIEM_PHONG; /* Cờ đóng không bị escape. */
    *kich_thuoc_phong_bi = idx; /* Ghi kết quả thông qua con trỏ. */
}

/* static ở phạm vi file: chỉ truy cập trong file này và giữ giá trị giữa
 * các lần gọi. Nhờ đó có thể cấp từng byte dù UART nhận rời rạc.
 */
static TrangThaiNhan_t state = TRANG_THAI_NGOAI_KHUNG;
static uint8_t bo_dem_tam[260]; /* Chứa nội dung SAU khi bỏ escape, không có cờ. */
static uint16_t ngon_tay_tam = 0; /* Số byte đã lưu / vị trí ghi tiếp theo. */

/* Gọi một lần cho mỗi byte UART nhận được, đúng thứ tự.
 * Chỉ đọc toàn bộ ro_dung_ket_qua khi hàm trả KETQUA_OK.
 * CHUA_XONG không đảm bảo rằng mọi byte đã nhận đều đúng.
 */
TrangThaiBocTach_t GiaoThuc_BocTachTungChu(uint8_t chu_cai_nhan, RoDungThu_t* ro_dung_ket_qua) {
    /* Kiểm tra cờ trước switch: kể cả khi đang chờ escape, cờ vẫn giúp
     * bỏ khung dở và bắt đầu lại. */
    if (chu_cai_nhan == CO_NIEM_PHONG) {
        /* Nội dung tối thiểu: TYPE + SEQ + LEN + CRC_H + CRC_L = 5 byte. */
        if (state == TRANG_THAI_TRONG_KHUNG && ngon_tay_tam >= 5) {
            uint8_t loai_doc = bo_dem_tam[0];
            uint8_t seq_doc = bo_dem_tam[1];
            uint8_t do_dai_doc = bo_dem_tam[2];
            uint8_t crc_msb = bo_dem_tam[ngon_tay_tam - 2];
            uint8_t crc_lsb = bo_dem_tam[ngon_tay_tam - 1];
            /* Ghép hai byte CRC thành số 16 bit. */
            uint16_t crc_doc = ((uint16_t)crc_msb << 8) | crc_lsb;

            /* LEN phải khớp số byte payload thực tế: tổng trừ header và CRC. */
            if (do_dai_doc == (ngon_tay_tam - 5)) {
                /* Tính lại CRC, không đưa hai byte CRC nhận được vào phép tính. */
                uint16_t crc_tu_tinh = GiaoThuc_TinhCRC16(bo_dem_tam, ngon_tay_tam - 2);
                if (crc_tu_tinh == crc_doc) {
                    /* Chỉ nhánh này mới xuất cả header và payload hợp lệ theo CRC. */
                    ro_dung_ket_qua->loai_thu = loai_doc;
                    ro_dung_ket_qua->so_thu_tu = seq_doc;
                    ro_dung_ket_qua->do_dai = do_dai_doc;
                    for(uint8_t i = 0; i < do_dai_doc; i++) ro_dung_ket_qua->ruot_thu[i] = bo_dem_tam[3 + i];
                    /* Không cần xóa cả mảng: đặt số byte đã dùng về 0 là đủ. */
                    state = TRANG_THAI_NGOAI_KHUNG; ngon_tay_tam = 0;
                    return KETQUA_OK;
                } else {
                    /* Giữ hành vi gốc: chỉ trả TYPE/SEQ khi CRC sai.
                     * Hai trường này KHÔNG đáng tin; LEN/payload vẫn là dữ liệu cũ.
                     */
                    ro_dung_ket_qua->loai_thu = loai_doc;
                    ro_dung_ket_qua->so_thu_tu = seq_doc;
                    state = TRANG_THAI_NGOAI_KHUNG; ngon_tay_tam = 0;
                    return KETQUA_LOI_CRC;
                }
            }
        }
        /* Cờ đầu tiên, cờ lặp, khung ngắn, sai LEN hoặc escape dang dở:
         * lấy cờ hiện tại làm điểm bắt đầu khung mới và bỏ dữ liệu đang có.
         */
        state = TRANG_THAI_TRONG_KHUNG; ngon_tay_tam = 0;
        return KETQUA_CHUA_XONG;
    }

    switch (state) {
        case TRANG_THAI_TRONG_KHUNG:
            /* 7D không được lưu; chuyển sang chờ mã thay thế. */
            if (chu_cai_nhan == TUI_HOA_TRANG) state = TRANG_THAI_HOA_TRANG;
            else if (ngon_tay_tam < 260) bo_dem_tam[ngon_tay_tam++] = chu_cai_nhan;
            /* Nếu đã đầy: byte bị bỏ qua. Có chặn ghi ngoài mảng nhưng
             * chưa đánh dấu lỗi để loại bỏ toàn bộ khung quá dài. */
            break;

        case TRANG_THAI_HOA_TRANG:
            /* Khôi phục byte đặc biệt ban đầu trước khi kiểm tra CRC. */
            if (chu_cai_nhan == MA_BUP_BE_DO) {
                if (ngon_tay_tam < 260) bo_dem_tam[ngon_tay_tam++] = CO_NIEM_PHONG;
            } else if (chu_cai_nhan == MA_MON_DO_XANH) {
                if (ngon_tay_tam < 260) bo_dem_tam[ngon_tay_tam++] = TUI_HOA_TRANG;
            }
            /* Mã khác 5E/5D cũng quay về nhận thường, không báo lỗi.
             * Vì thế một cặp escape sai có thể bị loại đi âm thầm. */
            state = TRANG_THAI_TRONG_KHUNG;
            break;

        case TRANG_THAI_NGOAI_KHUNG:
        default:
            break; /* Bỏ byte thường ngoài khung; không lọc rác tùy ý trong khung. */
    }
    return KETQUA_CHUA_XONG;
}

```

## 3. Code Test Module (Hàm main giả lập kiểm thử độc lập trên máy tính)

```c
/* KIỂM THỬ MODULE TẦNG KHUNG TRÊN PC
 * Đặt main_test.c và giao_thuc_uart.c trong cùng thư mục.
 * Biên dịch: gcc -std=c99 -Wall -Wextra main_test.c -o uart_demo
 * Chạy Linux/macOS: ./uart_demo ; Windows: uart_demo.exe
 * Chỉ biên dịch main_test.c: file thư viện đã được include bên dưới.
 * Không biên dịch thêm giao_thuc_uart.c trong cùng lệnh vì sẽ trùng định nghĩa.
 */
#include "giao_thuc_uart.c"

#include <stdio.h>

/* Main giả lập của người dùng, giữ nguyên logic.
 * Không mở cổng COM/UART thật; mảng mang_uart đóng vai trò đường truyền.
 */
int main() {
    printf("=== TEST MODULE TANG KHUNG (SV1) ===\n");

    /* TYPE=0 (demo gọi là DATA), SEQ=5, LEN=2, payload={7E,41}.
     * 'A' có mã 0x41 trong môi trường ASCII thông dụng.
     */
    RoDungThu_t thu_gui = {0, 5, 2, {0x7E, 'A'}};
    uint8_t mang_uart[50]; /* Đủ cho mẫu này; không đủ cho mọi LEN đến 255. */
    uint16_t size_uart = 0;

    GiaoThuc_DongGoiThu(&thu_gui, mang_uart, &size_uart);
    printf("Da dong goi xong. Kich thuoc UART: %d bytes\n", size_uart);

    RoDungThu_t thu_nhan;
    TrangThaiBocTach_t kq;

    /* Hai byte rác được chèn TRƯỚC cờ mở, không phải ở giữa nội dung khung. */
    printf("Day vao FSM: [Rac] ");
    GiaoThuc_BocTachTungChu(0xFF, &thu_nhan);
    GiaoThuc_BocTachTungChu(0xAA, &thu_nhan);

    printf("[Data Frame] ");
    for(int i = 0; i < size_uart; i++) {
        kq = GiaoThuc_BocTachTungChu(mang_uart[i], &thu_nhan);
        /* Demo chỉ xét kq sau vòng lặp vì mảng kết thúc đúng ở cờ đóng.
         * Ứng dụng thực phải kiểm tra kq sau MỖI byte để không bỏ lỡ gói.
         */
    }

    if (kq == KETQUA_OK) {
        /* Giữ nguyên thông báo gốc. Từ "hoan hao" chỉ là thông báo demo,
         * không phải kết luận rằng mọi trường hợp nhiễu đều được xử lý đúng.
         */
        printf("\n=> BOC TACH THANH CONG! Loai: %d, Seq: %d. FSM tu loc rac hoan hao!\n",
               thu_nhan.loai_thu, thu_nhan.so_thu_tu);
    }
    return 0;
}
```
