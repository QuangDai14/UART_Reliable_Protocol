import os

filepath = r'Core\Src\main.c'

new_main_c = '''/* USER CODE BEGIN Header */
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
#include <stdio.h>
#include <string.h>
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ====================================================================
// MODULE SV3: CHEN LOI (ERROR INJECTION MODULE)
// ====================================================================
typedef enum {
    KHONG_LOI = 0,
    LOI_BO_GOI = 1,
    LOI_DAO_BIT = 2,
    LOI_CHEN_RAC = 3,
    LOI_TRE_ACK = 4
} CheDoChenLoi_t;

CheDoChenLoi_t che_do_hien_tai = KHONG_LOI;

void SV3_GuiCoChenLoi(uint8_t* phong_bi, uint16_t kich_thuoc, bool la_thu_ack) {
    if (che_do_hien_tai == LOI_BO_GOI) {
        char msg[] = "\\r\\n[SV3 - MODULE LOI] Da huy bo goi tin (Drop Packet)!\\r\\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        che_do_hien_tai = KHONG_LOI; 
        return; // Khong gui, huy bo truc tiep
    }
    
    if (che_do_hien_tai == LOI_DAO_BIT) {
        char msg[] = "\\r\\n[SV3 - MODULE LOI] Da dao bit Checksum (Corrupt Packet)!\\r\\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        // Dao nguoc toan bo bit cua byte Checksum (nam truoc co ket thuc 0x7E)
        if (kich_thuoc >= 2) {
            phong_bi[kich_thuoc - 2] ^= 0xFF; 
        }
        UART_GuiBanGoc(phong_bi, kich_thuoc);
        che_do_hien_tai = KHONG_LOI; 
        return;
    }
    
    if (che_do_hien_tai == LOI_CHEN_RAC) {
        char msg[] = "\\r\\n[SV3 - MODULE LOI] Da chen rac vao giua khung (Garbage Insert)!\\r\\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        
        // Gui mot nua khung chuan
        UART_GuiBanGoc(phong_bi, kich_thuoc / 2);
        // Chen byte rac mo phong nhieu cap
        uint8_t rac[] = {0xAA, 0xBB, 0xCC};
        UART_GuiBanGoc(rac, 3);
        // Gui tiep nua con lai
        UART_GuiBanGoc(phong_bi + (kich_thuoc / 2), kich_thuoc - (kich_thuoc / 2));
        
        che_do_hien_tai = KHONG_LOI; 
        return;
    }
    
    if (che_do_hien_tai == LOI_TRE_ACK && la_thu_ack == true) {
        char msg[] = "\\r\\n[SV3 - MODULE LOI] Da giu ACK lai 4 giay (Delay ACK)!\\r\\n";
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        HAL_Delay(4000); // Giam giu luong 4 giay
        UART_GuiBanGoc(phong_bi, kich_thuoc);
        che_do_hien_tai = KHONG_LOI; 
        return;
    }

    // Neu hoat dong binh thuong, chuyen qua cap UART luon
    UART_GuiBanGoc(phong_bi, kich_thuoc);
}


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

RoDungThu_t cai_ro_cua_toi;

void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    // ----------------------------------------------------
    // SV3: DIEU KHIEN CHEN LOI QUA HERCULES (Phim ASCII)
    // ----------------------------------------------------
    if (chu_cai_nhan == '0') { 
        char m[]="\\r\\n[SV3] TAT CHEN LOI.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = KHONG_LOI; return; 
    }
    if (chu_cai_nhan == '1') { 
        char m[]="\\r\\n[SV3] MODE LOI: SE BO GOI TIEP THEO.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_BO_GOI; return; 
    }
    if (chu_cai_nhan == '2') { 
        char m[]="\\r\\n[SV3] MODE LOI: SE DAO BIT CHECKSUM GOI TIEP THEO.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_DAO_BIT; return; 
    }
    if (chu_cai_nhan == '3') { 
        char m[]="\\r\\n[SV3] MODE LOI: SE CHEN RAC VAO GOI TIEP THEO.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_CHEN_RAC; return; 
    }
    if (chu_cai_nhan == '4') { 
        char m[]="\\r\\n[SV3] MODE LOI: SE LAM TRE ACK 4 GIAY.\\r\\n"; UART_GuiBanGoc((uint8_t*)m, strlen(m)); 
        che_do_hien_tai = LOI_TRE_ACK; return; 
    }

    bool is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    
    if (is_done == true) {
        // MACH B (Nhan DATA)
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            char msg[150];
            if (cai_ro_cua_toi.so_thu_tu == seq_dang_cho_nhan) {
                sprintf(msg, "\\r\\n[MACH B] Nhan DUNG THU TU (Seq %d). Lay ra xai!\\r\\n", cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                seq_dang_cho_nhan++; 
            } else {
                sprintf(msg, "\\r\\n[MACH B] SAI THU TU! Dang doi Seq %d ma lai nhan Seq %d. Vut data!\\r\\n", seq_dang_cho_nhan, cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            }
            
            // Go-Back-N gui ACK Luy ke (Cumulative). Tuc la bao cao goi chuan cuoi cung nhan duoc.
            if (seq_dang_cho_nhan > 0) {
                RoDungThu_t thu_ack;
                thu_ack.loai_thu = THU_ACK;
                thu_ack.so_thu_tu = seq_dang_cho_nhan - 1; 
                thu_ack.do_dai = 0;
                
                uint8_t phong_bi_xuat[50];
                uint16_t kich_thuoc_phong_bi = 0;
                GiaoThuc_DongGoiThu(&thu_ack, phong_bi_xuat, &kich_thuoc_phong_bi); 
                
                // SV3: Di qua cua hai quan chen loi (Dau true bao hieu day la thu ACK)
                SV3_GuiCoChenLoi(phong_bi_xuat, kich_thuoc_phong_bi, true); 
            }
        }
        // MACH A (Nhan ACK gop tu PC)
        else if (cai_ro_cua_toi.loai_thu == THU_ACK) {
            uint8_t ack_seq = cai_ro_cua_toi.so_thu_tu;
            // Kiem tra xem ACK co hop le (Tu base den next_seq_num - 1)
            if (ack_seq >= base && ack_seq < next_seq_num) {
                char msg[150];
                sprintf(msg, "\\r\\n[MACH A] Nhan duoc ACK GOP (Seq %d). TRUOT CUA SO LEN!\\r\\n", ack_seq);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                
                // Truot cua so len (Vuot qua cai ACK do)
                base = ack_seq + 1;
                
                if (base == next_seq_num) {
                    dang_chay_timer = false; // Da duoc xac nhan het, tat dong ho
                } else {
                    thoi_gian_bat_dau_gui = Timer_LayThoiGian(); // Reset dong ho cho cac goi con lai
                }
            }
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
  HAL_Init();
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */
  SystemClock_Config();
  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  UART_KhoiTao();
  Timer_KhoiTao(); 
  
  char hello_msg[] = "\\r\\n=========================================\\r\\n"
                     "===== TEST SV2 (GBN) + SV3 (CHEN LOI) =====\\r\\n"
                     "Go cac phim 1, 2, 3, 4 tren Hercules de Kich hoat Loi\\r\\n"
                     "Go phim 0 de huy che do loi.\\r\\n"
                     "=========================================\\r\\n";
  UART_GuiBanGoc((uint8_t*)hello_msg, strlen(hello_msg));
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // ==========================================================
      // KICH BAN 3: GO-BACK-N (DANG MO DE CHAY)
      // ==========================================================
      if (base < GBN_TOTAL_PACKETS) {
          
          // 1. BAN LIEN THANH: Ban lien tiep cac goi nam trong Cua So (Chua duoc ACK)
          while (next_seq_num < base + GBN_WINDOW_SIZE && next_seq_num < GBN_TOTAL_PACKETS) {
              char msg[100];
              sprintf(msg, "\\r\\n[MACH A] Go-Back-N: Ban lien thanh goi (Seq %d)...\\r\\n", next_seq_num);
              UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
              
              RoDungThu_t thu_data;
              thu_data.loai_thu = THU_DATA;
              thu_data.so_thu_tu = next_seq_num;
              thu_data.do_dai = 1;
              thu_data.ruot_thu[0] = 0x88; 
              
              uint8_t phong_bi_xuat[50];
              uint16_t kich_thuoc_phong_bi = 0;
              GiaoThuc_DongGoiThu(&thu_data, phong_bi_xuat, &kich_thuoc_phong_bi); 
              
              // SV3: Di qua cua hai quan chen loi (Dau false bao hieu day la DATA)
              SV3_GuiCoChenLoi(phong_bi_xuat, kich_thuoc_phong_bi, false);
              
              // Chi bat dau tinh Timeout cho goi dau tien cua Cua so (Goi cu nhat)
              if (base == next_seq_num) {
                  thoi_gian_bat_dau_gui = Timer_LayThoiGian();
                  dang_chay_timer = true;
              }
              
              next_seq_num++;
              HAL_Delay(300); // Co tinh Delay 300ms giua cac phat ban de ban kip nhin tren man hinh
          }
          
          // 2. TIMEOUT & LUI CUA SO (GO-BACK-N)
          if (dang_chay_timer == true) {
              // Doi 5 giay xem co ai them gui ACK luy ke ve khong
              if (Timer_LayThoiGian() - thoi_gian_bat_dau_gui >= 5000) { 
                  char msg[150];
                  sprintf(msg, "\\r\\n[MACH A] TIMEOUT! Kich hoat GO-BACK-N. Huy cua so, keo ve truyen lai toan bo tu Seq %d...\\r\\n", base);
                  UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                  
                  // LUI CUA SO: Keo next_seq_num ve lai bang chinh xac base de vong lap while(1) sau se ban lai het
                  next_seq_num = base;
                  dang_chay_timer = false; // Tat tam timer cho no ban lai tu dau
                  HAL_Delay(2000); // Nghi ngoi 2 giay de lay suc ban lai
              }
          }
      } else {
          // Da gui thanh cong ca 8 goi. Xong viec!
          char msg[] = "\\r\\n[HOAN THANH] Da gui va nhan ACK cho toan bo 8 goi Go-Back-N!\\r\\n";
          UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
          HAL_Delay(10000); // Nghi 10 giay roi test lai tu dau
          
          base = 0;
          next_seq_num = 0;
          seq_dang_cho_nhan = 0;
          char msg2[] = "\\r\\n[RESET] Bat dau phien Go-Back-N moi...\\r\\n";
          UART_GuiBanGoc((uint8_t*)msg2, strlen(msg2));
          HAL_Delay(2000);
      }
    /* USER CODE END WHILE */

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

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
'''

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(new_main_c)