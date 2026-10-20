function Prepend-Comment {
    param([string]$Path, [string]$Student, [string]$Description)
    $filename = Split-Path $Path -Leaf
    $comment = "/* ============================================================================
 * Tên File    : $filename
 * Tác giả     : $Student
 * Chức năng   : $Description
 * ============================================================================ */

"
    
    # Kiem tra file ton tai tren Board A
    $pathA = "UART_Protocol_Project/Core/$Path"
    if (Test-Path $pathA) {
        $content = Get-Content $pathA -Raw
        if (-not $content.StartsWith("/* ============================================================================")) {
            $newContent = $comment + $content
            Set-Content $pathA $newContent -Encoding UTF8
        }
    }
    
    # Kiem tra file ton tai tren Board B
    $pathB = "Board_B/Core/$Path"
    if (Test-Path $pathB) {
        $content = Get-Content $pathB -Raw
        if (-not $content.StartsWith("/* ============================================================================")) {
            $newContent = $comment + $content
            Set-Content $pathB $newContent -Encoding UTF8
        }
    }
}

# 1. SV1
Prepend-Comment "Src/reliable_protocol.c" "Sinh viên 1 (Tầng Khung)" "Đóng khung dữ liệu, nhồi byte (COBS), mã hóa CRC16. Máy trạng thái (FSM) bóc tách byte và tự đồng bộ lại khi nhiễu."
Prepend-Comment "Inc/reliable_protocol.h" "Sinh viên 1 (Tầng Khung)" "Đóng khung dữ liệu, nhồi byte (COBS), mã hóa CRC16. Máy trạng thái (FSM) bóc tách byte và tự đồng bộ lại khi nhiễu."

Prepend-Comment "Src/uart_driver.c" "Sinh viên 1 (Tầng Khung)" "Giao tiếp phần cứng UART. Gửi byte thô và bắt Ngắt nhận (Rx Interrupt) đẩy lên cho Máy trạng thái (FSM) xử lý."
Prepend-Comment "Inc/uart_driver.h" "Sinh viên 1 (Tầng Khung)" "Giao tiếp phần cứng UART. Gửi byte thô và bắt Ngắt nhận (Rx Interrupt) đẩy lên cho Máy trạng thái (FSM) xử lý."

# 2. SV2 + SV4 (App Board A)
Prepend-Comment "Src/app_board_a.c" "Sinh viên 2 (Tầng Tin Cậy) & Sinh viên 4 (Tầng Ứng dụng)" "Điều khiển máy phát (Board A). Thuật toán Cửa sổ trượt (Go-Back-N Sender), Timeout (SV2). Đọc DHT11, quét nút, cập nhật OLED (SV4)."
Prepend-Comment "Inc/app_board_a.h" "Sinh viên 2 (Tầng Tin Cậy) & Sinh viên 4 (Tầng Ứng dụng)" "Điều khiển máy phát (Board A). Thuật toán Cửa sổ trượt (Go-Back-N Sender), Timeout (SV2). Đọc DHT11, quét nút, cập nhật OLED (SV4)."

# 3. SV2 + SV4 (App Board B)
Prepend-Comment "Src/app_board_b.c" "Sinh viên 2 (Tầng Tin Cậy) & Sinh viên 4 (Tầng Ứng dụng)" "Điều khiển máy thu (Board B). Lọc gói trùng, xét Sequence Number, trả Cumulative ACK/NACK (SV2). Thống kê gói đúng/lỗi/trùng lên OLED (SV4)."
Prepend-Comment "Inc/app_board_b.h" "Sinh viên 2 (Tầng Tin Cậy) & Sinh viên 4 (Tầng Ứng dụng)" "Điều khiển máy thu (Board B). Lọc gói trùng, xét Sequence Number, trả Cumulative ACK/NACK (SV2). Thống kê gói đúng/lỗi/trùng lên OLED (SV4)."

# 4. SV3
Prepend-Comment "Src/error_injection.c" "Sinh viên 3 (Module Chèn Lỗi)" "Proxy/Tin tặc đánh chặn tín hiệu trước khi ra mạng. Tạo lỗi nhân tạo: Bỏ gói (Drop), Đảo bit (Corrupt CRC), Chèn rác (Garbage), Trễ ACK."
Prepend-Comment "Inc/error_injection.h" "Sinh viên 3 (Module Chèn Lỗi)" "Proxy/Tin tặc đánh chặn tín hiệu trước khi ra mạng. Tạo lỗi nhân tạo: Bỏ gói (Drop), Đảo bit (Corrupt CRC), Chèn rác (Garbage), Trễ ACK."

# 5. SV4
Prepend-Comment "Src/benchmark.c" "Sinh viên 4 (Đo đạc & Ứng dụng)" "Hệ thống ép tải. Chạy 104 gói liên tục để đo và so sánh Thông lượng (Throughput) thực tế giữa Stop-and-Wait và Go-Back-N."
Prepend-Comment "Inc/benchmark.h" "Sinh viên 4 (Đo đạc & Ứng dụng)" "Hệ thống ép tải. Chạy 104 gói liên tục để đo và so sánh Thông lượng (Throughput) thực tế giữa Stop-and-Wait và Go-Back-N."

Prepend-Comment "Src/Anglas_OLED_SSD1306.c" "Sinh viên 4 (Đo đạc & Ứng dụng)" "Thư viện giao tiếp màn hình OLED qua chuẩn I2C, dùng để hiển thị giao diện người dùng (HMI)."
Prepend-Comment "Inc/Anglas_OLED_SSD1306.h" "Sinh viên 4 (Đo đạc & Ứng dụng)" "Thư viện giao tiếp màn hình OLED qua chuẩn I2C, dùng để hiển thị giao diện người dùng (HMI)."

Prepend-Comment "Src/dht11.c" "Sinh viên 4 (Đo đạc & Ứng dụng)" "Thư viện đọc dữ liệu nhiệt độ và độ ẩm từ cảm biến DHT11 (chuẩn 1-Wire)."
Prepend-Comment "Inc/dht11.h" "Sinh viên 4 (Đo đạc & Ứng dụng)" "Thư viện đọc dữ liệu nhiệt độ và độ ẩm từ cảm biến DHT11 (chuẩn 1-Wire)."

# 6. Dùng chung
Prepend-Comment "Src/timer_driver.c" "Dùng chung (Core System)" "Cung cấp bộ đếm mili-giây (SysTick) độc lập, giúp tính toán Timeout và định thời gian mà không bị treo CPU."
Prepend-Comment "Inc/timer_driver.h" "Dùng chung (Core System)" "Cung cấp bộ đếm mili-giây (SysTick) độc lập, giúp tính toán Timeout và định thời gian mà không bị treo CPU."

Prepend-Comment "Src/delay.c" "Dùng chung (Core System)" "Hàm delay bằng vòng lặp phần mềm (for loop), thay thế HAL_Delay để chống treo ngắt xung đột."
Prepend-Comment "Inc/delay.h" "Dùng chung (Core System)" "Hàm delay bằng vòng lặp phần mềm (for loop), thay thế HAL_Delay để chống treo ngắt xung đột."

Prepend-Comment "Src/main.c" "Dùng chung (Core System)" "Điểm khởi đầu của chương trình. Cấu hình xung nhịp, kích hoạt phần cứng HAL, gọi hàm khởi tạo của Tầng Ứng dụng."
Prepend-Comment "Inc/main.h" "Dùng chung (Core System)" "Điểm khởi đầu của chương trình. Cấu hình xung nhịp, kích hoạt phần cứng HAL, gọi hàm khởi tạo của Tầng Ứng dụng."

