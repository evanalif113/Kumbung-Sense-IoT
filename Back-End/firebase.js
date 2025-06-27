var admin = require("firebase-admin");

var serviceAccount = require("./kunci_firebase.json");

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: "https://kumbung-sense-default-rtdb.asia-southeast1.firebasedatabase.app"
});