import os

filepath = r'Core\Src\main.c'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace USER CODE 0 block
begin0_marker = '/* USER CODE BEGIN 0 */'
end0_marker = '/* USER CODE END 0 */'

begin0_idx = content.find(begin0_marker) + len(begin0_marker)
end0_idx = content.find(end0_marker)

new_user_code_0 = '''
// ====================================================================
// KICH BAN 1: TEST SV1 (FRAMING & BYTE STUFFING) - DA KHOA
// ====================================================================
/*
 * KICH BAN TEST SV1:
 * - Mach chi lam nhiem vu "Cai guong": Boc thu ra duoc the nao -> Dong goi lai y nguyen -> Phat tra ve.
 * - Khong quan tam den Timeout, khong quan tam den goi trung hay so thu tu.
 * 
 * VI DU 1 (Binh thuong) : Gui [7E 00 00 01 41 40 7E] -> Mach doi ve y xì đúc.
 * VI DU 2 (Hoa trang)   : Gui [7E 00 00 02 7D 5E 7D 5D 53 7E] -> Mach doi ve y xì đúc.
 */
/*
RoDungThu_t cai_ro_cua_toi;
void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    bool is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    if (is_done == true) {
        uint8_t phong_bi_xuat[256];
        uint16_t kich_thuoc_phong_bi = 0;
        GiaoThuc_DongGoiThu(&cai_ro_cua_toi, phong_bi_xuat, &kich_thuoc_phong_bi); 
        UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi);
    }
}
*/

// ====================================================================
// KICH BAN 2: TEST SV2 (STOP-AND-WAIT ARQ) - DA KHOA
// -> (De test: Xoa cap dau /* va */ bao quanh khoi nay, va lam tuong tu o vong lap while)
// ====================================================================
/*
uint8_t seq_dang_gui = 0;       
uint8_t seq_dang_cho_nhan = 0;  

bool dang_doi_ack = false;
uint32_t thoi_gian_bat_dau_gui = 0;
uint8_t so_lan_truyen_lai = 0;
#define MAX_TRUYEN_LAI 3
uint32_t thoi_gian_chu_dong_gui_lan_cuoi = 0;

RoDungThu_t cai_ro_cua_toi;

void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    bool is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    
    if (is_done == true) {
        if (cai_ro_cua_toi.loai_thu == THU_DATA) {
            char msg[150];
            if (cai_ro_cua_toi.so_thu_tu == seq_dang_cho_nhan) {
                sprintf(msg, "\r\n[MACH B] Nhan OK DATA Seq %d. Lay nhiet do ra xai!\r\n", cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                seq_dang_cho_nhan++; 
            } else {
                sprintf(msg, "\r\n[MACH B] PHAT HIEN GOI TRUNG (Seq %d)! Khong xai data, chi tra loi ACK.\r\n", cai_ro_cua_toi.so_thu_tu);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
            }
            
            RoDungThu_t thu_ack;
            thu_ack.loai_thu = THU_ACK;
            thu_ack.so_thu_tu = cai_ro_cua_toi.so_thu_tu;
            thu_ack.do_dai = 0; 
            
            uint8_t phong_bi_xuat[50];
            uint16_t kich_thuoc_phong_bi = 0;
            GiaoThuc_DongGoiThu(&thu_ack, phong_bi_xuat, &kich_thuoc_phong_bi); 
            UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi);
        }
        else if (cai_ro_cua_toi.loai_thu == THU_ACK) {
            if (dang_doi_ack == true && cai_ro_cua_toi.so_thu_tu == seq_dang_gui) {
                char msg[100];
                sprintf(msg, "\r\n[MACH A] NHAN ACK %d THANH CONG! Hoan thanh phien giao dich.\r\n", seq_dang_gui);
                UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                
                dang_doi_ack = false;
                so_lan_truyen_lai = 0;
                seq_dang_gui++; 
            }
        }
    }
}
*/

// ====================================================================
// KICH BAN 3: TEST SV2 (GO-BACK-N CUA SO TRUOT) - DANG MO DE CHAY
// -> (Neu muon tat: Them cap dau /* va */ de khoa toan bo khoi nay lai)
// ====================================================================
#define GBN_WINDOW_SIZE 4
#define GBN_TOTAL_PACKETS 8

uint8_t base = 0;
uint8_t next_seq_num = 0;
uint32_t thoi_gian_bat_dau_gui = 0;
bool dang_chay_timer = false;

uint8_t seq_dang_cho_nhan = 0; 

RoDungThu_t cai_ro_cua_toi;

void UART_HamNgatNhan(uint8_t chu_cai_nhan) {
    bool is_done = GiaoThuc_BocTachTungChu(chu_cai_nhan, &cai_ro_cua_toi);
    
    if (is_done == true) {
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
            
            // Go-Back-N gui ACK Luy ke (Cumulative). Tuc la bao cao goi chuan cuoi cung nhan duoc.
            if (seq_dang_cho_nhan > 0) {
                RoDungThu_t thu_ack;
                thu_ack.loai_thu = THU_ACK;
                thu_ack.so_thu_tu = seq_dang_cho_nhan - 1; 
                thu_ack.do_dai = 0;
                
                uint8_t phong_bi_xuat[50];
                uint16_t kich_thuoc_phong_bi = 0;
                GiaoThuc_DongGoiThu(&thu_ack, phong_bi_xuat, &kich_thuoc_phong_bi); 
                UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi);
            }
        }
        // MACH A (Nhan ACK gop tu PC)
        else if (cai_ro_cua_toi.loai_thu == THU_ACK) {
            uint8_t ack_seq = cai_ro_cua_toi.so_thu_tu;
            // Kiem tra xem ACK co hop le (Tu base den next_seq_num - 1)
            if (ack_seq >= base && ack_seq < next_seq_num) {
                char msg[150];
                sprintf(msg, "\r\n[MACH A] Nhan duoc ACK GOP (Seq %d). TRUOT CUA SO LEN!\r\n", ack_seq);
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
'''
content = content[:begin0_idx] + '\n' + new_user_code_0 + '\n' + content[end0_idx:]

