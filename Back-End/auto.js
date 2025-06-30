var firebase = require("firebase-admin");

// Ganti "./kunci_firebase.json" dengan path yang benar ke file kunci layanan Anda
var serviceAccount = require("./kunci_firebase.json");

firebase.initializeApp({
  credential: firebase.credential.cert(serviceAccount),
  databaseURL: "https://kumbung-sense-default-rtdb.asia-southeast1.firebasedatabase.app"
});

const db = firebase.database();
const UID = 'JU3dPIvArGbuoRUywdcTmdVkQRr2';

// ======================================================================
// ALGORITMA KONTROL OTOMATIS
// ======================================================================

// 1. Variabel untuk menyimpan state dari Firebase secara real-time
let currentThresholds = null;
let currentActuatorStates = null;
let isAutoMode = false; // Default ke mode manual

// Definisikan pemetaan antara sensor dan pin aktuatornya
const SENSOR_ACTUATOR_MAP = {
    temperature: '16', // Suhu dikontrol oleh Kipas di pin 16
    humidity: '17',    // Kelembapan dikontrol oleh Humidifier di pin 17
    light: '18',       // Cahaya dikontrol oleh Lampu di pin 18
    moisture: '19'     // Kelembapan media dikontrol oleh Pompa di pin 19
};


// Listener untuk Mode Auto
const modeRef = db.ref(`${UID}/mode/auto`);
modeRef.on('value', (snapshot) => {
    isAutoMode = snapshot.val();
    console.log(`--- MODE KONTROL DIUBAH ---`);
    console.log(`Mode Auto sekarang: ${isAutoMode ? 'AKTIF' : 'NON-AKTIF'}`);
    console.log(`--------------------------\n`);
}, (error) => console.error("Gagal membaca mode:", error));


// Listener untuk Thresholds (menyimpan data ke variabel)
const thresholdDataRef = db.ref(`${UID}/thresholds`);
thresholdDataRef.on('value', (snapshot) => {
  if (snapshot.exists()) {
    currentThresholds = snapshot.val();
    console.log("--- Data THRESHOLD Diperbarui ---");
    console.log("Threshold Saat Ini:", currentThresholds);
    console.log("-------------------------------\n");
  } else {
    console.log("Tidak ada data threshold.");
  }
}, (error) => console.error("Gagal membaca data threshold:", error));


// Listener untuk Aktuator (menyimpan data ke variabel)
const aktuatorDataRef = db.ref(`${UID}/aktuator/data`);
aktuatorDataRef.on('value', (snapshot) => {
  if (snapshot.exists()) {
    currentActuatorStates = snapshot.val();
    console.log("--- Status AKTUATOR Diperbarui ---");
    console.log("Status Aktuator Saat Ini:", currentActuatorStates);
    console.log("--------------------------------\n");
  } else {
    console.log("Tidak ada data aktuator.");
  }
}, (error) => console.error("Gagal membaca data aktuator:", error));


// Listener untuk Sensor (Pemicu & Logika Keputusan)
const sensorDataRef = db.ref(`${UID}/sensor/data`);
const latestSensorQuery = sensorDataRef.orderByKey().limitToLast(1);

latestSensorQuery.on('child_added', (snapshot) => {
  // Gunakan 'child_added' agar hanya terpicu saat ada data BARU
  if (!snapshot.exists()) return;

  const sensorData = snapshot.val();
  console.log("--- Data SENSOR Baru Diterima ---", sensorData);

  // Cek kondisi sebelum menjalankan logika
  if (!isAutoMode) {
    console.log("LOGIKA DITUNDA: Mode Auto tidak aktif.\n");
    return;
  }
  if (!currentThresholds || !currentActuatorStates) {
    console.log("LOGIKA DITUNDA: Data threshold atau aktuator belum siap.\n");
    return;
  }

  // Objek untuk menampung perubahan yang perlu dikirim ke Firebase
  const updates = {};

  // --- Logika Suhu -> Kipas ---
  const fanPin = SENSOR_ACTUATOR_MAP.temperature;
  if (sensorData.temperature > currentThresholds.temperatureMax) {
    // Jika suhu terlalu panas, dan kipas masih mati, nyalakan kipas
    if (currentActuatorStates[fanPin] === 0) updates[fanPin] = 1;
  } else if (sensorData.temperature < currentThresholds.temperatureMin) {
    // Jika suhu sudah cukup dingin, dan kipas masih nyala, matikan kipas
    if (currentActuatorStates[fanPin] === 1) updates[fanPin] = 0;
  }

  // --- Logika Kelembapan Udara -> Humidifier ---
  const humidifierPin = SENSOR_ACTUATOR_MAP.humidity;
  if (sensorData.humidity < currentThresholds.humidityMin) {
    // Jika terlalu kering, nyalakan humidifier
    if (currentActuatorStates[humidifierPin] === 0) updates[humidifierPin] = 1;
  } else if (sensorData.humidity > currentThresholds.humidityMax) {
    // Jika sudah cukup lembap, matikan humidifier
    if (currentActuatorStates[humidifierPin] === 1) updates[humidifierPin] = 0;
  }
  
  // --- Logika Cahaya -> Lampu ---
  const lightPin = SENSOR_ACTUATOR_MAP.light;
  if (sensorData.light < currentThresholds.lightMin) {
    // Jika terlalu gelap, nyalakan lampu
    if (currentActuatorStates[lightPin] === 0) updates[lightPin] = 1;
  } else if (sensorData.light > currentThresholds.lightMax) {
    // Jika sudah cukup terang, matikan lampu
    if (currentActuatorStates[lightPin] === 1) updates[lightPin] = 0;
  }

  // --- Logika Kelembapan Media -> Pompa ---
  const pumpPin = SENSOR_ACTUATOR_MAP.moisture;
  if (sensorData.moisture < currentThresholds.moistureMin) {
    // Jika media tanam kering, nyalakan pompa
    if (currentActuatorStates[pumpPin] === 0) updates[pumpPin] = 1;
  } else if (sensorData.moisture > currentThresholds.moistureMax) {
    // Jika media sudah basah, matikan pompa
    if (currentActuatorStates[pumpPin] === 1) updates[pumpPin] = 0;
  }


  // --- Aksi: Kirim update ke Firebase jika ada perubahan ---
  if (Object.keys(updates).length > 0) {
    console.log("AKSI: Terdeteksi kondisi di luar threshold, mengirim pembaruan:", updates);
    aktuatorDataRef.update(updates)
      .then(() => console.log("Update aktuator berhasil dikirim."))
      .catch((error) => console.error("Gagal mengirim update aktuator:", error));
  } else {
    console.log("AKSI: Kondisi normal, tidak ada perubahan pada aktuator.");
  }
  console.log("----------------------------------\n");
});

console.log("Script kontrol berjalan. Mendengarkan perubahan data dari Firebase...");