#include "timer_driver.h"
#include "main.h" // De lay cac Dinh nghia thanh ghi cua con chip

/* ==========================================================
 * MODULE 3: DONG HO THOI GIAN THUC (SYSTICK TIMER)
 * Nhiem vu: Tao ra mot cai dong ho bam gio (1ms) de canh chung Timeout.
 * ========================================================== */

// ==========================================================
// GIAI THICH TU KHOA 'volatile' (Luat cam luoi bieng cua CPU):
// - Neu khong co volatile: Trinh bien dich nghi bien nay khong ai sua,
//   nen no luu vao bo nho tam (Register) doc cho nhanh -> Khong bat duoc 
//   khoanh khac bien bi thay doi ngam o duoi ham ngat -> Treo chuong trinh.
// - Co volatile: Cam CPU dung tri nho. Moi lan dung bien nay BAT BUOC 
//   phai chay xuong tan kho RAM de doc gia tri moi nhat. 
// ==========================================================
volatile uint32_t thoi_gian_ms = 0;


// ==========================================================
// BAI TOAN 1: KHOI TAO DONG HO (1 MILI-GIAY KEU 1 LAN)
// ==========================================================
void Timer_KhoiTao(void) {
    
    // GIAI THICH THANH GHI LOAD VA VAL (Ban chat cua dong ho):
    // 1. SysTick->LOAD (Ngan chua muc cai dat - Diem xuat phat):
    //    Giong nhu num van tren lo vi song. Vidu toc do chip la 72 Trieu dao dong/s.
    //    De dem 1ms (1/1000 giay), ta van num LOAD = 72000.
    SysTick->LOAD = (SystemCoreClock / 1000) - 1; 
    
    // 2. SysTick->VAL (Man hinh hien thi thoi gian dang dem lui):
    //    Khi ta viet 1 so bat ky vao day (vi du so 0), no giong nhu an nut Reset:
    //    No xoa sach "rac bo nho" dang bi ket, roi Tu DONG chep con so 72000 tu 
    //    ngan LOAD do sang de bat dau cuoc dem lui moi (71999, 71998... 0).
    SysTick->VAL = 0;                             
    
    // Bat dong ho chay (Gom 3 thao tac gop vao bang mui ten >> va dau |): 
    // Bit 0 (ENABLE)    : 1 (Cho phep chay)
    // Bit 1 (TICKINT)   : 1 (Cho phep goi ham ngat moi khi dem ve 0)
    // Bit 2 (CLKSOURCE) : 1 (Dung toc do cao nhat cua nhan ARM)
    SysTick->CTRL = (1 << 2) | (1 << 1) | (1 << 0);
}


/* Ham SysTick_Handler da duoc chuyen sang stm32f1xx_it.c de tranh xung dot voi CubeMX */


// ==========================================================
// BAI TOAN 3: XUAT GIA TRI CHO TEP KHAC DUNG
// ==========================================================
uint32_t Timer_LayThoiGian(void) {
    return thoi_gian_ms;
}