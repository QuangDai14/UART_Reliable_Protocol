/* ============================================================================
 * TÊN FILE       : app_board_a.c
 * PHỤ TRÁCH      : SINH VIÊN 2 (Nguyễn Quang Đại) (Tầng Tin Cậy - Phía Phát (Sender))
 * CHỨC NĂNG CHÍNH: Khối điều khiển Mạch Phát (Board A): Thuật toán Go-Back-N (W=4, Seq 0-7), bắn liên thanh, bấm giờ Timeout 5s, trượt Cumulative ACK và xử lý NACK.
 * CÁC API CHÍNH  : App_BoardA_Init(), App_BoardA_Loop(), UART_HamNgatNhan()
 * ============================================================================ */
#include "app_board_a.h"
#include "uart_driver.h"
#include "reliable_protocol.h"
#include "timer_driver.h"
#include "error_injection.h"
#include "benchmark.h"
#include "delay.h"
#include "dht11.h"
#include "Anglas_OLED_SSD1306.h"
#include <stdio.h>
#include <string.h>
#include "main.h"

// ====================================================================
// ====================================================================
// DINH NGHIA HANG SO GO-BACK-N (PHAN CUA SINH VIEN 2)
// ====================================================================
// GBN_WINDOW_SIZE: Kích thước cửa sổ trượt (W = 4 gói).
// VI DU DE HIEU: Giống như một cái ống nước chứa được 4 quả bóng.
// Board A được phép bắn liên thanh tối đa 4 gói (Gói 0, 1, 2, 3) vào đường truyền
// mà CHƯA CẦN phải dừng lại chờ biên nhận (ACK).
#define GBN_WINDOW_SIZE   4

// GBN_TOTAL_PACKETS: Số gói gửi trong một phiên truyền (8 gói: từ Seq 0 đến 7).
// VI DU: Số thứ tự gói dùng 3 bit (2^3 = 8), đếm từ 0, 1, 2, 3, 4, 5, 6, 7 rồi quay vòng lại 0.
#define GBN_TOTAL_PACKETS 8

#define BENCHMARK_SO_GOI  104

// ====================================================================
// BIEN TOAN CUC CUA UNG DUNG BOARD A (QUAN LY CUA SO TRUOT)
// ====================================================================
// VI DU DE HIEU VE 2 BIEN "base" VA "next_seq_num":
// Hãy tưởng tượng một hàng 8 gói bánh được đánh số từ [0] đến [7]:
// - "base"        : Ngón tay trỏ vào gói đầu tiên CHƯA nhận được tiền (chưa có ACK).
// - "next_seq_num": Ngón tay trỏ vào gói TIẾP THEO chuẩn bị ném đi.
// Khoảng cách giữa 2 ngón tay tối đa là 4 (kích thước cửa sổ W=4).
static uint8_t base = 0;              // Đáy cửa sổ (gói cũ nhất đang chờ ACK)
static uint8_t next_seq_num = 0;       // Đỉnh cửa sổ (gói kế tiếp sẽ bắn đi)
static uint32_t thoi_gian_bat_dau_gui = 0; // Lưu mốc thời gian (ms) khi bắt đầu bấm giờ Timeout
static bool dang_chay_timer = false;   // Cờ báo hiệu đồng hồ Timeout có đang đếm không

static uint8_t seq_dang_cho_nhan = 0;

// --- BIEN THONG KE BOARD A ---
static uint32_t tong_goi_gui     = 0;
static uint32_t tong_lan_timeout  = 0;

// --- DU LIEU CAM BIEN DE GUI DI ---
static uint8_t nhiet_do_gui = 0;
static uint8_t do_am_gui    = 0;

static RoDungThu_t cai_ro_cua_toi;

