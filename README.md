# 🖱️ ESP32 BLE Mouse — Proof of Concept Manipulasi Aplikasi Presensi BPS

> **Proof of Concept** | Riset Keamanan | Pengungkapan Bertanggung Jawab

---

## ⚠️ Disclaimer

Proyek ini adalah **proof of concept untuk tujuan riset keamanan semata**. Proyek ini mendemonstrasikan celah keamanan pada aplikasi presensi Android milik Badan Pusat Statistik (BPS) yang hanya mengandalkan pemeriksaan keamanan di sisi perangkat. Tujuannya adalah untuk menyoroti kelemahan tersebut dan mendorong pengembang agar mengimplementasikan metode verifikasi yang lebih kuat dari sisi pengguna.

**Jangan gunakan ini untuk melakukan kecurangan, memalsukan catatan kehadiran, atau melanggar ketentuan layanan apapun.**

---

## 📖 Latar Belakang

Aplikasi **Presensi BPS** (Badan Pusat Statistik) mengimplementasikan beberapa langkah keamanan untuk mencegah kecurangan presensi, antara lain:

- 📍 Deteksi GPS palsu / mock location
- 🛠️ Deteksi developer options aktif
- 🔓 Deteksi root / ADB
- 📱 Pemeriksaan integritas perangkat

Semua pemeriksaan ini bersifat **device-level** — mereka memeriksa kondisi sistem Android. Namun, terdapat celah mendasar yang luput dari perhatian:

> **Aplikasi tidak dapat membedakan antara sentuhan jari manusia dengan input dari mouse Bluetooth (BLE HID).**

Proyek ini mengeksploitasi celah tersebut menggunakan **mikrokontroler ESP32** yang berperan sebagai mouse BLE HID, dikendalikan melalui antarmuka web, untuk mengotomasi ketukan dan gestur pada perangkat Android — tanpa memicu satupun pemeriksaan keamanan di atas.

---

## 🏗️ Cara Kerja

```
┌─────────────────┐       BLE HID Mouse (Bluetooth)      ┌──────────────────────┐
│   ESP32 Dev     │ ───────────────────────────────────▶  │  Perangkat Android   │
│   Module        │                                        │  (Aplikasi Presensi  │
│                 │                                        │       BPS)           │
│  - Web Server   │                                        │                      │
│  - BLE Mouse    │                                        │  ✅ Lolos semua      │
│  - Sequence     │                                        │     pemeriksaan      │
│    tersimpan    │                                        │     keamanan         │
└────────┬────────┘                                        │                      │
         │                                                 │  ❌ Tidak dapat      │
         │ HTTP (WiFi lokal)                               │     mendeteksi       │
         ▼                                                 │     input mouse BLE  │
┌─────────────────┐                                        └──────────────────────┘
│   Trigger UI    │
│  (Browser /     │
│   HP Android)   │
└─────────────────┘
```

### Alur Serangan

1. ESP32 terhubung ke WiFi dan menjalankan web server di IP statis
2. Pengguna membuka web UI dari browser manapun di jaringan yang sama
3. Urutan gerakan mouse yang telah direkam tersimpan di ESP32 (tidak hilang saat restart)
4. Saat dipicu (tombol web / tombol BOOT / perintah Serial), ESP32 memutar ulang urutan tersebut via BLE
5. Android menerima event HID mouse standar — tidak dapat dibedakan dari mouse sungguhan

---

## 🔬 Pemeriksaan Keamanan yang Berhasil Dilewati

| Pemeriksaan Keamanan | Berhasil Dilewati? | Alasan |
|---|---|---|
| Deteksi GPS Palsu | ✅ Ya | GPS asli digunakan, lokasi tidak dipalsukan |
| Deteksi Developer Options | ✅ Ya | Developer options bisa dinonaktifkan |
| Deteksi ADB | ✅ Ya | ADB tidak digunakan sama sekali |
| Deteksi Root | ✅ Ya | Root tidak diperlukan |
| Pemeriksaan Integritas Perangkat | ✅ Ya | Perangkat lulus integritas, tidak ada yang dimodifikasi |
| **Deteksi Input BLE Mouse** | **✅ Ya** | **Android tidak memiliki API untuk membedakan mouse vs. jari** |

---

## 🛠️ Perangkat Keras yang Dibutuhkan

- ESP32 Dev Module (varian apapun yang mendukung BLE)
- Kabel USB (untuk flashing dan konfigurasi Serial)
- Perangkat Android dengan Bluetooth

**Total biaya: ~Rp 50.000 – 75.000**

---

## 📦 Dependensi Software

Install melalui Arduino Library Manager:

