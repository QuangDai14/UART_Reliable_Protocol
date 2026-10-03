/*
 * dht11.c
 * Thu vien doc cam bien nhiet do - do am DHT11
 *
 * GIAO THUC HOAT DONG CUA DHT11 (1-Wire):
 * =========================================
 * Buoc 1: MCU keo chan DATA xuong thap (LOW) trong 18ms de "danh thuc" DHT11.
 * Buoc 2: MCU tha chan DATA len cao (HIGH), doi DHT11 tra loi.
 * Buoc 3: DHT11 keo LOW 80us, roi HIGH 80us de bao "toi da san sang".
 * Buoc 4: DHT11 gui 40 bit du lieu (5 byte):
 *          Byte 1: Do am phan nguyen
 *          Byte 2: Do am phan thap phan (luon = 0 voi DHT11)
 *          Byte 3: Nhiet do phan nguyen
 *          Byte 4: Nhiet do phan thap phan (luon = 0 voi DHT11)
 *          Byte 5: Checksum = Byte1 + Byte2 + Byte3 + Byte4
 * Moi bit bat dau bang 50us LOW, sau do:
 *   - Neu HIGH keo dai 26-28us => Bit 0
 *   - Neu HIGH keo dai 70us    => Bit 1
 */

#include "dht11.h"

/* ============================================================
 * CHAN KET NOI DHT11
 * ============================================================ */
#define DHT11_PORT   GPIOA
#define DHT11_PIN    GPIO_PIN_1

/* ============================================================
 * HAM TRE MICRO-GIAY (Dung bo dem DWT cua loi ARM Cortex-M3)
 * ============================================================
 * Giai thich: Bo dem DWT->CYCCNT dem so chu ky clock cua CPU.
 * Voi chip chay 72MHz, 1 micro-giay = 72 chu ky clock.
 * Ta chi can doi cho CYCCNT dem du so chu ky tuong ung la xong.
 */
static void delay_us(uint32_t us) {
    uint32_t startVal = DWT->CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - startVal) < delayTicks);
}

/* ============================================================
 * KHOI TAO BO DEM DWT VA CHAN GPIO
 * ============================================================ */
void DHT11_KhoiTao(void) {
    /* Bat bo dem DWT (Data Watchpoint and Trace) */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* Bat clock cho GPIOA (neu chua bat) */
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

/* ============================================================
 * CAU HINH CHAN DATA THANH CHAN XUAT (OUTPUT)
 * De MCU co the keo LOW/HIGH gui tin hieu cho DHT11
 * ============================================================ */
static void DHT11_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   /* Push-Pull: Day duoc ca HIGH lan LOW */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

/* ============================================================
 * CAU HINH CHAN DATA THANH CHAN NHAP (INPUT)
 * De MCU lang nghe tin hieu DHT11 gui ve
 * ============================================================ */
static void DHT11_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;        /* Che do doc tin hieu */
    GPIO_InitStruct.Pull = GPIO_PULLUP;             /* Keo len cao khi khong ai keo */
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

/* ============================================================
 * DOC 1 BYTE (8 BIT) TU DHT11
 * ============================================================ */
static uint8_t DHT11_DocMotByte(void) {
    uint8_t byte_doc = 0;
    uint8_t i;

    for (i = 0; i < 8; i++) {
        /* Doi cho chan DATA keo LOW (bat dau 1 bit moi) */
        uint32_t timeout = 10000;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
            if (--timeout == 0) return 0; /* Timeout bao ve */
        }

        /* Do thoi gian HIGH:
         * - Neu HIGH ngan (26-28us) => Bit 0
         * - Neu HIGH dai (70us)     => Bit 1
         * Cach lam: Doi 40us roi doc chan. Neu van HIGH => Bit 1. */
        delay_us(40);

        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
            byte_doc |= (1 << (7 - i)); /* Ghi bit 1 vao vi tri tuong ung */
        }

        /* Doi cho HIGH ket thuc (ve LOW) truoc khi doc bit tiep theo */
        timeout = 10000;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
            if (--timeout == 0) return 0;
        }
    }

    return byte_doc;
}

/* ============================================================
 * HAM CHINH: DOC DU LIEU TU CAM BIEN DHT11
 * ============================================================
 * Tra ve: 1 = Thanh cong, 0 = That bai
 * ============================================================ */
uint8_t DHT11_DocDuLieu(DHT11_Data_t *data) {
    uint8_t bytes[5]; /* 5 byte du lieu tu DHT11 */
    uint32_t timeout;

    /* ---- BUOC 1: MCU GUI TIN HIEU BAT DAU ---- */
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET); /* Keo LOW */
    HAL_Delay(20);                                              /* Giu LOW 20ms */
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);   /* Tha len HIGH */
    delay_us(30);                                               /* Doi 30us */

    /* ---- BUOC 2: CHUYEN SANG CHE DO NGHE ---- */
    DHT11_SetInput();

    /* ---- BUOC 3: DOI DHT11 TRA LOI ---- */
    /* DHT11 se keo LOW 80us */
    timeout = 10000;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        if (--timeout == 0) return 0; /* DHT11 khong phan hoi */
    }

    /* Doi het giai doan LOW 80us */
    timeout = 10000;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
        if (--timeout == 0) return 0;
    }

    /* Doi het giai doan HIGH 80us */
    timeout = 10000;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        if (--timeout == 0) return 0;
    }

    /* ---- BUOC 4: DOC 40 BIT (5 BYTE) DU LIEU ---- */
    bytes[0] = DHT11_DocMotByte(); /* Byte 1: Do am phan nguyen */
    bytes[1] = DHT11_DocMotByte(); /* Byte 2: Do am phan thap phan */
    bytes[2] = DHT11_DocMotByte(); /* Byte 3: Nhiet do phan nguyen */
    bytes[3] = DHT11_DocMotByte(); /* Byte 4: Nhiet do phan thap phan */
    bytes[4] = DHT11_DocMotByte(); /* Byte 5: Checksum */

    /* ---- BUOC 5: KIEM TRA CHECKSUM ---- */
    uint8_t checksum = bytes[0] + bytes[1] + bytes[2] + bytes[3];
    if (checksum == bytes[4]) {
        data->do_am = bytes[0];
        data->nhiet_do = bytes[2];
        data->checksum_ok = 1;
        return 1; /* Thanh cong */
    } else {
        data->do_am = 0;
        data->nhiet_do = 0;
        data->checksum_ok = 0;
        return 0; /* Checksum sai => Du lieu bi nhieu */
    }
}
