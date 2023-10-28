const {firestore} = require("./src/configs/firebase-admin.js");
const functions = require("firebase-functions");

const rootCollectionRef = firestore.collection("inventory-tracking-system");
const dataRef = rootCollectionRef.doc("data");
// const rfidBasesRef =
//   firestore.collection("inventory-tracking-system/data/bases");
// const rfidTagsRef =
//  firestore.collection("inventory-tracking-system/data/rfid-tags");
// const assetsRef =
//  firestore.collection("inventory-tracking-system/data/assets");

// // Create and Deploy Your First Cloud Functions
// // https://firebase.google.com/docs/functions/write-firebase-functions
//
// exports.helloWorld = functions.https.onRequest((request, response) => {
//   functions.logger.info("Hello logs!", {structuredData: true});
//   response.send("Hello from Firebase!");
// });

exports.baseActiveness = functions
    .region("asia-southeast1")
    .https.onRequest((req, res) => {
      console.info("Base is calling");
      res.status(200).send("Hello world!");
    });

exports.scheduledFunction = functions
    .region("asia-southeast1")
    .pubsub.schedule("*/2 * * * *")
    .timeZone("Asia/Bangkok")
    .onRun((context) => {
      // console.info("This will be run every minute!");

      // Reset bases to not active every 2 minutes
      dataRef.set({
        bases_activeness: {
          base_01: false,
          base_02: false,
        },
      }, {merge: true});
      // rfidBasesRef.get().then((snapshot) => {
      //   if (!snapshot.empty) {
      //     console.info("Data Exists");
      //     snapshot.forEach((doc) => {
      //       console.log(doc.id, "=>", doc.data());
      //     });
      //   } else {
      //     console.info("Data Not Exists");
      //   }
      // });
    });
