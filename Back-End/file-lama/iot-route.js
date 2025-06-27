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



// ===== Start Server =====
app.listen(port, function () {
  console.log(`Server berjalan di http://localhost:${port}`);
});