// ====================================================================
// HAM XU LY NGAT NHAN UART (GOI TU uart_driver.c)
// ====================================================================
void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    TrangThaiBocTach_t is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    
    if (is_done == KETQUA_OK) {
        // MACH B (Nhan DATA)
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            char msg[150];
            if (cai_ro_cua_toi.so_thu_tu == seq_dang_cho_nhan) {
                sprintf(msg, "\r\n[MACH B] Nhan DUNG THU TU (Seq %d). Lay ra xai!\r\n", cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                seq_dang_cho_nhan++; 
            } else {
                sprintf(msg, "\r\n[MACH B] SAI THU TU! Dang doi Seq %d ma lai nhan Seq %d. Vut data!\r\n", seq_dang_cho_nhan, cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            }
            
            // Go-Back-N gui ACK Luy ke
            if (seq_dang_cho_nhan > 0) {
                RoDungThu_t thu_ack;
                thu_ack.loai_thu = THU_ACK;
                thu_ack.so_thu_tu = seq_dang_cho_nhan - 1; 
                thu_ack.do_dai = 0;
                
                uint8_t phong_bi_xuat[50];
                uint16_t kich_thuoc_phong_bi = 0;
                GiaoThuc_DongGoiThu(&thu_ack, phong_bi_xuat, &kich_thuoc_phong_bi); 
                
                // Goi thu vien SV3 de kiem soat viec chen loi
                SV3_GuiCoChenLoi(phong_bi_xuat, kich_thuoc_phong_bi, true); 
            }
        }
        // -----------------------------------------------------------------
        // MACH A: XU LY KHI NHAN DUOC GOI TIN PHAN HOI "ACK" TU BOARD B
        // -----------------------------------------------------------------
        else if (cai_ro_cua_toi.loai_thu == THU_ACK) {
            uint8_t ack_seq = cai_ro_cua_toi.so_thu_tu;
            
            // Chuyen tiep cho module Benchmark (neu dang chay do thong luong)
            Benchmark_XuLyACK(ack_seq);
            
            // DIEU KIEN HOP LE: So ACK phai nam trong pham vi cua so dang gui [base ... next_seq_num - 1]
            if (ack_seq >= base && ack_seq < next_seq_num) {
                char msg[150];
                sprintf(msg, "\r\n[MACH A] Nhan duoc ACK GOP (Seq %d). TRUOT CUA SO LEN!\r\n", ack_seq);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                
                // =================================================================
                // CO CHE ACK LUY KE (CUMULATIVE ACK) - TINH HOA CUA GO-BACK-N:
                // VI DU: Board A da gui 4 goi: 0, 1, 2, 3 (base = 0).
                // Tren duong ve, ACK(0) va ACK(1) bi roi mat do dut cap.
                // Nhung Board A may man nhan duoc ACK(2).
                // Lenh: base = ack_seq + 1 (tuc base = 2 + 1 = 3).
                // Y NGHIA: Nhan duoc ACK(2) nghia la Board B da nhan duoc goi 0, 1 va 2!
                // Cua so lap tuc truot qua ca 3 goi ma KHONG can gui lai goi 0 va 1!
                // =================================================================
                base = ack_seq + 1;
                
                if (base == next_seq_num) {
                    // Tat ca cac goi da gui deu da nhan duoc ACK -> Tat Timer nghi ngoi!
                    dang_chay_timer = false; 
                } else {
                    // Neu van con goi nam trong cua so chua co ACK -> Reset lai Timer cho goi ke tiep
                    thoi_gian_bat_dau_gui = Timer_LayThoiGian(); 
                }
            }
        }
        // -----------------------------------------------------------------
        // MACH A: XU LY KHI NHAN DUOC "NACK" (BOARD B BAO GOI TIN BI LOI CRC)
        // -----------------------------------------------------------------
        else if (cai_ro_cua_toi.loai_thu == THU_NACK) {
            // Chuyen tiep cho module Benchmark (neu dang chay)
            Benchmark_XuLyNACK(cai_ro_cua_toi.so_thu_tu);
            
            char msg[150];
            sprintf(msg, "\r\n[MACH A] Nhan duoc NACK (Loi CRC tai Seq %d). TRUYEN LAI NGAY!\r\n", cai_ro_cua_toi.so_thu_tu);
            UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            
            // =================================================================
            // CO CHE FAST RETRANSMIT (TRUYEN LAI TUC THI KHI CO BAO HONG):
            // Thay vi dung tro mat doi het 5 giay Timeout, khi thay NACK bao loi CRC,
            // Board A lap tuc keo lui next_seq_num = base va phat lai ngay lap tuc!
            // Ghi chu: Neu co giao bao tat che do nay de bat buoc cho Timeout 5s,
            // chi can comment dong: next_seq_num = base; lai la xong.
            // =================================================================
            next_seq_num = base;
            dang_chay_timer = false;
        }
    } 
    // NEU GOI TIN BI SAI MA CRC16
    else if (is_done == KETQUA_LOI_CRC) {
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            char msg[150];
            sprintf(msg, "\r\n[MACH B] PHAT HIEN SAI MA CRC16 (Goi Seq %d). Tra ve NACK...\r\n", cai_ro_cua_toi.so_thu_tu);
            UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            
            RoDungThu_t thu_nack;
            thu_nack.loai_thu = THU_NACK;
            thu_nack.so_thu_tu = cai_ro_cua_toi.so_thu_tu;
            thu_nack.do_dai = 0;
            
            uint8_t phong_bi_xuat[50];
            uint16_t kich_thuoc_phong_bi = 0;
            GiaoThuc_DongGoiThu(&thu_nack, phong_bi_xuat, &kich_thuoc_phong_bi); 
            SV3_GuiCoChenLoi(phong_bi_xuat, kich_thuoc_phong_bi, true);
        }
    }
}

