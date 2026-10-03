/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uart_driver.h" 
#include "reliable_protocol.h" 
#include "timer_driver.h" 
#include "error_injection.h" // <--- THU VIEN MODULE 3 DA DUOC GOI VAO
#include <stdio.h>
#include <string.h>
#include "Anglas_OLED_SSD1306.h"
#include "dht11.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ====================================================================
// KICH BAN 1: TEST SV1 (FRAMING & BYTE STUFFING)
// -> DANG KHOA (De mo: Xoa cap dau /* va */ bao quanh)
// ====================================================================
/*
RoDungThu_t cai_ro_cua_toi;

void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    bool is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    if (is_done == true) {
        char msg[150];
        sprintf(msg, "\r\n[MACH B] Bóc tách SV1 THÀNH CÔNG! Loai: %d, Seq: %d, Dai: %d\r\n", 
                cai_ro_cua_toi.loai_thu, cai_ro_cua_toi.so_thu_tu, cai_ro_cua_toi.do_dai);
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    }
}
// Trong vong lap while(1): Khong can ghi gi them vi chi can test mach nhan
*/


// ====================================================================
// KICH BAN 2: TEST SV2 (STOP-AND-WAIT ARQ)
// -> DANG KHOA (De mo: Xoa cap dau /* va */ bao quanh)
// ====================================================================
/*
#define MAX_TRUYEN_LAI 3

uint8_t seq_gui_hien_tai = 0;
uint8_t so_lan_truyen_lai = 0;
uint32_t thoi_gian_bat_dau_gui = 0;
bool dang_cho_ack = false;
uint8_t seq_dang_cho_nhan = 0;
RoDungThu_t cai_ro_cua_toi;

void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    // 1. Cho phep nhan nut kich hoat Loi SV3 
    if (chu_cai_nhan >= '0' && chu_cai_nhan <= '4') { SV3_KichHoatLoi(chu_cai_nhan); return; }

    bool is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    if (is_done == true) {
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            if (cai_ro_cua_toi.so_thu_tu == seq_dang_cho_nhan) {
                char msg[] = "\r\n[MACH B] Nhan DUNG THU TU. Lay data & Tra loi ACK...\r\n";
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                seq_dang_cho_nhan = 1 - seq_dang_cho_nhan; // Dao bit 0 <-> 1
            } else {
                char msg[] = "\r\n[MACH B] Trung lap goi tin. Vut bo data & Tra lai ACK...\r\n";
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            }
            
            // Gui Giay bien nhan (ACK)
            RoDungThu_t thu_ack;
            thu_ack.loai_thu = THU_ACK;
            thu_ack.so_thu_tu = cai_ro_cua_toi.so_thu_tu;
            thu_ack.do_dai = 0;
            uint8_t p_xuat[50]; uint16_t k_thuoc = 0;
            GiaoThuc_DongGoiThu(&thu_ack, p_xuat, &k_thuoc);
            SV3_GuiCoChenLoi(p_xuat, k_thuoc, true); // Di qua tram thu phi SV3
        }
        else if (cai_ro_cua_toi.loai_thu == THU_ACK) {
            if (cai_ro_cua_toi.so_thu_tu == seq_gui_hien_tai && dang_cho_ack == true) {
                char msg[] = "\r\n[MACH A] Nhan duoc ACK. Giao hang thanh cong!\r\n";
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                dang_cho_ack = false;
                so_lan_truyen_lai = 0;
                seq_gui_hien_tai = 1 - seq_gui_hien_tai; // Dao bit 0 <-> 1 de gui goi tiep theo
            }
        }
    }
}

// ============= TRONG VONG LAP while(1) =============
// if (dang_cho_ack == false) {
//     char msg[100];
//     sprintf(msg, "\r\n[MACH A] Dang gui goi tin (Seq %d)...\r\n", seq_gui_hien_tai);
//     UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
//     RoDungThu_t thu_data;
//     thu_data.loai_thu = THU_DATA; thu_data.so_thu_tu = seq_gui_hien_tai; thu_data.do_dai = 1; thu_data.ruot_thu[0] = 0x88;
//     uint8_t phong_bi[50]; uint16_t kich_thuoc = 0;
//     GiaoThuc_DongGoiThu(&thu_data, phong_bi, &kich_thuoc);
//     SV3_GuiCoChenLoi(phong_bi, kich_thuoc, false); // Di qua tram thu phi SV3
//     dang_cho_ack = true;
//     thoi_gian_bat_dau_gui = Timer_LayThoiGian();
// } else {
//     if (Timer_LayThoiGian() - thoi_gian_bat_dau_gui >= 3000) { // Timeout 3s
//         if (so_lan_truyen_lai < MAX_TRUYEN_LAI) {
//             so_lan_truyen_lai++;
//             char msg[] = "\r\n[MACH A] TIMEOUT! Dang truyen lai goi tin...\r\n";
//             UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
//             // Goi ham gui lai ...
//             thoi_gian_bat_dau_gui = Timer_LayThoiGian();
//         } else {
//             char msg[] = "\r\n[MACH A] THAT BAI! Ket noi bi mat hoan toan.\r\n";
//             UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
//             HAL_Delay(5000); // Nghi 5s roi thu lai tu dau
//             so_lan_truyen_lai = 0;
//             dang_cho_ack = false;
//         }
//     }
// }
*/

