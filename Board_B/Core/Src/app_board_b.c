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
            if (cai_ro_cua_toi.so_thu_tu == seq_dang_cho_nhan) {
                // NHAN DUNG THU TU - Lay du lieu ra xai
                sprintf(msg, "\r\n[BOARD B] Nhan DUNG (Seq %d). Nhiet do: %d, Do am: %d\r\n", 
                        cai_ro_cua_toi.so_thu_tu,
                        (cai_ro_cua_toi.do_dai >= 2) ? cai_ro_cua_toi.ruot_thu[0] : 0,
                        (cai_ro_cua_toi.do_dai >= 2) ? cai_ro_cua_toi.ruot_thu[1] : 0);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                
                // Lay du lieu cam bien tu goi tin
                if (cai_ro_cua_toi.do_dai >= 2) {
                    nhiet_do_nhan = cai_ro_cua_toi.ruot_thu[0];
                    do_am_nhan    = cai_ro_cua_toi.ruot_thu[1];
                    co_du_lieu_moi = true;
                }
                
                tong_goi_nhan++;
                seq_dang_cho_nhan++;
                
                // KHI NHAN DU 8 GOI (0 DEN 7), RESET VE 0 DE DONG BO VOI BOARD A
                if (seq_dang_cho_nhan >= 8) {
                    seq_dang_cho_nhan = 0;
                    UART_GuiBanGoc((uint8_t*)"\r\n[BOARD B] Da nhan du 8 goi. Reset Seq ve 0 de doi phien moi!\r\n", 64);
                }
            } else {
                // GOI TRUNG LAP hoac SAI THU TU
                sprintf(msg, "\r\n[BOARD B] TRUNG LAP! Doi Seq %d, nhan Seq %d. Vut data!\r\n", 
                        seq_dang_cho_nhan, cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                tong_goi_trung++;
            }
            
            // Go-Back-N: Luon tra ACK luy ke (Cumulative ACK) cho goi dung gan nhat
            RoDungThu_t thu_ack;
            thu_ack.loai_thu = THU_ACK;
            
            if (seq_dang_cho_nhan == 0) {
                thu_ack.so_thu_tu = 7;
            } else {
                thu_ack.so_thu_tu = seq_dang_cho_nhan - 1;
            }
            thu_ack.do_dai = 0;
            
            uint8_t phong_bi_xuat[50];
            uint16_t kich_thuoc_phong_bi = 0;
            GiaoThuc_DongGoiThu(&thu_ack, phong_bi_xuat, &kich_thuoc_phong_bi); 
            UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi);
        }
    } 
    // GOI TIN BI SAI CRC16 -> Tra ve NACK
    else if (is_done == KETQUA_LOI_CRC) {
        char msg[100];
        sprintf(msg, "\r\n[BOARD B] LOI CRC16 (Seq %d)! Tra NACK...\r\n", cai_ro_cua_toi.so_thu_tu);
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        tong_loi_crc++;
        
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
