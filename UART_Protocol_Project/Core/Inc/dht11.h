/*
 * dht11.h
 * Thu vien doc cam bien nhiet do - do am DHT11
 * Giao thuc: 1-Wire (1 day du lieu duy nhat)
 * Chan du lieu: PA1
 */

#ifndef INC_DHT11_H_
#define INC_DHT11_H_

#include "main.h"

/* Cau truc luu ket qua doc tu DHT11 */
typedef struct {
    uint8_t do_am;       // Do am (%) - Phan nguyen
    uint8_t nhiet_do;    // Nhiet do (C) - Phan nguyen
    uint8_t checksum_ok; // 1 = Du lieu hop le, 0 = Du lieu bi loi
} DHT11_Data_t;

/* Khoi tao bo dem thoi gian micro-giay (DWT) */
void DHT11_KhoiTao(void);

/* Doc du lieu tu cam bien DHT11.
 * Tra ve: 1 = Doc thanh cong, 0 = Doc that bai (cam bien khong phan hoi) */
uint8_t DHT11_DocDuLieu(DHT11_Data_t *data);

#endif /* INC_DHT11_H_ */