// ====================================================================
// KICH BAN 3: TEST SV2 (GO-BACK-N CUA SO TRUOT)
// ====================================================================
/* 
 * ---------------- HUONG DAN TEST GO-BACK-N TREN HERCULES ----------------
 * CHUAN BI HEX O HERCULES:
 *   - DATA 0: 7E 00 00 01 88 89 7E (De test Mach B)
 *   - DATA 1: 7E 00 01 01 88 88 7E (De test Mach B)
 *   - DATA 2: 7E 00 02 01 88 8B 7E (De test Mach B)
 *   - ACK 3 : 7E 01 03 00 02 7E    (De test Mach A - Xac nhan 0, 1, 2, 3)
 *   - ACK 7 : 7E 01 07 00 06 7E    (De test Mach A - Xac nhan 4, 5, 6, 7)
 * ------------------------------------------------------------------------
 */
#define GBN_WINDOW_SIZE 4
#define GBN_TOTAL_PACKETS 8

uint8_t base = 0;
uint8_t next_seq_num = 0;
uint32_t thoi_gian_bat_dau_gui = 0;
bool dang_chay_timer = false;

uint8_t seq_dang_cho_nhan = 0; 

// --- BIEN THONG KE BOARD A ---
uint32_t tong_goi_gui     = 0;  // Tong so goi DATA da gui
uint32_t tong_lan_timeout  = 0;  // Tong so lan Timeout phai truyen lai

// --- DU LIEU CAM BIEN DE GUI DI ---
uint8_t nhiet_do_gui = 0;
uint8_t do_am_gui    = 0;

RoDungThu_t cai_ro_cua_toi;

