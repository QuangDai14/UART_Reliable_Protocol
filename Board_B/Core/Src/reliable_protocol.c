#include "reliable_protocol.h"

// ==============================================================================
// BAI TOAN 1: TINH TOAN MA CRC16 (CCITT-FALSE) - NANG CAP TU CHECKSUM
// ==============================================================================
uint16_t GiaoThuc_TinhCRC16(const uint8_t* du_lieu, uint16_t do_dai_du_lieu) {
    uint16_t crc = 0xFFFF; // Gia tri khoi tao
    for (uint16_t i = 0; i < do_dai_du_lieu; i++) {
        crc ^= (uint16_t)du_lieu[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021; // Polynomial chuan CCITT
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// ==============================================================================
// BAI TOAN 2: DONG GOI PHONG BI (HOA TRANG BYTE STUFFING) - UPGRADE CHO SV2
// ==============================================================================
void GiaoThuc_DongGoiThu(const RoDungThu_t* thu_vao, uint8_t* phong_bi_xuat, uint16_t* kich_thuoc_phong_bi) {
    uint16_t idx = 0;
    phong_bi_xuat[idx++] = CO_NIEM_PHONG; // 1. Băng dính Đỏ
    
    // 2. Gom tat ca (Loai, SEQ, Do dai, Ruot) vao Thung tam de de tinh Checksum va Hoa trang
    uint8_t thung_tam[260];
    thung_tam[0] = thu_vao->loai_thu;
    thung_tam[1] = thu_vao->so_thu_tu;
    thung_tam[2] = thu_vao->do_dai;
    for (uint8_t i = 0; i < thu_vao->do_dai; i++) {
        thung_tam[3 + i] = thu_vao->ruot_thu[i];
    }
    
    uint16_t chieu_dai_truoc_crc = 3 + thu_vao->do_dai;
    
    // 3. Tinh CRC16 tren TOAN BO Header va Ruot
    uint16_t crc16 = GiaoThuc_TinhCRC16(thung_tam, chieu_dai_truoc_crc);
    thung_tam[chieu_dai_truoc_crc]     = (uint8_t)(crc16 >> 8);  // Byte cao (MSB)
    thung_tam[chieu_dai_truoc_crc + 1] = (uint8_t)(crc16 & 0xFF);// Byte thap (LSB)
    
    uint16_t tong_so_do_vat = chieu_dai_truoc_crc + 2;
    
    // 4. Kiem tra va HOA TRANG tung mon do (Giong het bai truoc)
    for (uint16_t i = 0; i < tong_so_do_vat; i++) {
        uint8_t mon_do = thung_tam[i];
        if (mon_do == CO_NIEM_PHONG) { 
            phong_bi_xuat[idx++] = TUI_HOA_TRANG;  
            phong_bi_xuat[idx++] = MA_BUP_BE_DO;   
        } 
        else if (mon_do == TUI_HOA_TRANG) { 
            phong_bi_xuat[idx++] = TUI_HOA_TRANG;  
            phong_bi_xuat[idx++] = MA_MON_DO_XANH; 
        } 
        else {
            phong_bi_xuat[idx++] = mon_do; 
        }
    }
    
    phong_bi_xuat[idx++] = CO_NIEM_PHONG; // 5. Bang dinh Do ket thuc
    *kich_thuoc_phong_bi = idx;
}

// ==============================================================================
// BAI TOAN 3: MAY TRANG THAI FSM (BOC TACH BYTE STUFFING) - UPGRADE CHO SV2
// ==============================================================================
static TrangThaiNhan_t state = TRANG_THAI_NGOAI_KHUNG;
static uint8_t bo_dem_tam[260]; 
static uint16_t ngon_tay_tam = 0;

void GiaoThuc_HuyThu(void) {
    state = TRANG_THAI_NGOAI_KHUNG;
    ngon_tay_tam = 0;
}

TrangThaiBocTach_t GiaoThuc_BocTachTungChu(uint8_t chu_cai_nhan, RoDungThu_t* ro_dung_ket_qua) {
    if (chu_cai_nhan == CO_NIEM_PHONG) {
        // Mot buc thu phai co toi thieu 5 Byte: Loai, SEQ, DoDai, CRC_MSB, CRC_LSB.
        if (state == TRANG_THAI_TRONG_KHUNG && ngon_tay_tam >= 5) {
            
            uint8_t loai_doc_duoc = bo_dem_tam[0];
            uint8_t seq_doc_duoc = bo_dem_tam[1];
            uint8_t do_dai_doc_duoc = bo_dem_tam[2];
            uint8_t crc_msb = bo_dem_tam[ngon_tay_tam - 2];
            uint8_t crc_lsb = bo_dem_tam[ngon_tay_tam - 1];
            uint16_t crc_doc_duoc = ((uint16_t)crc_msb << 8) | crc_lsb;
            
            // Kiem tra do dai: Tong so byte (- 3 byte header, - 2 byte CRC) == do_dai_doc_duoc?
            if (do_dai_doc_duoc == (ngon_tay_tam - 5)) {
                
                // Tinh CRC16 tu bo_dem_tam[0] den sat CRC
                uint16_t crc_tu_tinh = GiaoThuc_TinhCRC16(bo_dem_tam, ngon_tay_tam - 2);
                
                if (crc_tu_tinh == crc_doc_duoc) {
                    // Thanh cong ruc ro! Chep vao ro chinh
                    ro_dung_ket_qua->loai_thu = loai_doc_duoc;
                    ro_dung_ket_qua->so_thu_tu = seq_doc_duoc;
                    ro_dung_ket_qua->do_dai = do_dai_doc_duoc;
                    for(uint8_t i = 0; i < do_dai_doc_duoc; i++) {
                        ro_dung_ket_qua->ruot_thu[i] = bo_dem_tam[3 + i];
                    }
                    GiaoThuc_HuyThu(); 
                    return KETQUA_OK;       
                } else {
                    // CRC bi sai (nhieu tren duong truyen hoac do test chen loi)
                    // Chep thong tin header co ban de tra NACK
                    ro_dung_ket_qua->loai_thu = loai_doc_duoc; 
                    ro_dung_ket_qua->so_thu_tu = seq_doc_duoc;
                    GiaoThuc_HuyThu();
                    return KETQUA_LOI_CRC;
                }
            }
        }
        GiaoThuc_HuyThu();
        state = TRANG_THAI_TRONG_KHUNG; 
        return KETQUA_CHUA_XONG;
    }      
    
    switch (state) {
        case TRANG_THAI_TRONG_KHUNG:
            if (chu_cai_nhan == TUI_HOA_TRANG) state = TRANG_THAI_HOA_TRANG;
            else if (ngon_tay_tam < 260) bo_dem_tam[ngon_tay_tam++] = chu_cai_nhan;
            break;
            
        case TRANG_THAI_HOA_TRANG:
            if (chu_cai_nhan == MA_BUP_BE_DO) {
                if (ngon_tay_tam < 260) bo_dem_tam[ngon_tay_tam++] = CO_NIEM_PHONG;
            } else if (chu_cai_nhan == MA_MON_DO_XANH) {
                if (ngon_tay_tam < 260) bo_dem_tam[ngon_tay_tam++] = TUI_HOA_TRANG;
            }
            state = TRANG_THAI_TRONG_KHUNG;
            break;
            
        case TRANG_THAI_NGOAI_KHUNG:
        default:
            break;
    }
    return KETQUA_CHUA_XONG;
}