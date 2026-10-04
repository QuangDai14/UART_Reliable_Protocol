#include "benchmark.h"
#include "uart_driver.h"
#include "reliable_protocol.h"
#include "timer_driver.h"
#include "error_injection.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

// ====================================================================
// MODULE DO LUONG HIEU NANG: SO SANH STOP-AND-WAIT vs GO-BACK-N
// ====================================================================

// --- Bien toan cuc tam thoi dung trong qua trinh Benchmark ---
static volatile uint8_t bm_ack_nhan_duoc = 0;   // Co bao da nhan ACK
static volatile uint8_t bm_ack_seq = 0;          // Seq cua ACK nhan duoc
static volatile uint8_t bm_nack_nhan_duoc = 0;   // Co bao nhan NACK
static volatile uint8_t bm_dang_benchmark = 0;   // Co bao dang chay benchmark
static volatile uint8_t bm_gbn_base = 0;         // Base cua GBN 

// Ham callback duoc goi tu UART_HamNgatNhan khi dang o che do benchmark
// (Se duoc tich hop vao main.c)
void Benchmark_XuLyACK(uint8_t seq) {
    if (bm_dang_benchmark) {
        bm_ack_seq = seq;
        bm_ack_nhan_duoc = 1;
    }
}

void Benchmark_XuLyNACK(uint8_t seq) {
    if (bm_dang_benchmark) {
        bm_nack_nhan_duoc = 1;
    }
}

// --- Bien co the truy cap tu ben ngoai ---
volatile uint8_t bm_mode_active = 0; // 0 = off, 1 = dang benchmark

// ====================================================================
// BENCHMARK 1: STOP-AND-WAIT (GUI 1 GOI -> CHO ACK -> GUI GOI TIEP)
// ====================================================================
void Benchmark_StopAndWait(uint16_t so_goi, uint8_t nhiet_do, uint8_t do_am, KetQuaBenchmark_t* ket_qua) {
    char msg[200];
    
    sprintf(msg, "\r\n========== BENCHMARK STOP-AND-WAIT (%d goi) ==========\r\n", so_goi);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    
    bm_dang_benchmark = 1;
    bm_mode_active = 1;
    
    uint32_t tong_gui = 0;
    uint32_t tong_ok = 0;
    uint32_t tong_timeout = 0;
    uint32_t lien_tiep_timeout = 0;
    uint8_t seq = 0;
    
    uint32_t thoi_gian_bat_dau = Timer_LayThoiGian();
    
    while (tong_ok < so_goi) {
        // Dong goi thu DATA
        RoDungThu_t thu_data;
        thu_data.loai_thu = THU_DATA;
        thu_data.so_thu_tu = seq;
        thu_data.do_dai = 2;
        thu_data.ruot_thu[0] = nhiet_do;
        thu_data.ruot_thu[1] = do_am;
        
        uint8_t phong_bi[50];
        uint16_t kich_thuoc = 0;
        GiaoThuc_DongGoiThu(&thu_data, phong_bi, &kich_thuoc);
        
        // Gui goi tin (Co cho phep SV3 chen loi)
        bm_ack_nhan_duoc = 0;
        bm_nack_nhan_duoc = 0;
        SV3_GuiCoChenLoi(phong_bi, kich_thuoc, false);
        tong_gui++;
        
        // Bat dau dem thoi gian cho ACK
        uint32_t t_gui = Timer_LayThoiGian();
        uint8_t da_nhan_ack = 0;
        
        // Cho ACK toi da 2 giay
        while (Timer_LayThoiGian() - t_gui < 2000) {
            if (bm_ack_nhan_duoc) {
                bm_ack_nhan_duoc = 0;
                if (bm_ack_seq == seq) {
                    da_nhan_ack = 1;
                    break;
                }
            }
            if (bm_nack_nhan_duoc) {
                bm_nack_nhan_duoc = 0;
                // Nhan NACK -> truyen lai ngay
                break;
            }
        }
        
        if (da_nhan_ack) {
            tong_ok++;
            seq++;
            if (seq >= 8) seq = 0; // Wrap-around
            lien_tiep_timeout = 0;
        } else {
            tong_timeout++;
            lien_tiep_timeout++;
            // Khong tang seq -> gui lai goi cu
        }
        
        if (lien_tiep_timeout >= 10) {
            UART_GuiBanGoc((uint8_t*)"\r\n[LOI] Mat ket noi voi Board B! Huy Benchmark SAW.\r\n", 53);
            break; 
        }
        
        // In tien trinh moi 10%
        if (tong_ok > 0 && tong_ok % (so_goi / 10) == 0) {
            sprintf(msg, "  [SAW] Tien do: %u/%d goi OK, Timeout: %u\r\n", tong_ok, so_goi, tong_timeout);
            UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        }
    }
    
    uint32_t thoi_gian_ket_thuc = Timer_LayThoiGian();
    
    // Ghi ket qua
    ket_qua->tong_goi_gui = tong_gui;
    ket_qua->tong_goi_thanh_cong = tong_ok;
    ket_qua->tong_lan_timeout = tong_timeout;
    ket_qua->thoi_gian_ms = thoi_gian_ket_thuc - thoi_gian_bat_dau;
    if (ket_qua->thoi_gian_ms > 0) {
        ket_qua->thong_luong = (float)tong_ok * 1000.0f / (float)ket_qua->thoi_gian_ms;
    } else {
        ket_qua->thong_luong = 0;
    }
    
    bm_dang_benchmark = 0;
    bm_mode_active = 0;
    
    sprintf(msg, "  [SAW] HOAN THANH! Thoi gian: %u ms, Thong luong: %.1f goi/s\r\n", 
            ket_qua->thoi_gian_ms, ket_qua->thong_luong);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
}

