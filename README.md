# Kumbung Sense

Aplikasi monitoring dan kontrol IoT untuk kumbung jamur berbasis Framework React

## Overview

Kumbung Sense adalah aplikasi web yang membantu petani jamur dalam memantau dan mengontrol kondisi lingkungan kumbung secara real-time. Dengan integrasi IoT, pengguna dapat melihat data sensor (suhu, kelembapan, intensitas cahaya, kelembapan media tanam) dan mengatur perangkat (kipas, humidifier, lampu) langsung dari dashboard.

## Features

- Autentikasi user (login/logout)
- Dashboard monitoring sensor secara real-time
- Kontrol perangkat (fan, humidifier, lampu)
- Riwayat data sensor dan aktuator
- Notifikasi
- Manajemen device
- Responsive UI

## Tech Stack

- [React](https://reactjs.org/)
- [Next.js](https://nextjs.org/)
- [TypeScript](https://www.typescriptlang.org/)
- [Firebase](https://firebase.google.com)
- [Tailwind CSS](https://tailwindcss.com/)
- [Plotly.js](https://plotly.com/javascript/) (visualisasi data)
- [lucide-react](https://)

## Getting Started

1. **Clone repository:**
   ```bash
   git clone https://github.com/username/kumbung-sense.git
   cd kumbung-sense
   ```
2. **Install dependencies:**
   ```bash
   npm install
   ```
3. **Jalankan aplikasi:**
   ```bash
   npm run dev
   ```
   Akses aplikasi di `http://localhost:3000`.

## Deployment

Untuk melakukan deployment aplikasi, ikuti langkah-langkah berikut:

1. **Build aplikasi untuk produksi:**
   ```bash
   npm run build
   ```
2. **Jalankan aplikasi yang sudah dibangun:**
   ```bash
   npm start
   ```

## Build your app

## How It Works

Kumbung Sense memanfaatkan teknologi berikut untuk memberikan fungsionalitasnya:

- **React** dan **Next.js** untuk antarmuka pengguna yang responsif dan dinamis.
- **TypeScript** untuk pengembangan yang lebih aman dan terstruktur.
- **Firebase Auth** untuk autentikasi pengguna yang aman.
- **Tailwind CSS** untuk desain antarmuka yang modern dan responsif.
- **Plotly.js** untuk visualisasi data sensor yang interaktif.

Aplikasi ini terintegrasi dengan perangkat IoT melalui protokol yang sesuai, memungkinkan komunikasi dua arah antara aplikasi dan perangkat keras. Data sensor dikumpulkan dan dikirim ke aplikasi secara real-time, memberikan wawasan langsung kepada pengguna tentang kondisi kumbung mereka. Pengguna juga dapat mengontrol perangkat seperti kipas, humidifier, dan lampu langsung dari aplikasi, memungkinkan penyesuaian cepat terhadap perubahan kondisi lingkungan.
