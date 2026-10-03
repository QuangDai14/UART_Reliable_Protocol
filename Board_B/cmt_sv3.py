import os

c_content = '''#include "error_injection.h"
#include "uart_driver.h"
#include "main.h" // de dung HAL_Delay
#include <string.h>

// Tu dien cac loai benh ly (Loi) can gia lap tren mang
typedef enum {
    KHONG_LOI = 0,    // Trang thai mang khoe manh binh thuong
    LOI_BO_GOI = 1,   // Gia lap dut cap, mat goi tin tren duong truyen
    LOI_DAO_BIT = 2,  // Gia lap nhieu dien tu lam sai lech Checksum
    LOI_CHEN_RAC = 3, // Gia lap bi thiet bi la chen song, chen du lieu rac vao giua khung
    LOI_TRE_ACK = 4   // Gia lap nghe mang, giay bien nhan (ACK) bi ket xe ve tre
} CheDoChenLoi_t;

// Bien toan cuc (static) de luu tru che do loi hien tai. Mac dinh la khong co loi.
static CheDoChenLoi_t che_do_hien_tai = KHONG_LOI;

// ====================================================================
// HAM BANG DIEU KHIEN: CHON CHE DO LOI QUA HERCULES
// - Ham nay duoc goi tu UART_HamNgatNhan khi nguoi dung go phim 0, 1, 2, 3, 4
// ====================================================================
void SV3_KichHoatLoi(uint8_t ma_phim) {
    if (ma_phim == '0') { 
        char m[]="\\r\\n[SV3] TAT CHEN LOI.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = KHONG_LOI; 
    }
    else if (ma_phim == '1') { 
        char m[]="\\r\\n[SV3] MODE LOI: SE BO GOI TIEP THEO.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_BO_GOI; 
    }
    else if (ma_phim == '2') { 
        char m[]="\\r\\n[SV3] MODE LOI: SE DAO BIT CHECKSUM GOI TIEP THEO.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_DAO_BIT; 
    }
    else if (ma_phim == '3') { 
        char m[]="\\r\\n[SV3] MODE LOI: SE CHEN RAC VAO GOI TIEP THEO.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_CHEN_RAC; 
    }
    else if (ma_phim == '4') { 
        char m[]="\\r\\n[SV3] MODE LOI: SE LAM TRE ACK 4 GIAY.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_TRE_ACK; 
    }
}

// ====================================================================
// TRAM THU PHI SV3: KE PHA HOAI DU LIEU TRUOC KHI GUI XUONG CAP
// - Thay the hoan toan lenh UART_GuiBanGoc trong main.c
// - Kiem soat va "dau doc" phong bi thu truoc khi tha ra ngoai
// ====================================================================
void SV3_GuiCoChenLoi(uint8_t* phong_bi, uint16_t kich_thuoc, bool la_thu_ack) {
    
    // ---------------------------------------------------------
    // 1. BENH MAT MANG (DROP PACKET)
    // ---------------------------------------------------------
    if (che_do_hien_tai == LOI_BO_GOI) {
        char msg[] = "\\r\\n[SV3 - MODULE LOI] Da huy bo goi tin (Drop Packet)!\\r\\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        
        che_do_hien_tai = KHONG_LOI; // Chi pha 1 lan roi tat
        return; // Lenh return lap tuc thoat ham, goi hang bi vut bo vinh vien
    }
    
    // ---------------------------------------------------------
    // 2. BENH SAI LECH DATA (CORRUPT PACKET)
    // ---------------------------------------------------------
    if (che_do_hien_tai == LOI_DAO_BIT) {
        char msg[] = "\\r\\n[SV3 - MODULE LOI] Da dao bit Checksum (Corrupt Packet)!\\r\\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        
        // Kiem tra an toan chong loi Out-of-bounds (treo chip Hard Fault)
        if (kich_thuoc >= 2) {
            // Vi tri [kich_thuoc - 2] luon luon la vi tri cua byte Checksum
            // Phep XOR (^= 0xFF) se lat nguoc toan bo 8 bit cua byte Checksum (VD: 00000001 -> 11111110)
            phong_bi[kich_thuoc - 2] ^= 0xFF; 
        }
        
        // Gui goi hang "loi Checksum" nay ra mang de test xem mach Nhan co biet bo vut di khong
        UART_GuiBanGoc(phong_bi, kich_thuoc);
        che_do_hien_tai = KHONG_LOI; 
        return;
    }
    
    // ---------------------------------------------------------
    // 3. BENH NHIEU SONG (GARBAGE INSERT)
    // ---------------------------------------------------------
    if (che_do_hien_tai == LOI_CHEN_RAC) {
        char msg[] = "\\r\\n[SV3 - MODULE LOI] Da chen rac vao giua khung (Garbage Insert)!\\r\\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        
        // Buoc 1: Gui Nua khuc dau cua goi hang ra cap UART
        UART_GuiBanGoc(phong_bi, kich_thuoc / 2);
        
        // Buoc 2: Nhet 3 byte rac (AA BB CC) vao giua de pha vo cau truc khung SV1
        uint8_t rac[] = {0xAA, 0xBB, 0xCC};
        UART_GuiBanGoc(rac, 3);
        
        // Buoc 3: Doi con tro mang dich di (kich_thuoc / 2) buoc, va gui not Nua khuc sau
        UART_GuiBanGoc(phong_bi + (kich_thuoc / 2), kich_thuoc - (kich_thuoc / 2));
        
        che_do_hien_tai = KHONG_LOI; 
        return;
    }
    
    // ---------------------------------------------------------
    // 4. BENH NGHEN MANG (DELAY ACK)
    // ---------------------------------------------------------
    // Chi ap dung loi nay cho thu ACK (Vi Du an yeu cau "Lam tre ACK")
    if (che_do_hien_tai == LOI_TRE_ACK && la_thu_ack == true) {
        char msg[] = "\\r\\n[SV3 - MODULE LOI] Da giu ACK lai 4 giay (Delay ACK)!\\r\\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        
        // Dong bang CPU trong 4000ms. Lam cho ACK ve tre, cham hon thoi gian Timeout cua mach gui
        HAL_Delay(4000); 
        
        // 4 giay sau moi chiu tha ACK ra cap
        UART_GuiBanGoc(phong_bi, kich_thuoc);
        che_do_hien_tai = KHONG_LOI; 
        return;
    }

    // ---------------------------------------------------------
    // 5. TRANG THAI LUONG THIEN (Khong loi)
    // ---------------------------------------------------------
    // Neu bien trang thai = 0, khong co if nao duoc chay, goi tin duoc day ra an toan
    UART_GuiBanGoc(phong_bi, kich_thuoc);
}
'''
with open(r'Core\Src\error_injection.c', 'w', encoding='utf-8') as f:
    f.write(c_content)