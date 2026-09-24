#include "uart_driver.h"
#include "stm32f1xx.h"
#include <stddef.h>

/*
 * MODULE 1: UART DRIVER (Muc Thanh Ghi)
 * Tac gia: dong Quang dai (Truong nhom)
 * Thiet bi: STM32F103C8T6
 * Chan giao tiep: PA9 (TX - Truyen), PA10 (RX - Nhan)
 */

void UART_Init(void) {
    // ---------------------------------------------------------
    // 1. BaT NGUoN (CLOCK) CHO CaC KHoI NGOaI VI
    // ---------------------------------------------------------
    // Hoi: duong truyen APB2 nghia la gi?
    // Tra loi: Ben trong con chip co mang luoi "duong cao toc" (Bus) de truyen xung nhip va du lieu. 
    // APB2 (Advanced Peripheral Bus 2) la duong cao toc toc do cao. Theo so do phan cung cua ST, 
    // bo USART1 va cong GPIOA duoc han noi vat ly vao duong cao toc APB2 nay. 
    //
    // Hoi: RCC->APB2ENR nghia la gi?
    // Tra loi: RCC (Reset and Clock Control) la "cong ty dien luc" cua chip. 
    // Ky hieu "->" dung de truy cap vao mot thanh ghi (cong tac) ten la APB2ENR nam ben trong bo RCC.
    // Lenh nay tuong duong viec gat cau dao cap dien cho cac thiet bi nam tren bus APB2.
    //
    // Bit 14: Bat dien cho USART1 (USART1EN)
    // Bit 2 : Bat dien cho Cong GPIO A (IOPAEN)
    RCC->APB2ENR |= (1 << 14) | (1 << 2);


    // ---------------------------------------------------------
    // 2. CaU HiNH CHaN PA9 Va PA10 THaNH CHaN CHUYeN DuNG CHO UART
    // ---------------------------------------------------------
    // Hoi: Tai sao chan PA9 bat dau tu bit so 4?
    // Tra loi: Thanh ghi CRH (Control Register High) co 32 bit, quan ly 8 chan tu PA8 den PA15.
    // Theo luat cua ST, moi chan chiem dung 4 o bit.
    // - Chan PA8 chiem bit 0, 1, 2, 3.
    // - Vay chan PA9 (chan tiep theo) se bat dau tu bit so 4 (chiem bit 4, 5, 6, 7).
    // - Chan PA10 se bat dau tu bit so 8 (chiem bit 8, 9, 10, 11).
    //
    // Hoi: Tai sao lenh &= ~ lai la xoa trang?
    // Tra loi: Gia tri 0xFF trong he nhi phan la 11111111 (8 so 1).
    // Dau '~' la phep daO NGuoC. Vay ~0xFF se thanh 00000000 (8 so 0).
    // Phep '&' (AND) co tinh chat: Bat ky so nao AND voi 0 cung ra 0.
    // Vay lenh "&= ~0xFF" nghia la ep 8 bit cua chan PA9 va PA10 (tu bit 4 den bit 11) bien thanh 0 (xoa trang),
    // nhung KHoNG LaM HoNG gia tri cua cac chan khac tren thanh ghi (vi cac chan khac duoc AND voi 1).
    GPIOA->CRH &= ~(0xFF << 4);

    // Ta can nap ma 0x4B (0100 1011) cho 2 chan nay:
    // - PA9 (TX): Chon che do Alternate Function Push-Pull, toc do 50MHz (Ma la 1011 = 0xB)
    // - PA10 (RX): Chon che do Input Floating (Ma la 0100 = 0x4)
    // 
    // Hoi: Chan PA9 dung truoc chan PA10, sao ma nap vao lai la 0100 1011 (0x4B) ma khong phai 1011 0100 (0xB4)?
    // Tra loi: Do quy tac sap xep Bit cua may tinh la tu Phai sang Trai (Bit thap o phai, Bit cao o trai).
    // - Chan PA9 nam o cac Bit thap (Bit 4, 5, 6, 7) -> Phai xep cau hinh cua no (1011) sang BeN PHaI.
    // - Chan PA10 nam o cac Bit cao (Bit 8, 9, 10, 11) -> Phai xep cau hinh cua no (0100) sang BeN TRaI.
    // Ghep lai theo dung vi tri Trai-Phai, ta co: [PA10] [PA9] = 0100 1011 = 0x4B.
    // Neu viet nguoc thanh 0xB4, CPU se tuong ban muon PA9 lam RX va PA10 lam TX!
    // Cuoi cung, day ma 0x4B sang trai 4 bit de khop dung vi tri bat dau cua chan PA9.
    GPIOA->CRH |= (0x4B << 4);


    // ---------------------------------------------------------
    // 3. CaI daT ToC do TRUYeN (BAUDRATE)
    // ---------------------------------------------------------
    // Hoi: Tai sao lai chon toc do Baudrate = 115200?
    // Tra loi: Baudrate la so bit truyen di trong 1 giay. 115200 la con so "chuan quoc dan" pho bien nhat, 
    // vua du nhanh de truyen muot ma khong bi tre (delay), vua du an toan it bi nhieu.
    // Bat buoc ca 2 Board phai cai chung mot toc do thi moi "nghe hieu" tieng cua nhau.
    //
    // Tinh toan: Gia su chip chay HCLK = 72MHz.
    // Cong thuc USARTDIV = 72.000.000 / (16 * 115200) = 39.0625.
    // Phan nguyen = 39 (Quy doi sang Hex la 0x27). Phan thap phan = 0.0625 * 16 = 1 (Hex la 0x1).
    // Ghep 2 manh lai ta duoc gia tri 0x271 de nap vao thanh ghi BRR.
    USART1->BRR = 0x271;


    // ---------------------------------------------------------
    // 4. BaT CaC TiNH NaNG CuA USART1
    // ---------------------------------------------------------
    // Thanh ghi CR1 (Control Register 1) la cong tac dieu khien UART.
    // Bat Bit 13 (UE): Kich hoat toan bo khoi USART1.
    // Bat Bit 3 (TE): Cho phep chan TX truyen du lieu.
    // Bat Bit 2 (RE): Cho phep chan RX lang nghe du lieu.
    // Bat Bit 5 (RXNEIE): Bat che do "Nhay vao ngat khi co du lieu den".
    USART1->CR1 |= (1 << 13) | (1 << 3) | (1 << 2) | (1 << 5);


    // ---------------------------------------------------------
    // 5. BaT BaO doNG NGaT TRONG CPU (NVIC)
    // ---------------------------------------------------------
    // Hoi: NVIC_EnableIRQ(USART1_IRQn); lenh nay nghia la sao?
    // Tra loi: NVIC (Nested Vectored Interrupt Controller) la bo Nao trung tam quan ly moi bao dong cua con chip.
    // o buoc 4, ta moi chi bat cong tac bao dong o ben trong bo UART1. Nhung neu cai Nao trung tam (NVIC) 
    // dang "bit tai" khong chiu nghe chuong tu UART1, thi CPU cung se khong phan ung gi ca.
    // Lenh nay ep Nao trung tam phai "mo tai" ra, chinh thuc cho phep bao dong tu UART1 chay vao CPU.
    NVIC_EnableIRQ(USART1_IRQn);
}

