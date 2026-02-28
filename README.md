# 🖱️ ESP32 BLE Mouse — Proof of Concept Manipulasi Aplikasi Presensi BPS

---

## ⚠️ Disclaimer

Proyek ini merupakan **proof of concept untuk demonstrasi kelemahan aplikasi**. Proyek ini mendemonstrasikan kelemahan pada aplikasi presensi Android Badan Pusat Statistik (BPS) yang hanya mengandalkan pemeriksaan keamanan di sisi perangkat. Tujuannya adalah untuk menyoroti kelemahan tersebut dan mendorong agar diimplementasikan metode verifikasi yang lebih kuat dari sisi pengguna.

---

## 📖 Latar Belakang

Aplikasi **Presensi BPS** telah mengimplementasikan beberapa langkah keamanan untuk mencegah kecurangan presensi, antara lain:

Dari sisi perangkat
- 📍 Deteksi Fake GPS / mock location
- 🛠️ Deteksi developer options aktif untuk mencegah otomatisasi dengan ADB
- 🔓 Deteksi root / ADB
- 📱 Rolling enkripsi untuk menghindari man-in-the-middle attack

Semua kemanan ini masih bersifat **device-level** — aplikasi memeriksa kondisi sistem Android. Namun, terdapat kekurangan mendasar yang luput dari perhatian:

> **Aplikasi tidak dapat membedakan antara sentuhan jari manusia dengan input dari mouse Bluetooth (BLE HID).**

Project ini mengeksploitasi celah tersebut menggunakan **ESP32** yang berperan sebagai mouse BLE HID, dikendalikan melalui antarmuka web (yang kemudian dapat dikembangkan unutk kendali via internet atau dengan trigger waktu jam tertentu), untuk mengotomasi touch pada perangkat Android.

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
         │ HTTP (WiFi)                                     │     mendeteksi       │
         ▼                                                 │     input mouse BLE  │
┌─────────────────┐                                        └──────────────────────┘
│   Trigger UI    │
│  (Browser /     │
│   HP Android)   │
└─────────────────┘
```

### Alur Kerja

1. ESP32 terhubung ke WiFi dan menjalankan web server di IP statis
2. Pengguna membuka web UI dari browser
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

## 🛠️ Perangkat yang Dibutuhkan

- ESP32 Dev Module (varian apapun yang mendukung BLE)
- Kabel USB (untuk flashing dan konfigurasi Serial)
- Perangkat Android dengan Bluetooth

**Total biaya: ~Rp 40.000 – 60.000**

---

## 📦 Dependensi Software

| Library | Author | Fungsi |
|---|---|---|
| `ESP32-BLE-Mouse` | T-vK | Emulasi mouse HID via BLE |

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

Buka Koneksi Serial (via USB) atur baud rate ke **115200** dan jalankan:

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

Setelah menentukan urutan dimana posisi layar yang akan dipencet, upload sequence menggunakan script ini :

```python
import serial, time

PORT = "COM4"   # sesuaikan dengan port di Arduino IDE → Tools → Port
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

## 🔴 Trigger
Sequence dapat dipicu dengan **3 cara**:

| Metode | Cara |
|---|---|
| 🌐 Web UI | Buka `http://192.168.1.200` → tekan **▶ Jalankan Sequence** |
| ⌨️ Serial | Ketik `RUN` di Serial Monitor |
| 🔘 Tombol BOOT | Tekan tombol BOOT fisik di board ESP32 |

---

## 🛡️ Rekomendasi Mitigasi

### 1. 🤳 Verifikasi Biometrik
Mewajibkan pengenalan wajah atau autentikasi sidik jari **pada saat melakukan presensi**. Hal ini tidak dapat diotomasi via input mouse.

```
Alur saat ini:  Buka App → Cek GPS → Tap Presensi ✅ (rentan)
Alur aman:      Buka App → Cek GPS → Scan Wajah → Tap Presensi ✅
```

### 2. 🎲 Random Challenge / CAPTCHA
Tampilkan tantangan yang diacak dan membutuhkan respons kognitif manusia yang asli:
- Tap **lingkaran merah** di antara 5 bentuk acak
- Masukkan **kode 4 digit** yang ditampilkan di layar
- Selesaikan operasi matematika sederhana

Sequence mouse yang telah direkam tidak dapat beradaptasi dengan random challenge.

### 3. 📡 Deteksi Input
API `MotionEvent` Android mengekspos input melalui `event.getSource()`. Aplikasi dapat memeriksa:

```java
if (event.getSource() == InputDevice.SOURCE_MOUSE) {
    // Tolak — ini adalah mouse, bukan jari
    return;
}
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
