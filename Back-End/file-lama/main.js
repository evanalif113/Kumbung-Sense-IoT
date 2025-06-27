const express = require('express');
const bodyParser = require('body-parser');
const mysql = require('mysql2');

const app = express();
const port = 2518;

// ===== Middleware =====
// parse application/x-www-form-urlencoded
app.use(bodyParser.urlencoded({extended: false }));

// ===== Koneksi MySQL =====
const db = mysql.createPool({
  host: 'localhost',
  user: 'root',
  password: '',
  database: 'DB_JAMUR'
});

///////////////////////////////////////////////////////////////////////////////////
// API TABEL PENGGUNA
// Endpoint signup (GET):
// Contoh: 
// http://localhost:2518/api/signup?username=namauser&password=katakunci

  app.get('/api/signup', (req, res) => {
    const { username, password } = req.query;
    if (!username || !password) {
      return res.status(400).json({ message: 'Username dan password wajib diisi.' });
    }

    // Cek apakah username sudah ada
    db.query(
      'SELECT id_pengguna FROM PENGGUNA WHERE username = ?',
      [username],
      (err, results) => {
        if (err) {
          console.error(err);
          return res.status(500).json({ message: 'Terjadi kesalahan server.' });
        }
        if (results.length > 0) {
          return res.status(409).json({ message: 'Username sudah terdaftar.' });
        }

        // Simpan user baru
        db.query(
          'INSERT INTO PENGGUNA (username, password) VALUES (?, ?)',
          [username, password],
          (err2, result) => {
            if (err2) {
              console.error(err2);
              return res.status(500).json({ message: 'Gagal mendaftar.' });
            }
            res.status(201).json({ message: 'Pendaftaran berhasil.' });
          }
        );
      }
    );
  });
/////////////////////////////////////////////////////////////////////////////////
// API TABEL DATA_SENSOR
// ===== ROUTE: GET kirim via URL-encoded =====
// contoh: GET /api/data-sensor/send?id_sensor=1&temperature=24.5&humidity=60&moisture=30&light=500
app.get('/api/data-sensor/send', (req, res) => {
  const { id_sensor, temperature, humidity, moisture, light } = req.query;

  if ( !id_sensor || !temperature || !humidity || !moisture || !light) {
    return res.status(400).send('Semua parameter (id_sensor, temperature, humidity, moisture, light) wajib diisi.');
  }

  const sql = `
    INSERT INTO DATA_SENSOR (id_sensor, temperature, humidity, moisture, light) 
    VALUES (?, ?, ?, ?, ?)
  `;

  db.query(sql, [id_sensor, temperature, humidity, moisture, light], (err) => {
    if (err) {
      console.error(err);
      return res.status(500).send('Gagal menyimpan data.');
    }
    console.log(`Data berhasil disimpan (GET): id_sensor=${id_sensor}, temperature=${temperature}, humidity=${humidity}, moisture=${moisture}, light=${light}`);
    res.send(`Data diterima via GET: 
      id_sensor=${id_sensor},
      temperature=${temperature}, 
      humidity=${humidity}, 
      moisture=${moisture}, 
      light=${light}`);
  });
});

// ===== ROUTE: GET ambil semua data =====
// GET /api/data/all
app.get('/api/data/all', (req, res) => {
  const sql = `
    SELECT id_sensor, temperature, humidity, moisture, light, created_at 
    FROM DATA_SENSOR 
    ORDER BY id_sensor DESC
  `;

  db.query(sql, (err, results) => {
    if (err) {
      console.error(err);
      return res.status(500).send('Gagal mengambil data.');
    }
    res.json(results);
  });
});

// ===== ROUTE: GET ambil satu record by ID =====
// GET /api/data/:id
app.get('/api/data/id', (req, res) => {
  const id = req.params.id;
  const sql = `
    SELECT id_sensor, temperature, humidity, moisture, light, created_at 
    FROM DATA_SENSOR 
    WHERE id_sensor = ?
  `;

  db.query(sql, [id], (err, results) => {
    if (err) {
      console.error(err);
      return res.status(500).send('Gagal mengambil data.');
    }
    if (results.length === 0) {
      return res.status(404).send('Data tidak ditemukan.');
    }
    res.json(results[0]);
  });
});
///////////////////////////////////////////////////////////////////////////////////////
// API UTILITAS DATA
app.get('/api/data-sensor/by-user', (req, res) => {
  const { id_pengguna, id_kumbung, id_sensor } = req.query;
  if (!id_pengguna || !id_kumbung || !id_sensor) {
    return res.status(400).send('Parameter id_pengguna, id_kumbung, dan id_sensor wajib diisi.');
  }
  const sql = `
    SELECT ds.*
    FROM DATA_SENSOR ds
    JOIN SENSOR s ON ds.id_sensor = s.id_sensor
    JOIN KUMBUNG k ON s.id_kumbung = k.id_kumbung
    JOIN PENGGUNA p ON k.id_pengguna = p.id_pengguna
    WHERE p.id_pengguna = ? AND k.id_kumbung = ? AND s.id_sensor = ?
    ORDER BY ds.created_at DESC
  `;
  db.query(sql, [id_pengguna, id_kumbung, id_sensor], (err, results) => {
    if (err) {
      console.error(err);
      return res.status(500).send('Gagal mengambil data.');
    }
    res.json(results);
  });
});

// ===== Start Server =====
app.listen(port, function () {
  console.log(`Server berjalan di http://localhost:${port}`);
});

