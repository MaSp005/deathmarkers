const DATABASE_FILENAME = "deaths.db";
const DUMMY_COUNT = 2000;
const LEVELID = 112920641;

const alphabet = "0123456789abcdef";
const random = l => new Array(l).fill(0).map(_ => alphabet[Math.floor(Math.random() * alphabet.length)]).join("");

const fs = require("fs");

let suffix = 0;
let copyFilename;
do {
  suffix++;
  copyFilename = DATABASE_FILENAME.replace(".", suffix + ".");
} while (fs.existsSync(copyFilename));
fs.copyFileSync(DATABASE_FILENAME, copyFilename);

const db = require("better-sqlite3")(DATABASE_FILENAME);

db.prepare("DELETE FROM format1;").run();
db.prepare("DELETE FROM format2;").run();

for (i = 0; i < DUMMY_COUNT; i++) {
  db.prepare("INSERT INTO format1 (userident, levelid, levelversion, practice, x, y, percentage) VALUES (?, ?, ?, ?, ?, ?, ?)")
    .run(
      random(40),
      // Math.floor(Math.random() * 200_000_000),
      LEVELID,
      Math.floor(Math.random() * 5),
      (Math.random() > .5) * 1,
      Math.floor(Math.random() * 30_000),
      Math.floor(Math.random() * 5_000),
      Math.floor(Math.random() * 102),
    );
  process.stdout.clearLine();
  process.stdout.cursorTo(0);
  process.stdout.write(`Wrote ${i + 1} / ${DUMMY_COUNT}`);
}