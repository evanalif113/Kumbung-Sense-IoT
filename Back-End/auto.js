var firebase = require("firebase-admin");

// Ganti "./kunci_firebase.json" dengan path yang benar ke file kunci layanan Anda
var serviceAccount = require("./kunci_firebase.json");

firebase.initializeApp({
  credential: firebase.credential.cert(serviceAccount),
  databaseURL: "https://kumbung-sense-default-rtdb.asia-southeast1.firebasedatabase.app"
});

// Dapatkan referensi ke database
const db = firebase.database();
const UID = 'JU3dPIvArGbuoRUywdcTmdVkQRr2'; // Simpan ID perangkat agar mudah diubah

// ======================================================================
// LISTENER 1: DATA SENSOR TERBARU
// ======================================================================

// Tentukan path ke node 'data' yang berisi semua log sensor
const sensorDataRef = db.ref(`${UID}/sensor/data`);

// Buat query untuk mendapatkan 1 data terakhir berdasarkan key (timestamp)
const latestSensorQuery = sensorDataRef.orderByKey().limitToLast(1);

latestSensorQuery.on('value', (snapshot) => {
  if (snapshot.exists()) {
    console.log("--- Data SENSOR Terbaru Diterima ---");
    
    // Karena kita hanya mengambil 1 data, kita bisa loop untuk mendapatkannya
    snapshot.forEach((childSnapshot) => {
      const timestamp = childSnapshot.key; // Key-nya adalah timestamp
      const data = childSnapshot.val();   // Val()-nya adalah objek data sensor
      
      // Tampilkan data ke konsol
      console.log(`Timestamp: ${timestamp}`);
      // Pastikan properti ada sebelum mengaksesnya untuk menghindari error
      console.log(`  Temperature: ${data.temperature || 'N/A'}°C`);
      console.log(`  Humidity: ${data.humidity || 'N/A'}%`);
      console.log(`  Moisture: ${data.moisture || 'N/A'}%`);
      console.log(`  Light: ${data.light || 'N/A'}`);
    });
    console.log("------------------------------------\n");

  } else {
    console.log("Tidak ada data sensor yang ditemukan.");
  }
}, (errorObject) => {
  console.error("Gagal membaca data sensor:", errorObject);
});


// ======================================================================
// LISTENER 2: DATA AKTUATOR
// ======================================================================

// Tentukan path ke node 'data' dari aktuator
const aktuatorDataRef = db.ref(`${UID}/aktuator/data`);

aktuatorDataRef.on('value', (snapshot) => {
  if (snapshot.exists()) {
    console.log("--- Data AKTUATOR Berubah ---");
    const data = snapshot.val(); // Dapatkan seluruh objek data aktuator
    
    // Tampilkan seluruh objek status aktuator
    console.log("Status Aktuator Saat Ini:", data);
    
    // Atau jika ingin menampilkannya satu per satu
    // for (const key in data) {
    //   console.log(`  Aktuator ${key}: Status ${data[key]}`);
    // }

    console.log("-----------------------------\n");
  } else {
    console.log("Tidak ada data aktuator yang ditemukan.");
  }
}, (errorObject) => {
  console.error("Gagal membaca data aktuator:", errorObject);
});

// ======================================================================
// LISTENER 3: DATA THRESHOLD
// ======================================================================

const thresholdDataRef = db.ref(`${UID}/thresholds`);

thresholdDataRef.on('value', (snapshot) => {
  if (snapshot.exists()) {
    console.log("--- Data THRESHOLD Berubah ---");
    const data = snapshot.val(); // Dapatkan seluruh objek threshold
    
    // Tampilkan seluruh objek threshold
    console.log("Threshold Saat Ini:", data);
    
    console.log("-----------------------------\n");
  } else {
    console.log("Tidak ada data threshold yang ditemukan.");
  }
}, (errorObject) => {
  console.error("Gagal membaca data threshold:", errorObject);
});


console.log("Script berjalan. Mendengarkan perubahan data dari Firebase...");