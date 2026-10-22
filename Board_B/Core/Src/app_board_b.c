/* ============================================================================
 * TÊN FILE       : app_board_b.c
 * PHỤ TRÁCH      : SINH VIÊN 2 - Đồng Quang Đại (Tầng Tin Cậy - Phía Thu (Receiver))
 * CHỨC NĂNG CHÍNH: Khối điều khiển Mạch Thu (Board B): Kiểm tra số thứ tự Sequence Number, bộ lọc loại bỏ gói trùng lặp (Duplicate Detection), và luôn phản hồi Cumulative ACK giải thoát bên phát.
 * CÁC API CHÍNH  : App_BoardB_Init(), App_BoardB_Loop(), UART_HamNgatNhan()
 * ============================================================================ */
#include "app_board_b.h"
#include "main.h"
#include "uart_driver.h"
#include "reliable_protocol.h"
#include "timer_driver.h"
#include "Anglas_OLED_SSD1306.h"
#include <stdio.h>
#include <string.h>

// ====================================================================
// BOARD B - BEN NHAN: Nhan du lieu tu Board A, kiem tra CRC16,
// tra ACK/NACK, hien thi du lieu cam bien + thong ke len OLED
// ====================================================================

// --- BIEN THONG KE HIEN THI TREN OLED ---
static uint32_t tong_goi_nhan  = 0;
static uint32_t tong_loi_crc   = 0;
static uint32_t tong_goi_trung = 0;

// --- BIEN DIEU KHIEN NHAN GO-BACK-N ---
static uint8_t seq_dang_cho_nhan = 0;

// --- DU LIEU CAM BIEN NHAN TU BOARD A ---
static uint8_t nhiet_do_nhan = 0;
static uint8_t do_am_nhan    = 0;
static bool    co_du_lieu_moi = false;

static RoDungThu_t cai_ro_cua_toi;

// ====================================================================
// HAM XU LY NGAT NHAN UART (GOI TU uart_driver.c)
// ====================================================================
void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    TrangThaiBocTach_t is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    
    if (is_done == KETQUA_OK) {
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            char msg[150];
            
            // =================================================================
            // TRUONG HOP 1: NHAN DUNG THU TU GOI TIN DANG CHO (Seq == seq_dang_cho_nhan)
            // VI DU: Board B dang doi goi Seq=0. Goi bay den dung la Seq=0.
            // -> Hop le 100%! Lay du lieu cam bien ra de hien thi len OLED.
            // =================================================================
            if (cai_ro_cua_toi.so_thu_tu == seq_dang_cho_nhan) {
                // NHAN DUNG THU TU - Lay du lieu ra xai
                sprintf(msg, "\r\n[BOARD B] Nhan DUNG (Seq %d). Nhiet do: %d, Do am: %d\r\n", 
                        cai_ro_cua_toi.so_thu_tu,
                        (cai_ro_cua_toi.do_dai >= 2) ? cai_ro_cua_toi.ruot_thu[0] : 0,
                        (cai_ro_cua_toi.do_dai >= 2) ? cai_ro_cua_toi.ruot_thu[1] : 0);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                
                // Lay du lieu cam bien tu goi tin
                if (cai_ro_cua_toi.do_dai >= 2) {
                    nhiet_do_nhan = cai_ro_cua_toi.ruot_thu[0]; // Byte 0: Nhiet do
                    do_am_nhan    = cai_ro_cua_toi.ruot_thu[1]; // Byte 1: Do am
                    co_du_lieu_moi = true;                     // Bat co bao cho OLED cap nhat
                }
                
                tong_goi_nhan++;     // Tang bien dem goi OK tren OLED
                seq_dang_cho_nhan++; // Tang ky vong len goi tiep theo (vi du 0 -> 1)
                
                // =============================================================
                // CO CHE QUAY VONG SO THU TU (MODULO 8: 0 DEN 7):
                // Khi da nhan du 8 goi (tu 0 den 7), reset seq ve 0 de dong bo voi A!
                // =============================================================
                if (seq_dang_cho_nhan >= 8) {
                    seq_dang_cho_nhan = 0;
                    UART_GuiBanGoc((uint8_t*)"\r\n[BOARD B] Da nhan du 8 goi. Reset Seq ve 0 de doi phien moi!\r\n", 64);
                }
            } 
            // =================================================================
            // TRUONG HOP 2: GOI TRUNG LAP (DUPLICATE) HOAC LAC THU TU
            // VI DU: Board B da nhan goi 0 roi va dang cho goi 1 (seq_dang_cho = 1).
            // Nhung tren duong ve, ACK(0) bi rot mang. Board A bi Timeout nen gui lai goi 0 lan 2!
            // Khi goi 0 toi: cai_ro_cua_toi.so_thu_tu (0) != seq_dang_cho_nhan (1).
            // -> BO LOC TRUNG KICH HOAT: Vut bo du lieu, KHONG cho hien thi len OLED!
            // =================================================================
            else {
                sprintf(msg, "\r\n[BOARD B] TRUNG LAP! Doi Seq %d, nhan Seq %d. Vut data!\r\n", 
                        seq_dang_cho_nhan, cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                tong_goi_trung++; // Tang bien dem "D" (Duplicate) tren OLED
                // CHU Y: O day KHONG gan nhiet_do_nhan va KHONG bat co_du_lieu_moi!
            }
            
            // =================================================================
            // QUY TAC SONG CON CUA GO-BACK-N: LUON TRA ACK CHO GOI DUNG GAN NHAT!
            // TAI SAO PHAI GUI ACK KE CA KHI NHAN DUOC GOI TRUNG LAP?
            // VI DU: O tren Board A gui lai goi 0 vi no chua nhan duoc ACK(0).
            // Neu Board B thay goi trung ma IM LANG khong gui ACK, thi Board A se bi ket
            // trong vong lap Timeout vo tan (Deadlock)!
            // Vi vay, du vut bo du lieu trung, B BAT BUOC phai gui lai ACK de cuu thoat A!
            // =================================================================
            RoDungThu_t thu_ack;
            thu_ack.loai_thu = THU_ACK;
            
            // Tinh so thu tu ACK luy ke:
            if (seq_dang_cho_nhan == 0) {
                thu_ack.so_thu_tu = 7; // Neu dang cho 0, nghia la goi dung gan nhat la 7
            } else {
                thu_ack.so_thu_tu = seq_dang_cho_nhan - 1; // Goi dung gan nhat = dang cho - 1
            }
            thu_ack.do_dai = 0; // ACK chi la giay bien nhan, khong chua ruot data
            
            // Dong goi va ban ra UART ve lai Board A
            uint8_t phong_bi_xuat[50];
            uint16_t kich_thuoc_phong_bi = 0;
            GiaoThuc_DongGoiThu(&thu_ack, phong_bi_xuat, &kich_thuoc_phong_bi); 
            UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi);
        }
    } 
    // =================================================================
    // TRUONG HOP 3: GOI TIN BI SAI MA CRC16 (DO NHIEU HOAC SV3 BAM NUT DAO BIT)
    // =================================================================
    else if (is_done == KETQUA_LOI_CRC) {
        char msg[100];
        sprintf(msg, "\r\n[BOARD B] LOI CRC16 (Seq %d)! Tra NACK...\r\n", cai_ro_cua_toi.so_thu_tu);
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        tong_loi_crc++; // Tang bien dem "Err" tren OLED
        
        // Gui ngay goi tin NACK ve de bao cho Board A truyen lai lap tuc (Fast Retransmit)
        RoDungThu_t thu_nack;
        thu_nack.loai_thu = THU_NACK;
        thu_nack.so_thu_tu = cai_ro_cua_toi.so_thu_tu;
        thu_nack.do_dai = 0;
        
        uint8_t phong_bi_xuat[50];
        uint16_t kich_thuoc_phong_bi = 0;
        GiaoThuc_DongGoiThu(&thu_nack, phong_bi_xuat, &kich_thuoc_phong_bi); 
        UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi);
    }
}