// Ham nay dung de nem mot phong bi (mang du lieu) vao duong truyen
bool UART_SendRaw(const uint8_t* data, uint16_t length) {
    // 1. Kiem tra an toan: Lo ai do nho gui thu nhung dua phong bi rong (NULL) 
    // hoac do dai = 0 thi ta tu choi luon (return false), tranh loi CPU.
    if (data == NULL || length == 0) return false;

    // 2. Vong lap For: Lan luot lay tung ky tu trong phong bi ra de gui di.
    for (uint16_t i = 0; i < length; i++) {
        
        // 3. Vong lap While thu nhat (dung cho phan cung ranh roi):
        // Hoi: Cu phap (!(USART1->SR & (1 << 7))) hoat dong the nao?
        // Tra loi:
        // - Lenh (1 << 7) tao ra mot cai mat na (Mask) mang gia tri nhi phan: 1000 0000.
        // - Lenh '&' (AND) up cai mat na nay vao thanh ghi trang thai SR de soi dung cai Bit so 7. 
        //   (Theo tai lieu, Bit 7 la co TXE - bao hieu Thanh ghi du lieu dang ranh).
        // 
        // - TiNH HUoNG 1 (Khi Bit 7 cua SR dang bang 0):
        //   Phep toan AND ra ket qua 0000 0000 (Trong C, 0 mang nghia la False). 
        //   -> y nghia vat ly: Phan cung dang ban day ky tu cu ra chan cam. Neu do them ky tu moi vao se de mat ky tu cu.
        //   -> Dau '!' (NOT) lat long bien False thanh True. Vong lap while(True) se troi CPU lai, bat dung dam chan tai cho cho doi.
        //
        // - TiNH HUoNG 2 (Khi Bit 7 cua SR da nay len 1): 
        //   Phep toan AND ra ket qua 1000 0000 (So lon hon 0 mang nghia la True). 
        //   -> y nghia vat ly: Phan cung da gui xong, ong truyen da ranh.
        //   -> Dau '!' bien True thanh False. Vong lap while(False) dut xich, giai phong CPU di xuong dong code duoi.
        while (!(USART1->SR & (1 << 7)));
        
        // 4. Khi taxi ranh, nhet 1 ky tu vao thanh ghi DR.
        // Phan cung UART se tu dong nem ky tu nay len day dien de chay sang board ben kia.
        USART1->DR = data[i];
    }
    
    // 5. Vong lap While thu hai (dung cho byte cuoi cung an toan den dich):
    // Hoi: Lenh (!(USART1->SR & (1 << 6))) hoat dong the nao?
    // Tra loi: Logic toan hoc y het nhu tren, nhung lan nay ta soi Bit so 6.
    // - Lenh (1 << 6) tao mat na: 0100 0000.
    // - Lenh '&' soi mat na nay vao thanh ghi SR de lay ra Bit 6.
    //   (Theo tai lieu, Bit 6 la co TC - Transmission Complete, bao hieu qua trinh truyen da HOaN TaT).
    //
    // - TiNH HUoNG 1 (Khi Bit 6 dang bang 0):
    //   -> y nghia vat ly: Du vong lap For da nhet xong ky tu cuoi cung vao thanh ghi DR, nhung phan cung van dang 
    //      hi huc day tung chut tin hieu ra day dien (chua xong han). Neu ta thoat ham ngay luc nay, nho dau ham khac
    //      goi lenh tat nguon UART thi ky tu cuoi cung se bi roi rot giua duong.
    //   -> Phep toan ra False, dau '!' lat thanh True. Vong lap while(True) tiep tuc troi CPU lai them mot chut nua.
    //
    // - TiNH HUoNG 2 (Khi Bit 6 nay len 1):
    //   -> y nghia vat ly: Byte cuoi cung da thuc su roi khoi chan truyen, trot lot 100%.
    //   -> Phep toan ra True, dau '!' lat thanh False. Vong lap while dut xich, ta yen tam ket thuc ham (return true).
    while (!(USART1->SR & (1 << 6)));
    
    // 6. Bao cao gui thanh cong trot lot toan bo mang.
    return true; 
}

