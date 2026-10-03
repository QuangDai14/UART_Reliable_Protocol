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
#include <stdio.h>
#include <string.h>
#include "Anglas_OLED_SSD1306.h"
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
// BOARD B - BEN NHAN: Nhan du lieu tu Board A, kiem tra CRC16,
// tra ACK/NACK, hien thi du lieu cam bien + thong ke len OLED
// ====================================================================

// --- BIEN THONG KE HIEN THI TREN OLED ---
uint32_t tong_goi_nhan  = 0;  // Tong so goi DATA nhan dung
uint32_t tong_loi_crc   = 0;  // Tong so goi bi sai CRC
uint32_t tong_goi_trung = 0;  // Tong so goi bi trung lap (duplicate)

// --- BIEN DIEU KHIEN NHAN GO-BACK-N ---
uint8_t seq_dang_cho_nhan = 0; // So thu tu goi dang cho nhan tiep theo

// --- DU LIEU CAM BIEN NHAN TU BOARD A ---
uint8_t nhiet_do_nhan = 0;
uint8_t do_am_nhan    = 0;
bool    co_du_lieu_moi = false; // Co du lieu moi can cap nhat OLED

RoDungThu_t cai_ro_cua_toi;

void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    TrangThaiBocTach_t is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    
    if (is_done == KETQUA_OK) {
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            char msg[150];
            if (cai_ro_cua_toi.so_thu_tu == seq_dang_cho_nhan) {
                // NHAN DUNG THU TU - Lay du lieu ra xai
                sprintf(msg, "\r\n[BOARD B] Nhan DUNG (Seq %d). Nhiet do: %d, Do am: %d\r\n", 
                        cai_ro_cua_toi.so_thu_tu,
                        (cai_ro_cua_toi.do_dai >= 2) ? cai_ro_cua_toi.ruot_thu[0] : 0,
                        (cai_ro_cua_toi.do_dai >= 2) ? cai_ro_cua_toi.ruot_thu[1] : 0);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                
                // Lay du lieu cam bien tu goi tin
                if (cai_ro_cua_toi.do_dai >= 2) {
                    nhiet_do_nhan = cai_ro_cua_toi.ruot_thu[0];
                    do_am_nhan    = cai_ro_cua_toi.ruot_thu[1];
                    co_du_lieu_moi = true;
                }
                
                tong_goi_nhan++;
                seq_dang_cho_nhan++;
                
                // KHI NHAN DU 8 GOI (0 DEN 7), RESET VE 0 DE DONG BO VOI BOARD A
                if (seq_dang_cho_nhan >= 8) {
                    seq_dang_cho_nhan = 0;
                    UART_GuiBanGoc((uint8_t*)"\r\n[BOARD B] Da nhan du 8 goi. Reset Seq ve 0 de doi phien moi!\r\n", 64);
                }
            } else {
                // GOI TRUNG LAP hoac SAI THU TU
                sprintf(msg, "\r\n[BOARD B] TRUNG LAP! Doi Seq %d, nhan Seq %d. Vut data!\r\n", 
                        seq_dang_cho_nhan, cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                tong_goi_trung++;
            }
            
            // Go-Back-N: Luon tra ACK luy ke (Cumulative ACK) cho goi dung gan nhat
            RoDungThu_t thu_ack;
            thu_ack.loai_thu = THU_ACK;
            
            // Xy ly ACK khi seq_dang_cho_nhan vua bi reset ve 0 (tuc la dang can tra ACK 7)
            if (seq_dang_cho_nhan == 0) {
                thu_ack.so_thu_tu = 7;
            } else {
                thu_ack.so_thu_tu = seq_dang_cho_nhan - 1;
            }
            thu_ack.do_dai = 0;
            
            uint8_t phong_bi_xuat[50];
            uint16_t kich_thuoc_phong_bi = 0;
            GiaoThuc_DongGoiThu(&thu_ack, phong_bi_xuat, &kich_thuoc_phong_bi); 
            UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi); // Gui ACK ve Board A
        }
    } 
    // GOI TIN BI SAI CRC16 -> Tra ve NACK
    else if (is_done == KETQUA_LOI_CRC) {
        char msg[100];
        sprintf(msg, "\r\n[BOARD B] LOI CRC16 (Seq %d)! Tra NACK...\r\n", cai_ro_cua_toi.so_thu_tu);
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
        tong_loi_crc++;
        
        RoDungThu_t thu_nack;
        thu_nack.loai_thu = THU_NACK;
        thu_nack.so_thu_tu = cai_ro_cua_toi.so_thu_tu;
        thu_nack.do_dai = 0;
        
        uint8_t phong_bi_xuat[50];
        uint16_t kich_thuoc_phong_bi = 0;
        GiaoThuc_DongGoiThu(&thu_nack, phong_bi_xuat, &kich_thuoc_phong_bi); 
        UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi); // Gui NACK ve Board A
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
  OLED_Print_Text(0, 0, 1, "NHOM 10 - BOARD B");
  OLED_Print_Text(2, 0, 1, "Nhiet do:     C");
  OLED_Print_Text(4, 0, 1, "Do am  :     %");
  OLED_Print_Text(6, 0, 1, "OK:    Err:   D:");
  
  char hello_msg[] = "\r\n=========================================\r\n"
                     "===== BOARD B - BEN NHAN DU LIEU =====\r\n"
                     "=========================================\r\n";
  UART_GuiBanGoc((uint8_t*)hello_msg, strlen(hello_msg));
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* ---- CAP NHAT OLED MOI 500ms ---- */
      {
          static uint32_t lan_cap_nhat_cuoi = 0;
          if (Timer_LayThoiGian() - lan_cap_nhat_cuoi >= 500) {
              lan_cap_nhat_cuoi = Timer_LayThoiGian();
              char txt[20];
              
              // Hien thi nhiet do va do am nhan tu Board A
              if (co_du_lieu_moi) {
                  sprintf(txt, "%d", nhiet_do_nhan);
                  OLED_Print_Text(2, 60, 1, "   ");
                  OLED_Print_Text(2, 60, 1, txt);
                  sprintf(txt, "%d", do_am_nhan);
                  OLED_Print_Text(4, 60, 1, "   ");
                  OLED_Print_Text(4, 60, 1, txt);
                  co_du_lieu_moi = false;
              }
              
              // Hien thi thong ke: OK (nhan dung), Err (loi CRC), D (trung lap)
              sprintf(txt, "%lu", tong_goi_nhan);
              OLED_Print_Text(6, 18, 1, "    ");
              OLED_Print_Text(6, 18, 1, txt);
              
              sprintf(txt, "%lu", tong_loi_crc);
              OLED_Print_Text(6, 72, 1, "   ");
              OLED_Print_Text(6, 72, 1, txt);
              
              sprintf(txt, "%lu", tong_goi_trung);
              OLED_Print_Text(6, 102, 1, "   ");
              OLED_Print_Text(6, 102, 1, txt);
          }
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