// ====================================================================
// KHOI TAO UNG DUNG BOARD A
// ====================================================================
void App_BoardA_Init(void) {
    UART_KhoiTao();
    Timer_KhoiTao();
    
    OLED_Init();
    OLED_Clear();
    OLED_Print_Text(0, 0, 1, "NHOM 10 - BOARD A");
    OLED_Print_Text(2, 0, 1, "Nhiet do:     C");
    OLED_Print_Text(4, 0, 1, "Do am  :     %");
    OLED_Print_Text(6, 0, 1, "Gui:    TO:");
    
    DHT11_KhoiTao();
    
    char hello_msg[] = "\r\n=========================================\r\n"
                       "===== BOARD A - BEN PHAT DU LIEU =====\r\n"
                       "PB11: Bo goi | PB12: Dao bit\r\n"
                       "PB13: Chen rac | PB14: Delay ACK\r\n"
                       "PB15: CHAY BENCHMARK SAW vs GBN\r\n"
                       "=========================================\r\n";
    UART_GuiBanGoc((uint8_t*)hello_msg, strlen(hello_msg));
}

// ====================================================================
// XU LY NUT NHAN VAT LY (DEBOUNCE BANG THANH GHI)
// ====================================================================
static void App_XuLyNutNhan(void) {
    uint8_t c11 = 0, c12 = 0, c13 = 0, c14 = 0, c15 = 0;
    for (int i = 0; i < 5; i++) {
        if (~GPIOB->IDR & (1 << 11)) c11++;
        if (~GPIOB->IDR & (1 << 12)) c12++;
        if (~GPIOB->IDR & (1 << 13)) c13++;
        if (~GPIOB->IDR & (1 << 14)) c14++;
        if (~GPIOB->IDR & (1 << 15)) c15++;
        delay_ms(2);
    }
    // PB11-PB14: Kich hoat 4 loai loi SV3
    if (c11 >= 4) { SV3_KichHoatLoi('1'); delay_ms(500); }
    else if (c12 >= 4) { SV3_KichHoatLoi('2'); delay_ms(500); }
    else if (c13 >= 4) { SV3_KichHoatLoi('3'); delay_ms(500); }
    else if (c14 >= 4) { SV3_KichHoatLoi('4'); delay_ms(500); }
    // PB15: CHAY BENCHMARK SO SANH STOP-AND-WAIT vs GO-BACK-N
    else if (c15 >= 4) {
        delay_ms(500);
        UART_GuiBanGoc((uint8_t*)"\r\n*** BAT DAU BENCHMARK SO SANH SAW vs GBN ***\r\n", 50);
        
        KetQuaBenchmark_t ket_qua_saw, ket_qua_gbn;
        
        // Buoc 1: Chay Stop-and-Wait
        Benchmark_StopAndWait(BENCHMARK_SO_GOI, nhiet_do_gui, do_am_gui, &ket_qua_saw);
        
        delay_ms(3000); // Nghi 3 giay de Board B on dinh
        
        // Buoc 2: Chay Go-Back-N (cua so truot W=4)
        Benchmark_GoBackN(BENCHMARK_SO_GOI, nhiet_do_gui, do_am_gui, 4, &ket_qua_gbn);
        
        // Buoc 3: In bang so sanh ket qua
        Benchmark_InKetQua(&ket_qua_saw, &ket_qua_gbn, BENCHMARK_SO_GOI);
        
        // Hien thi ket qua len OLED
        char txt[30];
        OLED_Clear();
        OLED_Print_Text(0, 0, 1, "=== BENCHMARK ===");
        sprintf(txt, "SAW:%.1f g/s", ket_qua_saw.thong_luong);
        OLED_Print_Text(2, 0, 1, txt);
        sprintf(txt, "GBN:%.1f g/s", ket_qua_gbn.thong_luong);
        OLED_Print_Text(4, 0, 1, txt);
        float ti_le = (ket_qua_saw.thong_luong > 0) ? ket_qua_gbn.thong_luong / ket_qua_saw.thong_luong : 0;
        sprintf(txt, "GBN gap %.1fx", ti_le);
        OLED_Print_Text(6, 0, 1, txt);
        
        delay_ms(10000); // Hien thi 10 giay
        
        // Khoi phuc OLED ve giao dien binh thuong
        OLED_Clear();
        OLED_Print_Text(0, 0, 1, "NHOM 10 - BOARD A");
        OLED_Print_Text(2, 0, 1, "Nhiet do:     C");
        OLED_Print_Text(4, 0, 1, "Do am  :     %");
        OLED_Print_Text(6, 0, 1, "Gui:    TO:");
        
        // Reset lai cac bien Go-Back-N cho vong lap chinh
        base = 0; next_seq_num = 0; seq_dang_cho_nhan = 0;
        tong_goi_gui = 0; tong_lan_timeout = 0;
    }
}