// ---------------------------------------------------------
// HaM NGaT PHaN CuNG (Se Tu doNG CHaY KHI Co Du LIeU deN)
// ---------------------------------------------------------
// Hoi: Ham USART1_IRQHandler hoat dong ra sao?
// Tra loi:
// - day la ham "Cuu hoa" (Ngat phan cung - Interrupt). Ban KHoNG BAO GIo phai goi ham nay trong vong lap main().
// - Ten ham nay duoc ST quy dinh chet (co dinh trong file he thong startup). Neu viet sai 1 chu, CPU se khong goi duoc.
// - Bat cu khi nao chan PA10 hut du tin hieu dien va nan ra duoc 1 Byte, phan cung se giat chuong bao dong. 
//   Luc nay, CPU se vut bo moi thu dang lam do o ham main(), va phong voi toc do anh sang vao ham nay de xu ly khan cap.
void USART1_IRQHandler(void) {
    
    // 1. Dung mat na (1 << 5) de soi Bit so 5 cua thanh ghi SR (Co RXNE - bao co du lieu toi).
    // Tai sao phai kiem tra? Vi CPU co the bi keo vao ham ngat nay do cac nguyen nhan khac 
    // (nhu loi ORE, nhieu NE). Phep toan '&' dam bao ta chi lam viec khi chac chan la co du lieu moi.
    if (USART1->SR & (1 << 5)) {
        
        // 2. Nhat du lieu (1 byte) ra khoi thanh ghi DR va cat vao bien rxByte.
        // CaI HAY NHaT CuA STM32: Ngay khoanh khac ban dung lenh doC thanh ghi DR, phan cung se ngam hieu: 
        // "a, chu nhan da lay hang di roi". No se Tu doNG keo co RXNE (Bit 5) tu 1 ve 0. Ban khong can phai code lenh xoa co!
        uint8_t rxByte = USART1->DR;
        
        // 3. day 1 byte vua nhat duoc sang ham UART_RxCallback.
        // Tu duy chia module: Module 1 chi lam tho khuan vac (lay byte tu phan cung).
        // Lay xong nem sang cho Module 2 lam tho soi kiem tra xem byte do la Start, Data hay Checksum.
        UART_RxCallback(rxByte);
    }
}

