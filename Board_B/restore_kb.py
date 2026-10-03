import os

filepath = r'Core\Src\main.c'

with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

target = '''// ====================================================================
// KICH BAN 3: TEST SV2 (GO-BACK-N CUA SO TRUOT)'''

kich_ban_1_va_2 = '''// ====================================================================
// KICH BAN 1: TEST SV1 (FRAMING & BYTE STUFFING)
// -> DANG KHOA (De mo: Xoa cap dau /* va */ bao quanh)
// ====================================================================
/*
RoDungThu_t cai_ro_cua_toi;

void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    bool is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    if (is_done == true) {
        char msg[150];
        sprintf(msg, "\\r\\n[MACH B] Bóc tách SV1 THÀNH CÔNG! Loai: %d, Seq: %d, Dai: %d\\r\\n", 
                cai_ro_cua_toi.loai_thu, cai_ro_cua_toi.so_thu_tu, cai_ro_cua_toi.do_dai);
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    }
}
// Trong vong lap while(1): Khong can ghi gi them vi chi can test mach nhan
*/


// ====================================================================
// KICH BAN 2: TEST SV2 (STOP-AND-WAIT ARQ)
// -> DANG KHOA (De mo: Xoa cap dau /* va */ bao quanh)
// ====================================================================
/*
#define MAX_TRUYEN_LAI 3

uint8_t seq_gui_hien_tai = 0;
uint8_t so_lan_truyen_lai = 0;
uint32_t thoi_gian_bat_dau_gui = 0;
bool dang_cho_ack = false;
uint8_t seq_dang_cho_nhan = 0;
RoDungThu_t cai_ro_cua_toi;

void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    // 1. Cho phep nhan nut kich hoat Loi SV3 
    if (chu_cai_nhan >= '0' && chu_cai_nhan <= '4') { SV3_KichHoatLoi(chu_cai_nhan); return; }

    bool is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    if (is_done == true) {
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            if (cai_ro_cua_toi.so_thu_tu == seq_dang_cho_nhan) {
                char msg[] = "\\r\\n[MACH B] Nhan DUNG THU TU. Lay data & Tra loi ACK...\\r\\n";
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                seq_dang_cho_nhan = 1 - seq_dang_cho_nhan; // Dao bit 0 <-> 1
            } else {
                char msg[] = "\\r\\n[MACH B] Trung lap goi tin. Vut bo data & Tra lai ACK...\\r\\n";
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            }
            
            // Gui Giay bien nhan (ACK)
            RoDungThu_t thu_ack;
            thu_ack.loai_thu = THU_ACK;
            thu_ack.so_thu_tu = cai_ro_cua_toi.so_thu_tu;
            thu_ack.do_dai = 0;
            uint8_t p_xuat[50]; uint16_t k_thuoc = 0;
            GiaoThuc_DongGoiThu(&thu_ack, p_xuat, &k_thuoc);
            SV3_GuiCoChenLoi(p_xuat, k_thuoc, true); // Di qua tram thu phi SV3
        }
        else if (cai_ro_cua_toi.loai_thu == THU_ACK) {
            if (cai_ro_cua_toi.so_thu_tu == seq_gui_hien_tai && dang_cho_ack == true) {
                char msg[] = "\\r\\n[MACH A] Nhan duoc ACK. Giao hang thanh cong!\\r\\n";
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                dang_cho_ack = false;
                so_lan_truyen_lai = 0;
                seq_gui_hien_tai = 1 - seq_gui_hien_tai; // Dao bit 0 <-> 1 de gui goi tiep theo
            }
        }
    }
}

// ============= TRONG VONG LAP while(1) =============
// if (dang_cho_ack == false) {
//     char msg[100];
//     sprintf(msg, "\\r\\n[MACH A] Dang gui goi tin (Seq %d)...\\r\\n", seq_gui_hien_tai);
//     UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
//     RoDungThu_t thu_data;
//     thu_data.loai_thu = THU_DATA; thu_data.so_thu_tu = seq_gui_hien_tai; thu_data.do_dai = 1; thu_data.ruot_thu[0] = 0x88;
//     uint8_t phong_bi[50]; uint16_t kich_thuoc = 0;
//     GiaoThuc_DongGoiThu(&thu_data, phong_bi, &kich_thuoc);
//     SV3_GuiCoChenLoi(phong_bi, kich_thuoc, false); // Di qua tram thu phi SV3
//     dang_cho_ack = true;
//     thoi_gian_bat_dau_gui = Timer_LayThoiGian();
// } else {
//     if (Timer_LayThoiGian() - thoi_gian_bat_dau_gui >= 3000) { // Timeout 3s
//         if (so_lan_truyen_lai < MAX_TRUYEN_LAI) {
//             so_lan_truyen_lai++;
//             char msg[] = "\\r\\n[MACH A] TIMEOUT! Dang truyen lai goi tin...\\r\\n";
//             UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
//             // Goi ham gui lai ...
//             thoi_gian_bat_dau_gui = Timer_LayThoiGian();
//         } else {
//             char msg[] = "\\r\\n[MACH A] THAT BAI! Ket noi bi mat hoan toan.\\r\\n";
//             UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
//             HAL_Delay(5000); // Nghi 5s roi thu lai tu dau
//             so_lan_truyen_lai = 0;
//             dang_cho_ack = false;
//         }
//     }
// }
*/

// ====================================================================
// KICH BAN 3: TEST SV2 (GO-BACK-N CUA SO TRUOT)'''

new_content = content.replace(target, kich_ban_1_va_2)

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(new_content)