var admin = require("firebase-admin");

// Ganti "./kunci_firebase.json" dengan path yang benar ke file kunci layanan Anda
var serviceAccount = require("./kunci_firebase.json");

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: "https://kumbung-sense-default-rtdb.asia-southeast1.firebasedatabase.app"
});

// Dapatkan referensi ke database
const db = admin.database();

// Tentukan path ke node 'data' yang berisi semua log sensor
const sensorDataRef = db.ref('/JU3dPIvArGbuoRUywdcTmdVkQRr2/sensor/data');

// Buat query untuk mendapatkan 1 data terakhir berdasarkan key (timestamp)
const latestSensorQuery = sensorDataRef.orderByKey().limitToLast(1);

// Dengarkan perubahan pada query tersebut
latestSensorQuery.on('value', (snapshot) => {
  // Periksa apakah data ada
  if (snapshot.exists()) {
    console.log("--- Data Sensor Terbaru Diterima ---");
    
    // Karena kita hanya mengambil 1 data, kita bisa loop untuk mendapatkannya
    snapshot.forEach((childSnapshot) => {
      const timestamp = childSnapshot.key; // Key-nya adalah timestamp
      const data = childSnapshot.val();   // Val()-nya adalah objek data sensor
      
      // Tampilkan data ke konsol
      console.log(`Timestamp: ${timestamp}`);
      console.log(`Temperature: ${data.temperature}°C`);
      console.log(`Humidity: ${data.humidity}%`);
      console.log(`Moisture: ${data.moisture}%`);
      console.log(`Light: ${data.light}`); // Menambahkan data 'light'
    });
    console.log("------------------------------------");

  } else {
    console.log("Tidak ada data yang ditemukan pada path tersebut.");
  }
}, (errorObject) => {
  // Tangani error jika gagal membaca data
  console.error("Gagal membaca data:", errorObject);
});