| Library | Author | Fungsi |
|---|---|---|
| `ESP32-BLE-Mouse` | T-vK | Emulasi mouse HID via BLE |

Sudah tersedia di ESP32 Arduino core (tidak perlu install):

- `WebServer` — menyajikan UI pemicu
- `Preferences` — menyimpan sequence ke flash
- `WiFi` / `WiFiClient` — konektivitas jaringan

**Pengaturan Board:**
- Board: `ESP32 Dev Module`
- Partition Scheme: `Huge APP` ← wajib untuk BLE

---

## 🚀 Cara Setup

### 1. Flash Firmware

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Tambahkan dukungan board ESP32 via Boards Manager
3. Install library `ESP32-BLE-Mouse`
4. Set partition scheme ke **Huge APP**
5. Flash `bot_absen.ino`

### 2. Konfigurasi WiFi (via Serial)

Buka Serial Monitor di **115200 baud**, line ending = **Newline**:

```
SET SSID NamaWiFiAnda
SET PASS PasswordWiFi
SET IP 192.168.1.200
SET GATEWAY 192.168.1.1
WIFISAVE
WIFICONNECT
```

### 3. Pasangkan BLE Mouse

Di perangkat Android target:
- Pengaturan → Bluetooth → Cari perangkat
- Pasangkan **"ESP32 BLE Mouse"**

### 4. Buka Web UI

Buka `http://192.168.1.200` dari browser manapun di jaringan yang sama.

---

## ⌨️ Referensi Perintah Serial

Buka Serial Monitor di **115200 baud**, line ending = **Newline**. Ketik `HELP` untuk melihat semua perintah.

### Perintah Sequence

| Perintah | Keterangan |
|---|---|
| `LIST` | Tampilkan sequence saat ini |
| `CLEAR` | Hapus semua langkah |
| `ADD MOVE dx dy` | Gerakan relatif, nilai negatif didukung. Contoh: `ADD MOVE -100 50` |
| `ADD CLICK` | Klik kiri |
| `ADD RCLICK` | Klik kanan |
| `ADD DCLICK` | Klik ganda |
| `ADD PRESS` | Tahan tombol kiri (mulai drag) |
| `ADD RELEASE` | Lepas tombol kiri (akhir drag) |
| `ADD DELAY ms` | Jeda dalam milidetik |
| `SAVE` | Simpan sequence ke flash (tahan reboot) |
| `RUN` | Jalankan sequence sekarang |

### Eksekusi Langsung (tidak tersimpan, untuk debugging)

| Perintah | Keterangan |
|---|---|
| `DO MOVE dx dy` | Gerakkan kursor sekarang |
| `DO CLICK` | Klik sekarang |
| `DO DCLICK` | Klik ganda sekarang |
| `DO PRESS` | Tahan tombol sekarang |
| `DO RELEASE` | Lepas tombol sekarang |
| `DO DELAY ms` | Jeda sekarang |

### Perintah WiFi

| Perintah | Keterangan |
|---|---|
| `WIFISHOW` | Tampilkan konfigurasi WiFi saat ini |
| `SET SSID <nama>` | Ganti SSID |
| `SET PASS <password>` | Ganti password |
| `SET IP <x.x.x.x>` | Ganti IP statis |
| `SET GATEWAY <x.x.x.x>` | Ganti gateway |
| `WIFISAVE` | Simpan konfigurasi ke flash |
| `WIFICONNECT` | Reconnect dengan pengaturan baru |

---

## 📋 Upload Sequence via Python

Daripada mengetik satu per satu, gunakan script ini dari PC:

```python
import serial, time

PORT = "COM3"   # sesuaikan dengan port di Arduino IDE → Tools → Port
BAUD = 115200

commands = [
    "CLEAR",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 1500",
    "ADD MOVE -1000 -1000",
    "ADD DELAY 500",
    "ADD MOVE 250 800",
    "ADD DELAY 500",
    "ADD PRESS",
    "ADD DELAY 200",
    "ADD MOVE 0 -100",
    "ADD DELAY 50",
    "ADD MOVE 0 -100",
    "ADD DELAY 50",
    "ADD MOVE 0 -100",
    "ADD DELAY 50",
    "ADD MOVE 0 -100",
    "ADD RELEASE",
    "SAVE",
    "LIST",
]

s = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)
for cmd in commands:
    s.write((cmd + "\n").encode())
    time.sleep(0.3)
    print(f"Terkirim: {cmd}")
    resp = s.read_all().decode(errors="ignore").strip()
    if resp: print(f"  >> {resp}")
s.close()
print("Selesai.")
```

```bash
pip install pyserial
python upload_sequence.py
```

