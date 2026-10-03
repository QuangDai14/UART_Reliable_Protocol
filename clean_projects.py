import os
import shutil
import re

base_dir = r"C:\Users\Quang Dai\Desktop\He_Thong_Nhung_1"
projects = ["SV1_Framing", "SV2_StopAndWait", "SV2_GoBackN", "SV3_ErrorInjection"]

# 1. Xoa cac file ngoai vi
for proj in projects:
    src_dir = os.path.join(base_dir, proj, "Core", "Src")
    inc_dir = os.path.join(base_dir, proj, "Core", "Inc")
    
    files_to_remove = [
        os.path.join(src_dir, "dht11.c"),
        os.path.join(inc_dir, "dht11.h"),
        os.path.join(src_dir, "Anglas_OLED_SSD1306.c"),
        os.path.join(inc_dir, "Anglas_OLED_SSD1306.h")
    ]
    for f in files_to_remove:
        if os.path.exists(f):
            os.remove(f)

# 2. Doc noi dung main.c goc lam template
with open(os.path.join(base_dir, "UART_Protocol_Project", "Core", "Src", "main.c"), "r", encoding="utf-8") as f:
    main_content = f.read()

# Ham ho tro thay the code block
def replace_block(content, begin_marker, end_marker, new_code):
    pattern = re.compile(rf"({re.escape(begin_marker)}).*?({re.escape(end_marker)})", re.DOTALL)
    return pattern.sub(rf"\1\n{new_code}\n  \2", content)

# --- SV1: FRAMING ---
sv1_main = main_content
sv1_main = replace_block(sv1_main, "/* USER CODE BEGIN Includes */", "/* USER CODE END Includes */", 
'''#include "uart_driver.h"
#include "reliable_protocol.h"
#include <stdio.h>
#include <string.h>''')
sv1_main = replace_block(sv1_main, "/* USER CODE BEGIN 0 */", "/* USER CODE END 0 */",
'''RoDungThu_t cai_ro_cua_toi;
void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    bool is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    if (is_done) {
        char msg[150];
        sprintf(msg, "\\r\\n[SV1] Boc tach THANH CONG! Loai: %d, Seq: %d, Dai: %d\\r\\n", 
                cai_ro_cua_toi.loai_thu, cai_ro_cua_toi.so_thu_tu, cai_ro_cua_toi.do_dai);
        UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
    }
}''')
sv1_main = replace_block(sv1_main, "/* USER CODE BEGIN 2 */", "/* USER CODE END 2 */",
'''  UART_KhoiTao();
  UART_GuiBanGoc((uint8_t*)"\\r\\n=== TEST MODULE 1 (FRAMING) ===\\r\\n", 37);''')
sv1_main = replace_block(sv1_main, "/* USER CODE BEGIN WHILE */", "/* USER CODE END WHILE */",
'''  while (1)
  {
      // SV1: Tu viet code tao goi tin va gui di de kiem tra byte stuffing o day
      HAL_Delay(2000);''')
with open(os.path.join(base_dir, "SV1_Framing", "Core", "Src", "main.c"), "w", encoding="utf-8") as f:
    f.write(sv1_main)

# --- SV2: STOP AND WAIT ---
sv2_sw_main = main_content
sv2_sw_main = replace_block(sv2_sw_main, "/* USER CODE BEGIN Includes */", "/* USER CODE END Includes */", 
'''#include "uart_driver.h"
#include "reliable_protocol.h"
#include "timer_driver.h"
#include <stdio.h>
#include <string.h>''')
sv2_sw_main = replace_block(sv2_sw_main, "/* USER CODE BEGIN 0 */", "/* USER CODE END 0 */",
'''#define MAX_TRUYEN_LAI 3
uint8_t seq_gui = 0;
uint8_t so_lan_truyen_lai = 0;
uint32_t tg_bat_dau = 0;
bool dang_cho_ack = false;
uint8_t seq_cho_nhan = 0;
RoDungThu_t r;

void UART_HamNgatNhan(uint8_t c) {
    if (GiaoThuc_BocTachTungChu(c, &r)) {
        if (r.loai_thu == THU_DATA && r.so_thu_tu == seq_cho_nhan) {
            UART_GuiBanGoc((uint8_t*)"\\r\\n[B] Nhan DATA dung. Tra ACK.\\r\\n", 32);
            seq_cho_nhan = 1 - seq_cho_nhan;
            // Tra ACK ... (SV2 tu code them)
        }
        else if (r.loai_thu == THU_ACK && r.so_thu_tu == seq_gui && dang_cho_ack) {
            UART_GuiBanGoc((uint8_t*)"\\r\\n[A] Nhan ACK ok!\\r\\n", 21);
            dang_cho_ack = false;
            so_lan_truyen_lai = 0;
            seq_gui = 1 - seq_gui;
        }
    }
}''')
sv2_sw_main = replace_block(sv2_sw_main, "/* USER CODE BEGIN 2 */", "/* USER CODE END 2 */",
'''  UART_KhoiTao();
  Timer_KhoiTao();
  UART_GuiBanGoc((uint8_t*)"\\r\\n=== TEST MODULE 2 (STOP-AND-WAIT) ===\\r\\n", 43);''')
