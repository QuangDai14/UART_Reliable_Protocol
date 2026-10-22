/* ============================================================================
 * TÊN FILE       : error_injection.h
 * PHỤ TRÁCH      : SINH VIÊN 3 - Nguyễn Duy Khánh (Module Chèn Lỗi Độc Lập)
 * CHỨC NĂNG CHÍNH: Khai báo các chế độ chèn lỗi và API gửi dữ liệu có can thiệp lỗi nhân tạo.
 * CÁC API CHÍNH  : SV3_KichHoatLoi(), SV3_GuiCoChenLoi()
 * ============================================================================ */
#ifndef ERROR_INJECTION_H
#define ERROR_INJECTION_H

#include <stdint.h>
#include <stdbool.h>

void SV3_KichHoatLoi(uint8_t ma_phim);
void SV3_GuiCoChenLoi(uint8_t* phong_bi, uint16_t kich_thuoc, bool la_thu_ack);

#endif // ERROR_INJECTION_H

