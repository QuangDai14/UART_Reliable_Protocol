#ifndef RELIABLE_PROTOCOL_H
#define RELIABLE_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/* 
 * MODULE 2: RELIABLE PROTOCOL LOGIC
 * Phụ trách đóng gói dữ liệu, kiểm tra Checksum, và phân tích gói tin đến (FSM).
 * Tác giả: Sinh viên 2
 */

// Định nghĩa các byte cờ
#define PROTOCOL_START_BYTE 0xAA
#define PROTOCOL_END_BYTE   0x55
#define PROTOCOL_ACK_BYTE   0x06
#define PROTOCOL_NACK_BYTE  0x15

#define MAX_PAYLOAD_SIZE 64

// Cấu trúc một gói tin (Frame)
typedef struct {
    uint8_t start;
    uint8_t length;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t checksum;
    uint8_t end;
} ProtocolPacket_t;

// Các trạng thái của máy trạng thái nhận (FSM)
typedef enum {
    RX_STATE_WAIT_START = 0,
    RX_STATE_WAIT_LENGTH,
    RX_STATE_WAIT_PAYLOAD,
    RX_STATE_WAIT_CHECKSUM,
    RX_STATE_WAIT_END
} RxState_t;

// Hàm tính Checksum (Tổng các byte dữ liệu)
uint8_t Protocol_CalculateChecksum(const uint8_t* payload, uint8_t length);

// Hàm đóng gói dữ liệu vào buffer để chuẩn bị gửi đi
// data: dữ liệu cần gửi, length: độ dài dữ liệu
// out_buffer: mảng chứa frame hoàn chỉnh, out_length: độ dài của frame
void Protocol_PackData(const uint8_t* data, uint8_t length, uint8_t* out_buffer, uint16_t* out_length);

// Máy trạng thái (FSM) phân tích từng byte nhận được từ UART
// Gọi hàm này trong ngắt UART_RxCallback
// Trả về true nếu đã nhận thành công 1 gói tin hoàn chỉnh và không lỗi
bool Protocol_ParseByte(uint8_t byte, ProtocolPacket_t* out_packet);

// Các hàm tạo nhanh gói tin ACK / NACK
void Protocol_CreateAck(uint8_t* out_buffer, uint16_t* out_length);
void Protocol_CreateNack(uint8_t* out_buffer, uint16_t* out_length);

#endif // RELIABLE_PROTOCOL_H