// ====================================================================
// DOC CAM BIEN DHT11 MOI 2 GIAY
// ====================================================================
static void App_DocCamBien(void) {
    static uint32_t lan_doc_cuoi = 0;
    if (Timer_LayThoiGian() - lan_doc_cuoi >= 2000) {
        lan_doc_cuoi = Timer_LayThoiGian();
        DHT11_Data_t cam_bien;
        if (DHT11_DocDuLieu(&cam_bien)) {
            nhiet_do_gui = cam_bien.nhiet_do;
            do_am_gui    = cam_bien.do_am;
            char txt[20];
            sprintf(txt, "%d", nhiet_do_gui);
            OLED_Print_Text(2, 60, 1, "   ");
            OLED_Print_Text(2, 60, 1, txt);
            sprintf(txt, "%d", do_am_gui);
            OLED_Print_Text(4, 60, 1, "   ");
            OLED_Print_Text(4, 60, 1, txt);
        }
    }
}

// ====================================================================
// CAP NHAT THONG KE OLED MOI 500ms
// ====================================================================
static void App_CapNhatThongKe(void) {
    static uint32_t lan_tk_cuoi = 0;
    if (Timer_LayThoiGian() - lan_tk_cuoi >= 500) {
        lan_tk_cuoi = Timer_LayThoiGian();
        char txt[20];
        sprintf(txt, "%u", tong_goi_gui);
        OLED_Print_Text(6, 24, 1, "    ");
        OLED_Print_Text(6, 24, 1, txt);
        sprintf(txt, "%u", tong_lan_timeout);
        OLED_Print_Text(6, 78, 1, "   ");
        OLED_Print_Text(6, 78, 1, txt);
    }
}