// ====================================================================
// KHOI TAO UNG DUNG BOARD B
// ====================================================================
void App_BoardB_Init(void) {
    UART_KhoiTao();
    Timer_KhoiTao();
    
    OLED_Init();
    OLED_Clear();
    OLED_Print_Text(0, 0, 1, "NHOM 10 - BOARD B");
    OLED_Print_Text(2, 0, 1, "Nhiet do:     C");
    OLED_Print_Text(4, 0, 1, "Do am  :     %");
    OLED_Print_Text(6, 0, 1, "OK:    Err:   D:");
    
    char hello_msg[] = "\r\n=========================================\r\n"
                       "===== BOARD B - BEN NHAN DU LIEU =====\r\n"
                       "=========================================\r\n";
    UART_GuiBanGoc((uint8_t*)hello_msg, strlen(hello_msg));
}

// ====================================================================
// CAP NHAT OLED MOI 500ms
// ====================================================================
static void App_CapNhatOLED(void) {
    static uint32_t lan_cap_nhat_cuoi = 0;
    if (Timer_LayThoiGian() - lan_cap_nhat_cuoi >= 500) {
        lan_cap_nhat_cuoi = Timer_LayThoiGian();
        char txt[20];
        
        // Hien thi nhiet do va do am nhan tu Board A
        if (co_du_lieu_moi) {
            sprintf(txt, "%d", nhiet_do_nhan);
            OLED_Print_Text(2, 60, 1, "   ");
            OLED_Print_Text(2, 60, 1, txt);
            sprintf(txt, "%d", do_am_nhan);
            OLED_Print_Text(4, 60, 1, "   ");
            OLED_Print_Text(4, 60, 1, txt);
            co_du_lieu_moi = false;
        }
        
        // Hien thi thong ke: OK (nhan dung), Err (loi CRC), D (trung lap)
        sprintf(txt, "%u", tong_goi_nhan);
        OLED_Print_Text(6, 18, 1, "    ");
        OLED_Print_Text(6, 18, 1, txt);
        
        sprintf(txt, "%u", tong_loi_crc);
        OLED_Print_Text(6, 72, 1, "   ");
        OLED_Print_Text(6, 72, 1, txt);
        
        sprintf(txt, "%u", tong_goi_trung);
        OLED_Print_Text(6, 102, 1, "   ");
        OLED_Print_Text(6, 102, 1, txt);
    }
}

// ====================================================================
// VONG LAP CHINH CUA BOARD B (GOI TRONG while(1) CUA main.c)
// ====================================================================
void App_BoardB_Loop(void) {
    App_CapNhatOLED();
}