> ⚠️ Tutup Arduino Serial Monitor sebelum menjalankan script — hanya satu program yang bisa menggunakan port COM secara bersamaan.

---

## 🔴 Metode Pemicu

Sequence dapat dipicu dengan **3 cara**:

| Metode | Cara |
|---|---|
| 🌐 Web UI | Buka `http://192.168.1.200` → tekan **▶ Jalankan Sequence** |
| ⌨️ Serial | Ketik `RUN` di Serial Monitor |
| 🔘 Tombol BOOT | Tekan tombol BOOT fisik di board ESP32 |

---

## 🛡️ Rekomendasi Mitigasi untuk Pengembang Aplikasi

PoC ini mengungkap celah keamanan yang nyata dan banyak diabaikan. Berikut rekomendasi untuk menutup celah tersebut:

### 1. 🤳 Verifikasi Biometrik
Wajibkan pengenalan wajah atau autentikasi sidik jari **pada saat melakukan presensi**. Hal ini tidak dapat diotomasi via input mouse.

```
Alur saat ini:  Buka App → Cek GPS → Tap Presensi ✅ (rentan)
Alur aman:      Buka App → Cek GPS → Scan Wajah → Tap Presensi ✅
```

### 2. 🎲 Tantangan Acak / CAPTCHA
Tampilkan tantangan yang diacak dan membutuhkan respons kognitif manusia yang asli:
- Tap **lingkaran merah** di antara 5 bentuk acak
- Masukkan **kode 4 digit** yang ditampilkan di layar
- Selesaikan operasi matematika sederhana

Sequence mouse yang telah direkam tidak dapat beradaptasi dengan tantangan acak.

### 3. 📡 Deteksi Sumber Input
API `MotionEvent` Android mengekspos sumber input melalui `event.getSource()`. Aplikasi dapat memeriksa:

```java
if (event.getSource() == InputDevice.SOURCE_MOUSE) {
    // Tolak — ini adalah mouse, bukan jari
    return;
}
```

Ini secara langsung mendeteksi dan memblokir input mouse BLE.

### 4. ⏱️ Biometrik Perilaku
Analisis pola sentuhan: tekanan, ukuran area sentuh, kecepatan gesekan, dan getaran mikro. Input mouse menghasilkan gerakan yang terlalu sempurna, lurus, dan tepat piksel — secara statistik dapat dibedakan dari sentuhan manusia.

### 5. 🔐 Verifikasi Sisi Server
Jangan pernah mempercayai klien sepenuhnya. Implementasikan pemeriksaan sisi server:
- Validasi waktu presensi terhadap jadwal kerja
- Tandai pola presensi yang tidak masuk akal secara statistik (waktu yang sama setiap hari hingga detik)
- Korelasikan dengan lokasi jaringan/WiFi selain GPS

---

## 📊 Ringkasan Serangan vs. Pertahanan

```
CELAH YANG DIEKSPLOITASI              MITIGASI YANG DIREKOMENDASIKAN
────────────────────────────────────────────────────────────────────────
Input Mouse BLE          ──────▶  Cek InputDevice.SOURCE_MOUSE
Sequence Terekam         ──────▶  Tantangan acak / CAPTCHA
Tanpa kehadiran fisik    ──────▶  Verifikasi biometrik (wajah / sidik jari)
Pola waktu sempurna      ──────▶  Biometrik perilaku
Eksekusi otomatis        ──────▶  Deteksi anomali sisi server
```

---

## 📁 Struktur Proyek

```
bot_absen/
├── bot_absen.ino            # Sketch utama ESP32
├── upload_sequence.py       # Script Python untuk upload sequence massal
└── README.md                # Dokumentasi ini
```

---

## 🔗 Referensi

- [Android InputDevice API](https://developer.android.com/reference/android/view/InputDevice)
- [Android MotionEvent.getSource()](https://developer.android.com/reference/android/view/MotionEvent#getSource())
- [ESP32-BLE-Mouse Library](https://github.com/T-vK/ESP32-BLE-Mouse)
- [Play Integrity API](https://developer.android.com/google/play/integrity)
- [OWASP Mobile Security Testing Guide](https://owasp.org/www-project-mobile-security-testing-guide/)

---

## 📜 Lisensi

MIT License — lihat [LICENSE](LICENSE) untuk detail.

Proyek ini dibagikan untuk **tujuan edukasi dan riset keamanan**. Penulis tidak mendukung penyalahgunaan.

---

*Dibuat dengan ESP32 seharga Rp 50.000, banyak Serial.println(), dan kesadaran bahwa mikrokontroler murah dapat melewati sistem keamanan presensi instansi pemerintah.*