// ====================================================================
// LOGIC GO-BACK-N (GUI GOI TIN CHUA DU LIEU CAM BIEN)
// ====================================================================
static void App_GoBackN_Logic(void) {
    if (base < GBN_TOTAL_PACKETS) {
        // =================================================================
        // 1. BAN LIEN THANH CAC GOI TRONG CUA SO W = 4:
        // Vong lap while nay giup Board A khong phai cho tung goi mot!
        // VI DU: base = 0. Cua so cho phep gui den: 0 + 4 = 4.
        // next_seq_num se chay tu 0, 1, 2, 3 va ban lien tiep 4 goi vao mang.
        // Ghi chu sua code: Neu co giao bat doi W=2, chi can doi macro GBN_WINDOW_SIZE thanh 2.
        // =================================================================
        while (next_seq_num < base + GBN_WINDOW_SIZE && next_seq_num < GBN_TOTAL_PACKETS) {
            char msg[100];
            sprintf(msg, "\r\n[BOARD A] GBN: Gui goi (Seq %d) - T:%dC, H:%d%%\r\n", 
                    next_seq_num, nhiet_do_gui, do_am_gui);
            UART_GuiBanGoc((uint8_t*)msg, strlen(msg));

            // DONG GOI DU LIEU CAM BIEN:
            RoDungThu_t thu_data;
            thu_data.loai_thu = THU_DATA;
            thu_data.so_thu_tu = next_seq_num; // Danh so thu tu Seq (0 den 7)
            
            // GIAI THICH: do_dai = 2 vi phan ruot goi tin chua dung 2 byte du lieu:
            thu_data.do_dai = 2;
            thu_data.ruot_thu[0] = nhiet_do_gui; // Byte 1: Nhiet do (vi du 30 do C)
            thu_data.ruot_thu[1] = do_am_gui;    // Byte 2: Do am    (vi du 65%)

            // Goi ham SV1 dong goi (them CRC16, chen cờ 0x7E, nhồi byte thoat)
            uint8_t phong_bi_xuat[50];
            uint16_t kich_thuoc_phong_bi = 0;
            GiaoThuc_DongGoiThu(&thu_data, phong_bi_xuat, &kich_thuoc_phong_bi);
            
            // Goi ham SV3 gui qua UART (co the bi chen loi neu bam nut PB11-PB14)
            SV3_GuiCoChenLoi(phong_bi_xuat, kich_thuoc_phong_bi, false);

            tong_goi_gui++; // Tang bien thong ke tong so goi da phat

            // =============================================================
            // BAM DONG HO TIMEOUT (CHI BAM TAI GOI DAU TIEN CUA CUA SO):
            // Neu base == next_seq_num (tuc goi so 0 trong cua so), ta bat dau bam gio.
            // Cac goi tiep theo (1, 2, 3) di theo sau khong can bam lai Timer.
            // =============================================================
            if (base == next_seq_num) {
                thoi_gian_bat_dau_gui = Timer_LayThoiGian();
                dang_chay_timer = true;
            }
            next_seq_num++; // Tang con tro sang goi ke tiep
            delay_ms(300);  // Nghi 300ms giua cac goi de tranh nghen bo dem UART
        }

        // =================================================================
        // 2. KIEM TRA HET GIO (TIMEOUT) VA TRUYEN LAI GO-BACK-N:
        // =================================================================
        if (dang_chay_timer == true) {
            // DONG 327 (HAY BI CO BAT SUA): 5000ms = 5 giay.
            // Neu co bat doi Timeout thanh 2 giay, chi can doi so 5000 thanh 2000.
            if (Timer_LayThoiGian() - thoi_gian_bat_dau_gui >= 5000) {
                char msg[150];
                sprintf(msg, "\r\n[BOARD A] TIMEOUT! Go-Back-N truyen lai tu Seq %d...\r\n", base);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                
                // =========================================================
                // TINH HOA "GO-BACK-N": LUI CON TRO next_seq_num VE base!
                // VI DU: Da gui 4 goi [0, 1, 2, 3]. Goi 0 bi rot mang nen khong co ACK.
                // Het 5 giay, lenh: next_seq_num = base (tuc next_seq_num lui ve 0).
                // Vong lap while ben tren se lap tuc ban lai toan bo ca 4 goi 0, 1, 2, 3!
                // =========================================================
                next_seq_num = base;
                dang_chay_timer = false;
                tong_lan_timeout++;
                delay_ms(2000); // Nghi 2 giay de log tren may tinh kip hien ro
            }
        }
    } else {
        char msg[] = "\r\n[HOAN THANH] Da gui xong toan bo 8 goi Go-Back-N!\r\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        delay_ms(10000);
        base = 0; next_seq_num = 0; seq_dang_cho_nhan = 0;
        char msg2[] = "\r\n[RESET] Bat dau phien Go-Back-N moi...\r\n";
        UART_GuiBanGoc((uint8_t*)msg2, strlen(msg2));
        delay_ms(2000);
    }
}

// ====================================================================
// VONG LAP CHINH CUA BOARD A (GOI TRONG while(1) CUA main.c)
// ====================================================================
void App_BoardA_Loop(void) {
    App_XuLyNutNhan();
    App_DocCamBien();
    App_CapNhatThongKe();
    App_GoBackN_Logic();
}