// ====================================================================
// BENCHMARK 2: GO-BACK-N (GUI LIEN THANH THEO CUA SO TRUOT)
// ====================================================================
void Benchmark_GoBackN(uint16_t so_goi, uint8_t nhiet_do, uint8_t do_am, uint8_t window_size, KetQuaBenchmark_t* ket_qua) {
    char msg[200];
    
    sprintf(msg, "\r\n========== BENCHMARK GO-BACK-N (W=%d, %d goi) ==========\r\n", window_size, so_goi);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    
    bm_dang_benchmark = 1;
    bm_mode_active = 1;
    
    uint32_t tong_gui = 0;
    uint32_t tong_timeout = 0;
    uint32_t lien_tiep_timeout = 0;
    uint8_t base = 0;
    uint8_t next_seq = 0;
    uint32_t timer_start = 0;
    uint8_t timer_active = 0;
    
    uint32_t thoi_gian_bat_dau = Timer_LayThoiGian();
    
    while (base < so_goi) {
        // Gui lien thanh cac goi trong cua so
        while (next_seq < base + window_size && next_seq < so_goi) {
            RoDungThu_t thu_data;
            thu_data.loai_thu = THU_DATA;
            thu_data.so_thu_tu = next_seq % 8; // Wrap-around so thu tu trong khung 0-7
            thu_data.do_dai = 2;
            thu_data.ruot_thu[0] = nhiet_do;
            thu_data.ruot_thu[1] = do_am;
            
            uint8_t phong_bi[50];
            uint16_t kich_thuoc = 0;
            GiaoThuc_DongGoiThu(&thu_data, phong_bi, &kich_thuoc);
            SV3_GuiCoChenLoi(phong_bi, kich_thuoc, false);
            tong_gui++;
            
            if (base == next_seq) {
                timer_start = Timer_LayThoiGian();
                timer_active = 1;
            }
            next_seq++;
            HAL_Delay(10); // Delay nho de Board B kip xu ly
        }
        
        // Cho ACK hoac Timeout
        while (timer_active) {
            // Kiem tra ACK
            if (bm_ack_nhan_duoc) {
                // Fix Race Condition: Xoa co TRUOC KHI xu ly
                bm_ack_nhan_duoc = 0;
                uint8_t ack_seq_abs = bm_ack_seq;
                
                // Tim gia tri tuyet doi cua ACK (ACK luy ke, ack_seq la so thu tu trong khung 0-7)
                uint16_t candidate = base;
                while (candidate < next_seq) {
                    if ((candidate % 8) == ack_seq_abs) {
                        break;
                    }
                    candidate++;
                }
                
                if (candidate < next_seq) {
                    base = candidate + 1;
                    lien_tiep_timeout = 0; // Reset bien dem timeout lien tiep
                    
                    if (base >= next_seq) {
                        timer_active = 0;
                    } else {
                        timer_start = Timer_LayThoiGian();
                    }
                }
                
                if (base >= so_goi) break;
                
                // Thoat de gui goi moi neu cua so con cho
                if (next_seq < base + window_size && next_seq < so_goi) break;
            }
            
            // Kiem tra NACK
            if (bm_nack_nhan_duoc) {
                bm_nack_nhan_duoc = 0;
                next_seq = base;
                timer_active = 0;
                break;
            }
            
            // Kiem tra Timeout (3 giay)
            if (Timer_LayThoiGian() - timer_start >= 3000) {
                tong_timeout++;
                lien_tiep_timeout++;
                next_seq = base;
                timer_active = 0;
                break;
            }
        }
        
        // Neu timeout lien tiep 10 lan (tuc la mat ket noi) -> huy benchmark de khoi treo OLED
        if (lien_tiep_timeout >= 10) {
            UART_GuiBanGoc((uint8_t*)"\r\n[LOI] Mat ket noi voi Board B! Huy Benchmark GBN.\r\n", 53);
            break; 
        }
        
        // In tien trinh moi 10%
        if (base > 0 && base % (so_goi / 10) == 0) {
            sprintf(msg, "  [GBN] Tien do: %u/%d goi OK, Timeout: %u\r\n", base, so_goi, tong_timeout);
            UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        }
    }
    
    uint32_t thoi_gian_ket_thuc = Timer_LayThoiGian();
    
    // Ghi ket qua
    ket_qua->tong_goi_gui = tong_gui;
    ket_qua->tong_goi_thanh_cong = base; // base == so_goi khi hoan thanh
    ket_qua->tong_lan_timeout = tong_timeout;
    ket_qua->thoi_gian_ms = thoi_gian_ket_thuc - thoi_gian_bat_dau;
    if (ket_qua->thoi_gian_ms > 0) {
        ket_qua->thong_luong = (float)ket_qua->tong_goi_thanh_cong * 1000.0f / (float)ket_qua->thoi_gian_ms;
    } else {
        ket_qua->thong_luong = 0;
    }
    
    bm_dang_benchmark = 0;
    bm_mode_active = 0;
    
    sprintf(msg, "  [GBN] HOAN THANH! Thoi gian: %u ms, Thong luong: %.1f goi/s\r\n", 
            ket_qua->thoi_gian_ms, ket_qua->thong_luong);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
}

