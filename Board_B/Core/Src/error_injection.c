#include "error_injection.h"
#include "uart_driver.h"
#include "delay.h"
#include <string.h>

// Tu dien cac loai benh ly (Loi) can gia lap tren mang
typedef enum {
    KHONG_LOI = 0,    // Trang thai mang khoe manh binh thuong
    LOI_BO_GOI = 1,   // Gia lap dut cap, mat goi tin tren duong truyen
    LOI_DAO_BIT = 2,  // Gia lap nhieu dien tu lam lat bit Checksum
    LOI_CHEN_RAC = 3, // Gia lap bi thiet bi la chen song, chen du lieu rac vao giua
    LOI_TRE_ACK = 4   // Gia lap nghen mang, thu ACK bi ket xe ve tre
} CheDoChenLoi_t;

// Bien toan cuc (static) luu tru che do loi. Mac dinh la khong co loi.
static CheDoChenLoi_t che_do_hien_tai = KHONG_LOI;

// ====================================================================
// HAM BANG DIEU KHIEN: CHON CHE DO LOI QUA HERCULES
// ====================================================================
void SV3_KichHoatLoi(uint8_t ma_phim) {
    if (ma_phim == '0') { 
        char m[]="\r\n[SV3] TAT CHEN LOI.\r\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = KHONG_LOI; 
    }
    else if (ma_phim == '1') { 
        char m[]="\r\n[SV3] MODE LOI: SE BO GOI TIEP THEO.\r\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_BO_GOI; 
    }
    else if (ma_phim == '2') { 
        char m[]="\r\n[SV3] MODE LOI: SE DAO BIT CHECKSUM GOI TIEP THEO.\r\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_DAO_BIT; 
    }
    else if (ma_phim == '3') { 
        char m[]="\r\n[SV3] MODE LOI: SE CHEN RAC VAO GOI TIEP THEO.\r\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_CHEN_RAC; 
    }
    else if (ma_phim == '4') { 
        char m[]="\r\n[SV3] MODE LOI: SE LAM TRE ACK 4 GIAY.\r\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_TRE_ACK; 
    }
}

// ====================================================================
// TRAM THU PHI SV3: KE PHA HOAI DU LIEU TRUOC KHI GUI XUONG CAP
// Bien la_thu_ack (true/false) duoc truyen vao tu main.c de giup
// Tram thu phi biet duoc day la thu DATA hay thu ACK ma khong can doc ruot.
// Nho do giup viec "Loc doi tuong" de dang va nhanh chong hon.
// ====================================================================
void SV3_GuiCoChenLoi(uint8_t* phong_bi, uint16_t kich_thuoc, bool la_thu_ack) {
    
    // ---------------------------------------------------------
    // 1. BENH MAT MANG (DROP PACKET)
    // ---------------------------------------------------------
    if (che_do_hien_tai == LOI_BO_GOI) {
        char msg[] = "\r\n[SV3 - MODULE LOI] Da huy bo goi tin (Drop Packet)!\r\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        
        che_do_hien_tai = KHONG_LOI; // Chi pha 1 lan roi tat de chong treo mang
        return; // Lenh return lap tuc thoat ham, goi hang bi vut bo vinh vien, khong the toi lenh Gui
    }
    
    // ---------------------------------------------------------
    // 2. BENH NHIEU DIEN TU (CORRUPT PACKET - LAT BIT CHECKSUM)
    // ---------------------------------------------------------
    if (che_do_hien_tai == LOI_DAO_BIT) {
        char msg[] = "\r\n[SV3 - MODULE LOI] Da dao bit Checksum (Corrupt Packet)!\r\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        
        // if (kich_thuoc >= 2): Tam khien bao ve chong loi vuot qua mang (Out of bounds) gay treo Hard Fault STM32
        if (kich_thuoc >= 2) {
            // phong_bi[kich_thuoc - 2] luon luon la byte Checksum (Dung truoc co 0x7E cuoi cung)
            // Phep XOR (^= 0xFF) lat nguoc toan bo 8 bit. Vi du: 00000001 thanh 11111110. 
            // Cach nay gia lap hoan hao viec nhieu dien tu lam sai lech du lieu tren duong truyen.
            phong_bi[kich_thuoc - 2] ^= 0xFF; 
        }
        
        // Gui goi hang "loi Checksum" nay ra mang
        UART_GuiBanGoc(phong_bi, kich_thuoc);
        che_do_hien_tai = KHONG_LOI; 
        return;
    }
    
    // ---------------------------------------------------------
    // 3. BENH NHIEU SONG (GARBAGE INSERT - CAT DOI KHUC GIO)
    // ---------------------------------------------------------
    if (che_do_hien_tai == LOI_CHEN_RAC) {
        char msg[] = "\r\n[SV3 - MODULE LOI] Da chen rac vao giua khung (Garbage Insert)!\r\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        
        // Nhat chem 1: Gui Nua khuc dau cua goi hang ra cap UART
        UART_GuiBanGoc(phong_bi, kich_thuoc / 2);
        
        // Nhat chem 2: Nhet 3 byte rac (AA BB CC) vao giua de pha vo cau truc
        uint8_t rac[] = {0xAA, 0xBB, 0xCC};
        UART_GuiBanGoc(rac, 3);
        
        // Nhat chem 3: Doi con tro mang dich di (kich_thuoc / 2) buoc, va gui not Nua khuc sau
        // Vi du: phong_bi + 3 nghia la bat dau lay tu byte thu 4 tro di
        UART_GuiBanGoc(phong_bi + (kich_thuoc / 2), kich_thuoc - (kich_thuoc / 2));
        
        che_do_hien_tai = KHONG_LOI; 
        return;
    }
    
    // ---------------------------------------------------------
    // 4. BENH NGHEN MANG (DELAY ACK)
    // ---------------------------------------------------------
    // Su dung "la_thu_ack == true" de loc doi tuong. Loi delay 4 giay NAY CHI DUOC PHEP AP DUNG cho thu ACK.
    // Neu la thu DATA (mang the can cuoc "false"), bieu thuc IF se Sai va no se duoc di thang qua.
    if (che_do_hien_tai == LOI_TRE_ACK && la_thu_ack == true) {
        char msg[] = "\r\n[SV3 - MODULE LOI] Da giu ACK lai 4 giay (Delay ACK)!\r\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        
        // Dong bang CPU trong 4000ms. Lam cho ACK ve tre, hien tuong Lag mang xay ra
        delay_ms(4000); 
        
        // 4 giay sau moi chiu tha ACK ra cap. Ben gui se nghi la dut cap va truyen lai DATA, gay ra loi trung lap (Duplicate)
        UART_GuiBanGoc(phong_bi, kich_thuoc);
        che_do_hien_tai = KHONG_LOI; 
        return;
    }

    // ---------------------------------------------------------
    // KET BAI: NGUOI LUONG THIEN (Khong co loi hoac IF bi sai)
    // ---------------------------------------------------------
    // Neu khong bi loi gi, hoac co loi 4 nhung day lai la thu DATA, no se chay thang xuong day va tha di an toan
    UART_GuiBanGoc(phong_bi, kich_thuoc);
}