sv2_sw_main = replace_block(sv2_sw_main, "/* USER CODE BEGIN WHILE */", "/* USER CODE END WHILE */",
'''  while (1)
  {
      if (!dang_cho_ack) {
          // Goi goi tin moi
          dang_cho_ack = true;
          tg_bat_dau = Timer_LayThoiGian();
          HAL_Delay(3000); // Gia lap thoi gian cho
      } else {
          if (Timer_LayThoiGian() - tg_bat_dau >= 3000) {
              UART_GuiBanGoc((uint8_t*)"\\r\\n[A] TIMEOUT! Truyen lai...\\r\\n", 31);
              tg_bat_dau = Timer_LayThoiGian();
          }
      }''')
with open(os.path.join(base_dir, "SV2_StopAndWait", "Core", "Src", "main.c"), "w", encoding="utf-8") as f:
    f.write(sv2_sw_main)

# --- SV2: GO BACK N ---
sv2_gbn_main = main_content
sv2_gbn_main = replace_block(sv2_gbn_main, "/* USER CODE BEGIN Includes */", "/* USER CODE END Includes */", 
'''#include "uart_driver.h"
#include "reliable_protocol.h"
#include "timer_driver.h"
#include <stdio.h>
#include <string.h>''')
# Giu nguyen logic Go-Back-N trong phan 0
# Xoa bo OLED va DHT11 trong phan 2
sv2_gbn_main = replace_block(sv2_gbn_main, "/* USER CODE BEGIN 2 */", "/* USER CODE END 2 */",
'''  UART_KhoiTao();
  Timer_KhoiTao(); 
  UART_GuiBanGoc((uint8_t*)"\\r\\n=== TEST MODULE 2 (GO-BACK-N) ===\\r\\n", 39);''')
# Trong while, bo doan doc DHT11
sv2_gbn_main = re.sub(r"/\* ---- DOC CAM BIEN DHT11 MOI 2 GIAY ---- \*/.*?/\* ---- LOGIC GO-BACK-N \(GUI GOI TIN\) ---- \*/", "/* ---- LOGIC GO-BACK-N (GUI GOI TIN) ---- */", sv2_gbn_main, flags=re.DOTALL)
with open(os.path.join(base_dir, "SV2_GoBackN", "Core", "Src", "main.c"), "w", encoding="utf-8") as f:
    f.write(sv2_gbn_main)

# --- SV3: ERROR INJECTION ---
sv3_main = main_content
sv3_main = replace_block(sv3_main, "/* USER CODE BEGIN Includes */", "/* USER CODE END Includes */", 
'''#include "uart_driver.h"
#include "reliable_protocol.h"
#include "error_injection.h"
#include <stdio.h>
#include <string.h>''')
sv3_main = replace_block(sv3_main, "/* USER CODE BEGIN 0 */", "/* USER CODE END 0 */",
'''void UART_HamNgatNhan(uint8_t c) {
    if (c >= '0' && c <= '4') {
        SV3_KichHoatLoi(c);
    }
}''')
sv3_main = replace_block(sv3_main, "/* USER CODE BEGIN 2 */", "/* USER CODE END 2 */",
'''  UART_KhoiTao();
  UART_GuiBanGoc((uint8_t*)"\\r\\n=== TEST MODULE 3 (ERROR INJECTION) ===\\r\\n"
                             "Nhan 1,2,3,4 de test cac kieu loi.\\r\\n", 83);''')
sv3_main = replace_block(sv3_main, "/* USER CODE BEGIN WHILE */", "/* USER CODE END WHILE */",
'''  while (1)
  {
      // SV3: Viet code test viec goi ham chen loi vao goi tin o day
      HAL_Delay(2000);''')
with open(os.path.join(base_dir, "SV3_ErrorInjection", "Core", "Src", "main.c"), "w", encoding="utf-8") as f:
    f.write(sv3_main)

print("Done!")
