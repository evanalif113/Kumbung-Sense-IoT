'use strict';
var firebase = require("firebase-admin");

// Ganti "./kunci_firebase.json" dengan path yang benar ke file kunci layanan Anda
var serviceAccount = require("./kunci_firebase.json");

firebase.initializeApp({
  credential: firebase.credential.cert(serviceAccount),
  databaseURL: "https://kumbung-sense-default-rtdb.asia-southeast1.firebasedatabase.app"
});

var db = firebase.database();
var userStates = {}; // Objek untuk menyimpan state setiap pengguna (thresholds, aktuator, mode)

// Definisikan pemetaan antara sensor dan pin aktuatornya
var SENSOR_ACTUATOR_MAP = {
    temperature: '16', // Suhu dikontrol oleh Kipas di pin 16
    humidity: '17',    // Kelembapan dikontrol oleh Humidifier di pin 17
    light: '18',       // Cahaya dikontrol oleh Lampu di pin 18
    moisture: '19'     // Kelembapan media dikontrol oleh Pompa di pin 19
};

// Fungsi untuk mengatur listener untuk setiap pengguna
function setupListenersForUser(uid) {
    console.log('Mengatur listener untuk UID: ' + uid);

    // Inisialisasi state untuk pengguna ini
    userStates[uid] = {
        currentThresholds: null,
        currentActuatorStates: null,
        isAutoMode: false
    };

    // Listener untuk Mode Auto
    var modeRef = db.ref(uid + '/mode/auto');
    modeRef.on('value', function(snapshot) {
        userStates[uid].isAutoMode = snapshot.val();
        console.log('[' + uid + '] --- MODE KONTROL DIUBAH ---');
        console.log('[' + uid + '] Mode Auto sekarang: ' + (userStates[uid].isAutoMode ? 'AKTIF' : 'NON-AKTIF'));
        console.log('[' + uid + '] --------------------------\n');
    }, function(error) { console.error('[' + uid + '] Gagal membaca mode:', error); });

    // Listener untuk Thresholds
    var thresholdDataRef = db.ref(uid + '/thresholds');
    thresholdDataRef.on('value', function(snapshot) {
        if (snapshot.exists()) {
            userStates[uid].currentThresholds = snapshot.val();
            console.log('[' + uid + '] --- Data THRESHOLD Diperbarui ---');
            console.log('[' + uid + '] Threshold Saat Ini:', userStates[uid].currentThresholds);
            console.log('[' + uid + '] -------------------------------\n');
        } else {
            console.log('[' + uid + '] Tidak ada data threshold.');
        }
    }, function(error) { console.error('[' + uid + '] Gagal membaca data threshold:', error); });

    // Listener untuk Aktuator
    var aktuatorDataRef = db.ref(uid + '/aktuator/data');
    aktuatorDataRef.on('value', function(snapshot) {
        if (snapshot.exists()) {
            userStates[uid].currentActuatorStates = snapshot.val();
            console.log('[' + uid + '] --- Status AKTUATOR Diperbarui ---');
            console.log('[' + uid + '] Status Aktuator Saat Ini:', userStates[uid].currentActuatorStates);
            console.log('[' + uid + '] --------------------------------\n');
        } else {
            console.log('[' + uid + '] Tidak ada data aktuator.');
        }
    }, function(error) { console.error('[' + uid + '] Gagal membaca data aktuator:', error); });

    // Listener untuk Sensor (Pemicu & Logika Keputusan)
    var sensorDataRef = db.ref(uid + '/sensor/data');
    var latestSensorQuery = sensorDataRef.orderByKey().limitToLast(1);

    latestSensorQuery.on('child_added', function(snapshot) {
        if (!snapshot.exists()) {
            return;
        }

        var sensorData = snapshot.val();
        console.log('[' + uid + '] --- Data SENSOR Baru Diterima ---', sensorData);

        var userState = userStates[uid];

        if (!userState.isAutoMode) {
            console.log('[' + uid + '] LOGIKA DITUNDA: Mode Auto tidak aktif.\n');
            return;
        }
        if (!userState.currentThresholds || !userState.currentActuatorStates) {
            console.log('[' + uid + '] LOGIKA DITUNDA: Data threshold atau aktuator belum siap.\n');
            return;
        }

        var updates = {};

        // --- Logika Suhu -> Kipas ---
        var fanPin = SENSOR_ACTUATOR_MAP.temperature;
        if (sensorData.temperature > userState.currentThresholds.temperatureMax) {
            // Jika suhu terlalu panas, dan kipas masih mati, nyalakan kipas
            if (userState.currentActuatorStates[fanPin] === 0) updates[fanPin] = 1;
        } else if (sensorData.temperature < userState.currentThresholds.temperatureMin) {
            // Jika suhu sudah cukup dingin, dan kipas masih nyala, matikan kipas
            if (userState.currentActuatorStates[fanPin] === 1) updates[fanPin] = 0;
        }

        // --- Logika Kelembapan Udara -> Humidifier ---
        var humidifierPin = SENSOR_ACTUATOR_MAP.humidity;
        if (sensorData.humidity < userState.currentThresholds.humidityMin) {
            // Jika terlalu kering, nyalakan humidifier
            if (userState.currentActuatorStates[humidifierPin] === 0) updates[humidifierPin] = 1;
        } else if (sensorData.humidity > userState.currentThresholds.humidityMax) {
            // Jika sudah cukup lembap, matikan humidifier
            if (userState.currentActuatorStates[humidifierPin] === 1) updates[humidifierPin] = 0;
        }
        
        // --- Logika Cahaya -> Lampu ---
        var lightPin = SENSOR_ACTUATOR_MAP.light;
        if (sensorData.light < userState.currentThresholds.lightMin) {
            // Jika terlalu gelap, nyalakan lampu
            if (userState.currentActuatorStates[lightPin] === 0) updates[lightPin] = 1;
        } else if (sensorData.light > userState.currentThresholds.lightMax) {
            // Jika sudah cukup terang, matikan lampu
            if (userState.currentActuatorStates[lightPin] === 1) updates[lightPin] = 0;
        }

        // --- Logika Kelembapan Media -> Pompa ---
        var pumpPin = SENSOR_ACTUATOR_MAP.moisture;
        if (sensorData.moisture < userState.currentThresholds.moistureMin) {
            // Jika media tanam kering, nyalakan pompa
            if (userState.currentActuatorStates[pumpPin] === 0) updates[pumpPin] = 1;
        } else if (sensorData.moisture > userState.currentThresholds.moistureMax) {
            // Jika media sudah basah, matikan pompa
            if (userState.currentActuatorStates[pumpPin] === 1) updates[pumpPin] = 0;
        }

        // --- Aksi: Kirim update ke Firebase jika ada perubahan ---
        if (Object.keys(updates).length > 0) {
            console.log('[' + uid + '] AKSI: Terdeteksi kondisi di luar threshold, mengirim pembaruan:', updates);
            aktuatorDataRef.update(updates)
                .then(function() { console.log('[' + uid + '] Update aktuator berhasil dikirim.'); })
                .catch(function(error) { console.error('[' + uid + '] Gagal mengirim update aktuator:', error); });
        } else {
            console.log('[' + uid + '] AKSI: Kondisi normal, tidak ada perubahan pada aktuator.');
        }
        console.log('[' + uid + '] ----------------------------------\n');
    });
}

// Listener utama untuk mendeteksi semua pengguna (UID) di root database
var usersRef = db.ref('/');
usersRef.on('child_added', function(snapshot) {
    var uid = snapshot.key;
    // Pastikan itu adalah UID pengguna yang valid (misalnya, dengan memeriksa keberadaan node tertentu)
    if (snapshot.hasChild('sensor') && snapshot.hasChild('aktuator')) {
        console.log('Pengguna terdeteksi: ' + uid);
        setupListenersForUser(uid);
    } else {
        console.log('Node terdeteksi "' + uid + '" dilewati (bukan struktur pengguna yang valid).');
    }
});

console.log("Script kontrol multi-user berjalan. Mendengarkan pengguna baru dari Firebase...");