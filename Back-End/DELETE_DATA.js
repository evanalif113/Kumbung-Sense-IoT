var admin = require("firebase-admin");

var serviceAccount = require("./kunci_firebase.json");

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: "https://kumbung-sense-default-rtdb.asia-southeast1.firebasedatabase.app"
});

// Dapatkan referensi ke database
const db = admin.database();

// Tentukan path ke data yang ingin dihapus
const ref = db.ref('/JU3dPIvArGbuoRUywdcTmdVkQRr2/sensor');

// Hapus data pada referensi yang ditentukan
ref.remove()
  .then(() => {
    console.log('Semua data pada path /JU3dPIvArGbuoRUywdcTmdVkQRr2/sensor berhasil dihapus.');
    process.exit(0); // Keluar dari skrip setelah berhasil
  })
  .catch((error) => {
    console.error('Gagal menghapus data:', error);
    process.exit(1); // Keluar dari skrip dengan kode error
  });