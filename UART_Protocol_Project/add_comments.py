import os

filepath = r'Core\Src\main.c'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

target = '''// ====================================================================
// KICH BAN 3: TEST SV2 (GO-BACK-N CUA SO TRUOT) - DANG MO DE CHAY
// -> (Neu muon tat: Them cap dau /* va */ de khoa toan bo khoi nay lai)
// ===================================================================='''

replacement = target + '''
/* 
 * ---------------- HUONG DAN TEST GO-BACK-N TREN HERCULES ----------------
 * CHUAN BI HEX O HERCULES:
 *   - DATA 0: 7E 00 00 01 88 89 7E (De test Mach B)
 *   - DATA 1: 7E 00 01 01 88 88 7E (De test Mach B)
 *   - DATA 2: 7E 00 02 01 88 8B 7E (De test Mach B)
 *   - ACK 3 : 7E 01 03 00 02 7E    (De test Mach A - Xac nhan 0, 1, 2, 3)
 *   - ACK 7 : 7E 01 07 00 06 7E    (De test Mach A - Xac nhan 4, 5, 6, 7)
 *
 * KICH BAN TEST [1] - Lui cua so (Timeout):
 *   -> Khong lam gi ca. De cho mach tu ban Seq 0 den 3.
 *   -> Doi 5s, Mach se bao TIMEOUT va tu dong ban lai y nguyen truyen 0 den 3.
 * 
 * KICH BAN TEST [2] - ACK Gop va Truot Cua So:
 *   -> Ngay khi mach ban xong 0, 1, 2, 3. Ban gui ngay ACK 3.
 *   -> Mach hieu la 4 goi da den noi an toan -> TRUOT CUA SO -> Ban tiep 4, 5, 6, 7.
 *   -> Ban gui ACK 7 de hoan thanh.
 *
 * KICH BAN TEST [3] - Mach B nhan duoc goi DUNG thu tu:
 *   -> Ban gui lan luot DATA 0, roi DATA 1.
 *   -> Mach B se lien tuc khen "Nhan DUNG THU TU... Lay ra xai" va doi lai ACK 0, ACK 1.
 *
 * KICH BAN TEST [4] - Mach B phat hien goi bi ROI / SAI thu tu:
 *   -> Ban gui DATA 0. Mach B khen dung, sau do no dang doi DATA 1.
 *   -> Ban co tinh gui nhay coc DATA 2 vao.
 *   -> Mach B lap tuc manng "SAI THU TU... Vut data" va doi lai ACK 0 (báo tui chi co moi goi 0).
 * ------------------------------------------------------------------------
 */'''

new_content = content.replace(target, replacement)

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(new_content)