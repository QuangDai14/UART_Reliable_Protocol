#ifndef ERROR_INJECTION_H
#define ERROR_INJECTION_H

#include <stdint.h>
#include <stdbool.h>

void SV3_KichHoatLoi(uint8_t ma_phim);
void SV3_GuiCoChenLoi(uint8_t* phong_bi, uint16_t kich_thuoc, bool la_thu_ack);

#endif // ERROR_INJECTION_H
