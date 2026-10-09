#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdint.h>

// ====================================================================
// MODULE DO LUONG HIEU NANG (BENCHMARK)
// So sanh thong luong giua Stop-and-Wait va Go-Back-N
// ====================================================================

// Ket qua cua 1 phien benchmark
typedef struct {
    uint32_t tong_goi_gui;        // Tong so goi DATA da gui (bao gom ca truyen lai)
    uint32_t tong_goi_thanh_cong; // Tong so goi nhan duoc ACK (goi "tot")
    uint32_t tong_lan_timeout;    // Tong so lan timeout
    uint32_t thoi_gian_ms;        // Thoi gian chay (ms)
    float    thong_luong;         // Thong luong (goi thanh cong / giay)
} KetQuaBenchmark_t;

// Chay benchmark Stop-and-Wait: gui 'so_goi' goi, tra ve ket qua
void Benchmark_StopAndWait(uint16_t so_goi, uint8_t nhiet_do, uint8_t do_am, KetQuaBenchmark_t* ket_qua);

// Chay benchmark Go-Back-N: gui 'so_goi' goi, tra ve ket qua
void Benchmark_GoBackN(uint16_t so_goi, uint8_t nhiet_do, uint8_t do_am, uint8_t window_size, KetQuaBenchmark_t* ket_qua);

// In ket qua so sanh ra UART (Hercules)
void Benchmark_InKetQua(KetQuaBenchmark_t* saw, KetQuaBenchmark_t* gbn, uint16_t so_goi);

// Ham xu ly chuyen tiep ACK/NACK
void Benchmark_XuLyACK(uint8_t seq);
void Benchmark_XuLyNACK(uint8_t seq);

#endif // BENCHMARK_H