// ====================================================================
// IN BANG SO SANH KET QUA RA HERCULES
// ====================================================================
void Benchmark_InKetQua(KetQuaBenchmark_t* saw, KetQuaBenchmark_t* gbn, uint16_t so_goi) {
    char msg[300];
    
    sprintf(msg, "\r\n");
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "+----------------------------------------------------+\r\n");
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "|    BANG SO SANH HIEU NANG: SAW vs GBN (%d goi)   |\r\n", so_goi);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "+------------------+---------------+-----------------+\r\n");
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "|   Chi so         | Stop-and-Wait |   Go-Back-N     |\r\n");
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "+------------------+---------------+-----------------+\r\n");
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "| Tong goi gui     | %13u | %15u |\r\n", saw->tong_goi_gui, gbn->tong_goi_gui);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "| Goi thanh cong   | %13u | %15u |\r\n", saw->tong_goi_thanh_cong, gbn->tong_goi_thanh_cong);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "| Lan timeout      | %13u | %15u |\r\n", saw->tong_lan_timeout, gbn->tong_lan_timeout);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "| Thoi gian (ms)   | %13u | %15u |\r\n", saw->thoi_gian_ms, gbn->thoi_gian_ms);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "| Thong luong(g/s) | %13.1f | %15.1f |\r\n", saw->thong_luong, gbn->thong_luong);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "+------------------+---------------+-----------------+\r\n");
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    
    // Tinh ti le cai thien
    float ti_le = 0;
    if (saw->thong_luong > 0) {
        ti_le = (gbn->thong_luong / saw->thong_luong - 1.0f) * 100.0f;
    }
    sprintf(msg, "| GBN nhanh hon    |               |    +%4.0f%%%%      |\r\n", ti_le);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "+------------------+---------------+-----------------+\r\n");
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    
    // Ket luan
    sprintf(msg, "\r\n>> KET LUAN: Go-Back-N (W=4) dat thong luong %.1f goi/s,\r\n", gbn->thong_luong);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "   gap %.1f lan so voi Stop-and-Wait (%.1f goi/s).\r\n", 
            (saw->thong_luong > 0) ? gbn->thong_luong / saw->thong_luong : 0,
            saw->thong_luong);
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    sprintf(msg, "   Cua so truot giup su dung hieu qua bang thong khi khong co loi.\r\n\r\n");
    UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
}