# Replace USER CODE WHILE block
begin_while_marker = '/* USER CODE BEGIN WHILE */\n  while (1)\n  {'
end_while_marker = '/* USER CODE END WHILE */'

begin_while_idx = content.find(begin_while_marker) + len(begin_while_marker)
end_while_idx = content.find(end_while_marker)

new_while_code = '''
      // ==========================================================
      // KICH BAN 2: STOP-AND-WAIT (DA KHOA)
      // ==========================================================
      /*
      uint32_t thoi_gian_hien_tai = Timer_LayThoiGian();
      if (dang_doi_ack == false) {
          if (thoi_gian_hien_tai - thoi_gian_chu_dong_gui_lan_cuoi >= 5000) {
              thoi_gian_chu_dong_gui_lan_cuoi = thoi_gian_hien_tai;
              char msg[100]; sprintf(msg, "\\r\\n[MACH A] >>> GUI GOI DATA (Seq %d) lan 1...\\r\\n", seq_dang_gui); UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
              RoDungThu_t thu_data; thu_data.loai_thu = THU_DATA; thu_data.so_thu_tu = seq_dang_gui; thu_data.do_dai = 1; thu_data.ruot_thu[0] = 0x99; 
              uint8_t phong_bi_xuat[50]; uint16_t kich_thuoc_phong_bi = 0; GiaoThuc_DongGoiThu(&thu_data, phong_bi_xuat, &kich_thuoc_phong_bi); UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi);
              dang_doi_ack = true; thoi_gian_bat_dau_gui = thoi_gian_hien_tai; so_lan_truyen_lai = 0;
          }
      } else {
          if (thoi_gian_hien_tai - thoi_gian_bat_dau_gui >= 2000) {
              if (so_lan_truyen_lai < MAX_TRUYEN_LAI) {
                  so_lan_truyen_lai++; thoi_gian_bat_dau_gui = thoi_gian_hien_tai;
                  char msg[100]; sprintf(msg, "\\r\\n[MACH A] TIMEOUT! Truyen lai DATA (Seq %d) lan %d...\\r\\n", seq_dang_gui, so_lan_truyen_lai); UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                  RoDungThu_t thu_data; thu_data.loai_thu = THU_DATA; thu_data.so_thu_tu = seq_dang_gui; thu_data.do_dai = 1; thu_data.ruot_thu[0] = 0x99;
                  uint8_t phong_bi_xuat[50]; uint16_t kich_thuoc_phong_bi = 0; GiaoThuc_DongGoiThu(&thu_data, phong_bi_xuat, &kich_thuoc_phong_bi); UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi);
              } else {
                  char msg[] = "\\r\\n[MACH A] THAT BAI! Da dat gioi han truyen lai.\\r\\n"; UART_GuiBanGoc((uint8_t*)msg, strlen(msg));
                  dang_doi_ack = false; seq_dang_gui++; thoi_gian_chu_dong_gui_lan_cuoi = thoi_gian_hien_tai;
              }
          }
      }
      */
      
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
              UART_GuiBanGoc(phong_bi_xuat, kich_thuoc_phong_bi);
              
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
      
    '''
content = content[:begin_while_idx] + '\n' + new_while_code + '\n    ' + content[end_while_idx:]

# Change Hello Message
content = content.replace('char hello_msg[] = "\\r\\n===== TEST SV2 (STOP-AND-WAIT ARQ) =====\\r\\nMach A tu dong gui DATA moi 5 giay.\\r\\nHay dung Hercules de gui ACK phan hoi nhe!\\r\\n";', 'char hello_msg[] = "\\r\\n===== TEST SV2 (GO-BACK-N) =====\\r\\nMach A se ban lien thanh 4 goi.\\r\\nHay test ACK Gop tren Hercules!\\r\\n";')

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(content)