#ifndef RELIABLE_PROTOCOL_H
#define RELIABLE_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// ==============================================================================
// 1. DINH NGHIA CAC TRANG THAI CUA MAY FSM (BAC BAO VE UPGRADE BYTE STUFFING)
// ==============================================================================
typedef enum {
    TRANG_THAI_NGOAI_KHUNG = 0, 
    TRANG_THAI_TRONG_KHUNG,     
    TRANG_THAI_HOA_TRANG        
} TrangThaiNhan_t;

#define CO_NIEM_PHONG   0x7E  
#define TUI_HOA_TRANG   0x7D  
#define MA_BUP_BE_DO    0x5E  
#define MA_MON_DO_XANH  0x5D  

// ==============================================================================
// 2. DINH NGHIA CAU TRUC CUA MOT BUC THU (PACKET) - DA UPGRADE CHO SV2
// ==============================================================================
// Dinh nghia cac loai thu (De biet duong tra loi ACK)
typedef enum {
    THU_DATA = 0, // Thu chua du lieu cam bien binh thuong
    THU_ACK  = 1, // Thu bao "Da nhan thanh cong" (Khong co ruot)
    THU_NACK = 2  // Thu bao "Loi Checksum roi" (Khong co ruot)
} LoaiThu_t;

typedef struct {
    uint8_t loai_thu;     // DATA, ACK, hoac NACK
    uint8_t so_thu_tu;    // So thu tu (Sequence Number: 0, 1, 2...)
    uint8_t do_dai;       // Chieu dai cua mang ruot_thu
    uint8_t ruot_thu[250]; 
} RoDungThu_t;

// ==============================================================================
// 3. KHAI BAO CAC HAM DE FILE KHAC GOI DUOC (API)
// ==============================================================================
typedef enum {
    KETQUA_CHUA_XONG = 0,
    KETQUA_OK = 1,
    KETQUA_LOI_CRC = 2
} TrangThaiBocTach_t;

uint16_t GiaoThuc_TinhCRC16(const uint8_t* du_lieu, uint16_t do_dai_du_lieu);
void GiaoThuc_DongGoiThu(const RoDungThu_t* thu_vao, uint8_t* phong_bi_xuat, uint16_t* kich_thuoc_phong_bi);
TrangThaiBocTach_t GiaoThuc_BocTachTungChu(uint8_t chu_cai_nhan, RoDungThu_t* ro_dung_ket_qua);
void GiaoThuc_HuyThu(void);
TrangThaiNhan_t GiaoThuc_LayTrangThai(void);

#endif // RELIABLE_PROTOCOL_H