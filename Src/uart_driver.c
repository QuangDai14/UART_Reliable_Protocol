#include "uart_driver.h"
#include "stm32f1xx.h" // Thư viện chứa các định nghĩa thanh ghi của STM32F1

/*
 * MODULE 1: UART DRIVER (Mức Thanh Ghi - Register Level)
 * Chip: STM32F103C8T6
 * Chân TX: PA9 | Chân RX: PA10
 */

void UART_Init(void) {
    // 1. Cấp xung nhịp (Clock) cho USART1 và Port A
    // Bit 14: USART1EN, Bit 2: IOPAEN (Port A)
    RCC->APB2ENR |= (1 << 14) | (1 << 2); 

    // 2. Cấu hình chân PA9 (TX) và PA10 (RX) trên thanh ghi CRH của GPIOA
    // Reset cấu hình cũ của PA9 và PA10
    GPIOA->CRH &= ~(0xFF << 4); 
    
    // PA9 (TX): Alternate function output Push-pull, tốc độ 50MHz (MODE=11, CNF=10 => 1011 = 0xB)
    // PA10 (RX): Input floating (MODE=00, CNF=01 => 0100 = 0x4)
    GPIOA->CRH |= (0x4B << 4); 

    // 3. Cấu hình Baudrate (Giả sử hệ thống đang chạy 72MHz, cần Baudrate 115200)
    // USARTDIV = 72,000,000 / (16 * 115200) = 39.0625
    // Mantissa = 39 (0x27), Fraction = 0.0625 * 16 = 1 (0x1) => BRR = 0x271
    USART1->BRR = 0x271;

    // 4. Cấu hình thanh ghi điều khiển CR1
    // Bật USART (UE), bật TX (TE), bật RX (RE), bật ngắt nhận RXNEIE
    USART1->CR1 = (1 << 13) | (1 << 3) | (1 << 2) | (1 << 5);

    // 5. Bật ngắt USART1 trong bộ điều khiển ngắt trung tâm (NVIC)
    NVIC_EnableIRQ(USART1_IRQn);
}

bool UART_SendRaw(const uint8_t* data, uint16_t length) {
    if (data == NULL || length == 0) return false;

    for (uint16_t i = 0; i < length; i++) {
        // Chờ cờ TXE (Transmit data register empty) lên 1 (Thanh ghi dữ liệu trống)
        while (!(USART1->SR & (1 << 7)));
        
        // Ghi dữ liệu vào thanh ghi DR
        USART1->DR = data[i];
    }
    
    // Chờ cờ TC (Transmission complete) lên 1 (Truyền xong hoàn toàn)
    while (!(USART1->SR & (1 << 6)));
    
    return true;
}

// Hàm ngắt phần cứng thực tế của chip STM32F1 cho USART1
// Khi có 1 byte bay vào chân PA10, hàm này sẽ tự động chạy
void USART1_IRQHandler(void) {
    // Kiểm tra cờ RXNE (Read data register not empty)
    if (USART1->SR & (1 << 5)) {
        // Đọc dữ liệu từ thanh ghi DR (việc đọc này sẽ tự động xóa cờ RXNE)
        uint8_t rxByte = USART1->DR;
        
        // Gọi hàm Callback để đẩy byte này sang Module 2 (Giao thức) xử lý
        UART_RxCallback(rxByte);
    }
}
