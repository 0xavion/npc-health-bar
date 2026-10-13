# NPC Health Bar (GTA:SA Android)

Plugin untuk **Android Mod Loader (AML)** pada game GTA San Andreas Android yang menampilkan health bar (dan armour bar) di atas kepala NPC saat dipukul atau diserang oleh pemain.

## Fitur

- **Trigger Saat Diserang**: Health bar otomatis muncul selama **15 detik** setelah NPC menerima damage dari player (pukulan, senjata melee, tembakan, atau ditabrak kendaraan).
- **Armour Bar Adaptif**: Menampilkan bar armour di atas health bar jika NPC memiliki armour (misal: polisi/SWAT).
- **Line-of-Sight Check**: Health bar tidak tembus pandang; otomatis tersembunyi jika NPC terhalang dinding, gedung, atau objek lingkungan.
- **Gradasi Warna Dinamis**: Warna health bar bertransisi halus sesuai sisa HP (Hijau saat penuh, Kuning saat sedang, Merah saat sekarat).
- **Efek Transisi & Kematian**: Fade-out halus di detik terakhir, serta tetap terlihat selama 2 detik saat NPC mati untuk konfirmasi kill sebelum menghilang.
- **Zero GC & Memory Safe**: Menggunakan tabel tracking statis O(1) berbasis slot pool ped tanpa alokasi heap dinamis (`malloc`/`new`), bebas memory leak, dan sangat ringan tanpa drop FPS.
- **Multi-Arsitektur**: Mendukung **GTA:SA v2.00** (`armeabi-v7a`) dan **GTA:SA v2.10** (`arm64-v8a`).

## Struktur Project

```text
npc_health_bar/
├── Android.mk                  # Konfigurasi build NDK
├── Application.mk              # Konfigurasi ABI (arm & arm64) & STL
├── build.ps1                   # Script build PowerShell
├── NDKPath.txt                 # Path lokal ke Android NDK
├── main.cpp                    # Mod entry point (AML registration)
├── src/
│   ├── HealthBar.h             # Header interface HealthBar
│   └── HealthBar.cpp           # Logika deteksi hit, tracking, LOS, & rendering
├── mod/                        # AML core library (amlmod.h, logger)
└── plugin-sdk/                 # GTA:SA Plugin-SDK Android (aml-psdk)
```

## Persyaratan Build

- **Android NDK**: r21 atau yang lebih baru (misal: NDK 27+)
- **C++ Standard**: C++17
- **PowerShell**: Windows PowerShell atau PowerShell 7+

## Cara Build

1. Pastikan path Android NDK sudah terisi di file `NDKPath.txt`, contoh:
   ```text
   D:\Android\Sdk\ndk\27.1.12297006
   ```

2. Jalankan script build melalui PowerShell:
   ```powershell
   ./build.ps1
   ```

3. File library hasil compile (`libnpchealthbar.so`) akan berada di:
   - `libs/armeabi-v7a/libnpchealthbar.so` (untuk GTA:SA v2.00 32-bit)
   - `libs/arm64-v8a/libnpchealthbar.so` (untuk GTA:SA v2.10 64-bit)

## Cara Pemasangan di Android

1. Install **Android Mod Loader (AML)** pada game GTA:SA Android Anda.
2. Salin file `libnpchealthbar.so` (sesuai versi/arsitektur APK GTA:SA Anda) ke folder mod AML di perangkat, umumnya di:
   ```text
   /Android/data/com.rockstargames.gtasa/
   ```
   atau ke folder `mods/` AML sesuai konfigurasi AML Anda.

## Lisensi & Atribusi

- **NPC Health Bar Plugin**: Licensed under the [MIT License](LICENSE) (Copyright © 2026 Xavion)
- **AML Plugin SDK (aml-psdk)**: Created by [RusJJ](https://github.com/AndroidModLoader/aml-psdk) (GPLv3)
- **Android Mod Loader (AML)**: Created by [RusJJ](https://github.com/AndroidModLoader/AndroidModLoader)