void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    TrangThaiBocTach_t is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    
    if (is_done == KETQUA_OK) {
        // MACH B (Nhan DATA)
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            char msg[150];
            if (cai_ro_cua_toi.so_thu_tu == seq_dang_cho_nhan) {
                sprintf(msg, "\r\n[MACH B] Nhan DUNG THU TU (Seq %d). Lay ra xai!\r\n", cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                seq_dang_cho_nhan++; 
            } else {
                sprintf(msg, "\r\n[MACH B] SAI THU TU! Dang doi Seq %d ma lai nhan Seq %d. Vut data!\r\n", seq_dang_cho_nhan, cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            }
            
            // Go-Back-N gui ACK Luy ke
            if (seq_dang_cho_nhan > 0) {
                RoDungThu_t thu_ack;
                thu_ack.loai_thu = THU_ACK;
                thu_ack.so_thu_tu = seq_dang_cho_nhan - 1; 
                thu_ack.do_dai = 0;
                
                uint8_t phong_bi_xuat[50];
                uint16_t kich_thuoc_phong_bi = 0;
                GiaoThuc_DongGoiThu(&thu_ack, phong_bi_xuat, &kich_thuoc_phong_bi); 
                
                // Goi thu vien SV3 de kiem soat viec chen loi
                SV3_GuiCoChenLoi(phong_bi_xuat, kich_thuoc_phong_bi, true); 
            }
        }
        // MACH A (Nhan ACK)
        else if (cai_ro_cua_toi.loai_thu == THU_ACK) {
            uint8_t ack_seq = cai_ro_cua_toi.so_thu_tu;
            if (ack_seq >= base && ack_seq < next_seq_num) {
                char msg[150];
                sprintf(msg, "\r\n[MACH A] Nhan duoc ACK GOP (Seq %d). TRUOT CUA SO LEN!\r\n", ack_seq);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                
                base = ack_seq + 1;
                
                if (base == next_seq_num) {
                    dang_chay_timer = false; 
                } else {
                    thoi_gian_bat_dau_gui = Timer_LayThoiGian(); 
                }
            }
        }
        // MACH A (Nhan NACK do Board B phan hoi khi CRC sai)
        else if (cai_ro_cua_toi.loai_thu == THU_NACK) {
            char msg[150];
            sprintf(msg, "\r\n[MACH A] Nhan duoc NACK (Loi CRC tai Seq %d). TRUYEN LAI NGAY!\r\n", cai_ro_cua_toi.so_thu_tu);
            UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            
            // Go-Back-N keo nhanh next_seq ve base de truyen lai ngay lap tuc ma khong can doi Timeout
            next_seq_num = base;
            dang_chay_timer = false;
        }
    } 
    // NEU GOI TIN BI SAI MA CRC16 (Do nhieu tren duong day hoac SV3 co tinh chen loi)
    else if (is_done == KETQUA_LOI_CRC) {
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            char msg[150];
            sprintf(msg, "\r\n[MACH B] PHAT HIEN SAI MA CRC16 (Goi Seq %d). Tra ve NACK...\r\n", cai_ro_cua_toi.so_thu_tu);
            UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            
            RoDungThu_t thu_nack;
            thu_nack.loai_thu = THU_NACK;
            thu_nack.so_thu_tu = cai_ro_cua_toi.so_thu_tu;
            thu_nack.do_dai = 0;
            
            uint8_t phong_bi_xuat[50];
            uint16_t kich_thuoc_phong_bi = 0;
            GiaoThuc_DongGoiThu(&thu_nack, phong_bi_xuat, &kich_thuoc_phong_bi); 
            SV3_GuiCoChenLoi(phong_bi_xuat, kich_thuoc_phong_bi, true); // Gui tra NACK cho Board A
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  UART_KhoiTao();
  Timer_KhoiTao(); 

  OLED_Init();
  OLED_Clear();
  OLED_Print_Text(0, 0, 1, "NHOM 10 - BOARD A");
  OLED_Print_Text(2, 0, 1, "Nhiet do:     C");
  OLED_Print_Text(4, 0, 1, "Do am  :     %");
  OLED_Print_Text(6, 0, 1, "Gui:    TO:");

  DHT11_KhoiTao();
  
  char hello_msg[] = "\r\n=========================================\r\n"
                     "===== BOARD A - BEN PHAT DU LIEU =====\r\n"
                     "Go cac phim 1, 2, 3, 4 de kich hoat loi\r\n"
                     "Go phim 0 de huy che do loi.\r\n"
                     "=========================================\r\n";
  UART_GuiBanGoc((uint8_t*)hello_msg, strlen(hello_msg));
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* ---- DOC NUT NHAN VAT LY CO CHONG NHIEU (DEBOUNCE) BANG THANH GHI ---- */
      {
          uint8_t c1 = 0, c2 = 0, c3 = 0, c4 = 0;
          for(int i=0; i<5; i++) {
              if (~GPIOB->IDR & (1 << 12)) c1++;
              if (~GPIOB->IDR & (1 << 13)) c2++;
              if (~GPIOB->IDR & (1 << 14)) c3++;
              if (~GPIOB->IDR & (1 << 15)) c4++;
              HAL_Delay(2); // Lay mau 5 lan, moi lan cach 2ms
          }
          if (c1 >= 4) { SV3_KichHoatLoi('1'); HAL_Delay(500); }
          else if (c2 >= 4) { SV3_KichHoatLoi('2'); HAL_Delay(500); }
          else if (c3 >= 4) { SV3_KichHoatLoi('3'); HAL_Delay(500); }
          else if (c4 >= 4) { SV3_KichHoatLoi('4'); HAL_Delay(500); }
      }

      /* ---- DOC CAM BIEN DHT11 MOI 2 GIAY ---- */
      {
          static uint32_t lan_doc_cuoi = 0;
          if (Timer_LayThoiGian() - lan_doc_cuoi >= 2000) {
              lan_doc_cuoi = Timer_LayThoiGian();
              DHT11_Data_t cam_bien;
              if (DHT11_DocDuLieu(&cam_bien)) {
                  nhiet_do_gui = cam_bien.nhiet_do;
                  do_am_gui    = cam_bien.do_am;
                  char txt[20];
                  sprintf(txt, "%d", nhiet_do_gui);
                  OLED_Print_Text(2, 60, 1, "   ");
                  OLED_Print_Text(2, 60, 1, txt);
                  sprintf(txt, "%d", do_am_gui);
                  OLED_Print_Text(4, 60, 1, "   ");
                  OLED_Print_Text(4, 60, 1, txt);
              }
          }
      }

      /* ---- CAP NHAT THONG KE OLED MOI 500ms ---- */
      {
          static uint32_t lan_tk_cuoi = 0;
          if (Timer_LayThoiGian() - lan_tk_cuoi >= 500) {
              lan_tk_cuoi = Timer_LayThoiGian();
              char txt[20];
              sprintf(txt, "%lu", tong_goi_gui);
              OLED_Print_Text(6, 24, 1, "    ");
              OLED_Print_Text(6, 24, 1, txt);
              sprintf(txt, "%lu", tong_lan_timeout);
              OLED_Print_Text(6, 78, 1, "   ");
              OLED_Print_Text(6, 78, 1, txt);
          }
      }

      /* ---- LOGIC GO-BACK-N (GUI GOI TIN CHUA DU LIEU CAM BIEN) ---- */
      if (base < GBN_TOTAL_PACKETS) {
          while (next_seq_num < base + GBN_WINDOW_SIZE && next_seq_num < GBN_TOTAL_PACKETS) {
              char msg[100];
              sprintf(msg, "\r\n[BOARD A] GBN: Gui goi (Seq %d) - T:%dC, H:%d%%\r\n", 
                      next_seq_num, nhiet_do_gui, do_am_gui);
              UART_GuiBanGoc((uint8_t*)msg, strlen(msg));

              RoDungThu_t thu_data;
              thu_data.loai_thu = THU_DATA;
              thu_data.so_thu_tu = next_seq_num;
              thu_data.do_dai = 2; // 2 byte: nhiet do + do am
              thu_data.ruot_thu[0] = nhiet_do_gui;
              thu_data.ruot_thu[1] = do_am_gui;

              uint8_t phong_bi_xuat[50];
              uint16_t kich_thuoc_phong_bi = 0;
              GiaoThuc_DongGoiThu(&thu_data, phong_bi_xuat, &kich_thuoc_phong_bi);
              SV3_GuiCoChenLoi(phong_bi_xuat, kich_thuoc_phong_bi, false);

              tong_goi_gui++;

              if (base == next_seq_num) {
                  thoi_gian_bat_dau_gui = Timer_LayThoiGian();
                  dang_chay_timer = true;
              }
              next_seq_num++;
              HAL_Delay(300);
          }

          if (dang_chay_timer == true) {
              if (Timer_LayThoiGian() - thoi_gian_bat_dau_gui >= 5000) {
                  char msg[150];
                  sprintf(msg, "\r\n[BOARD A] TIMEOUT! Go-Back-N truyen lai tu Seq %d...\r\n", base);
                  UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                  next_seq_num = base;
                  dang_chay_timer = false;
                  tong_lan_timeout++;
                  HAL_Delay(2000);
              }
          }
      } else {
          char msg[] = "\r\n[HOAN THANH] Da gui xong toan bo 8 goi Go-Back-N!\r\n";
          UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
          HAL_Delay(10000);
          base = 0; next_seq_num = 0; seq_dang_cho_nhan = 0;
          char msg2[] = "\r\n[RESET] Bat dau phien Go-Back-N moi...\r\n";
          UART_GuiBanGoc((uint8_t*)msg2, strlen(msg2));
          HAL_Delay(2000);
      }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  // ---------------------------------------------------------
  // CAU HINH NUT NHAN (PB12-PB15) BANG THANH GHI CHUAN YEU CAU DO AN
  // ---------------------------------------------------------
  RCC->APB2ENR |= (1 << 3); // Bat Clock GPIOB
  
  // Xoa cau hinh cu va set Input Pull-up/Pull-down (0x8) cho PB12-PB15
  GPIOB->CRH &= 0x0000FFFF;
  GPIOB->CRH |= 0x88880000;
  
  // Bat Pull-Up de ghim dien ap muc Cao (chong nhieu)
  GPIOB->ODR |= (1 << 12) | (1 << 13) | (1 << 14) | (1 << 15);
  
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

/* USER CODE BEGIN 5 */

/* USER CODE END 5 */
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