/* ==============================================================================
 * PHu LuC KIeN THuC: BaNG TRA CuU 10 BIT CuA THANH GHI TRaNG THaI SR (Status Register)
 * ==============================================================================
 * Thanh ghi SR co 32 bit nhung phan cung STM32F1 chi dung 10 bit dau (nhu bang dong ho o to).
 * Duoi day la y nghia chi tiet trang thai cua tung Bit:
 * 
 * [Bit 9] CTS  : dieu khien luong (0 = May kia chua san sang, 1 = May kia da cho phep gui).
 * [Bit 8] LBD  : Dung cho mang LIN o to (0 = Mang binh thuong, 1 = Phat hien dut mang).
 * [Bit 7] TXE  : Trang thai ong truyen DR (0 = Ban, dang go lung day du lieu cu | 1 = Ranh, san sang nhan du lieu moi).
 * [Bit 6] TC   : Hoan tat truyen (0 = Tin hieu dang chay lo do tren day | 1 = Tin hieu cuoi cung da an toan den dich).
 * [Bit 5] RXNE : Trang thai ong nhan DR (0 = Trong rong | 1 = Co du lieu moi bay vao, CPU can ra doc ngay).
 * [Bit 4] IDLE : Bao hieu duong day tinh (0 = dang co tin hieu truyen qua lai | 1 = duong day im ang qua 1 khung truyen).
 * [Bit 3] ORE  : Co loi TRaN (0 = An toan | 1 = LoI: Du lieu moi lao vao de bep du lieu cu do CPU ra doc qua cham).
 * [Bit 2] NE   : Co loi NHIeU (0 = Song sach | 1 = LoI: duong day dien bi nhieu song tu tinh).
 * [Bit 1] FE   : Co loi KHUNG (0 = Khop | 1 = LoI: Hai board cai sai toc do Baudrate hoac sai bit chot Stop bit).
 * [Bit 0] PE   : Co loi PARITY (0 = Chan le dung | 1 = LoI: Tinh chan le bi sai, goi tin bi hong).
 * ============================================================================== */

/* ==============================================================================
 * PHu LuC 2: BaN CHaT VaT Ly CuA QUa TRiNH GuI/NHaN UART (DR vs Shift Register)
 * ==============================================================================
 * Rat nhieu nguoi lam tuong nhet du lieu vao thanh ghi DR la no se chay thang ra day dien. 
 * Su that la co mot vung dem goi la Thanh Ghi Dich (Shift Register) dung o giua.
 *
 * Hinh dung thuc te: 
 * - Thanh ghi DR = Phong cho cua ben xe.
 * - Shift Register = Chiec xe khach (cho 8 nguoi).
 * - Chan cam TX/RX = Cong ben xe.
 *
 * 1. QUa TRiNH GuI (TX):
 * - CPU nhet 1 Byte (Khach VIP) vao Phong cho DR.
 * - Ngay lap tuc, phan cung UART lua Byte do tu DR len Xe khach (Shift Register). 
 * - Luc nay Phong cho DR hoan toan TRoNG. Co TXE nay len 1 bao cho CPU: "Phong ranh roi, nhet khach tiep theo vao di".
 * - Trong khi CPU di lay khach khac, thi chiec Xe khach am tham chay ra cong, day tung bit mot ra day dien.
 * - day xong bit cuoi cung, co TC nay len 1 (Bao hieu da tien xong hoan toan chuyen xe).
 *
 * 2. QUa TRiNH NHaN (RX):
 * - o ngoai cong RX, mot chiec Xe khach dung ha cua gom tung bit dien ap dang bay vao.
 * - Gom du 8 bit (1 Byte hoan chinh), Xe khach chay vao trong va hat ca 8 bit do vao Phong cho DR.
 * - Coi bao dong ru len (Co RXNE nay len 1). Nao NVIC reo CPU: "Co khach o phong cho roi, ra hot ngay!"
 * - CPU chay lenh (rxByte = USART1->DR) de hot Byte do di. Coi tat, Phong cho lai trong rong don chuyen xe sau.
 * ============================================================================== */
