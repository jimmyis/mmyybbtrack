const admin = require("firebase-admin");
const functions = require("firebase-functions");
const {
  getFirestore, /* , Timestamp, FieldValue, Filter */
} = require("firebase-admin/firestore");


admin.initializeApp({
  credential: admin.credential.cert({
    privateKey: functions.config().private.key.replace(/\\n/g, "\n"),
    projectId: functions.config().project.id,
    clientEmail: functions.config().client.email,
  }),
  databaseURL: "https://jimmyis-iot-proto-default-rtdb.asia-southeast1.firebasedatabase.app",
});

const firestore = getFirestore();

module.exports = {admin, firestore};
