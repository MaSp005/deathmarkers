if (process.argv.includes("--help") || process.argv.includes("-h")) {
  console.log(`
  -a           Appends to the existing deaths.db instead of replacing all records
  -B           Does not back up the database before writing (recommended with -a)
  -c [count]   Determined count of dummy deaths to add (2000 by default)
  -l [levelid] Changes levelid on every added record (otherwise random per entry)
  -i [fn]      Database Filename (deaths.db by default)
  WARNING: Does not support abbreviated Flags (like -aB) im too lazy to implement them
    `);
  process.exit(0);
}

const DATABASE_FILENAME = process.argv.includes("-i") ?
  process.argv[process.argv.indexOf("-c") + 1] : "deaths.db";
const DUMMY_COUNT =
  process.argv.includes("-c") ?
    parseInt(process.argv[process.argv.indexOf("-c") + 1]) :
    2000;
const LEVELID =
  process.argv.includes("-l") ?
    parseInt(process.argv[process.argv.indexOf("-l") + 1]) : 0;

const alphabet = "0123456789abcdef";
const random = l => new Array(l).fill(0).map(_ => alphabet[Math.floor(Math.random() * alphabet.length)]).join("");

const fs = require("fs");
if (!fs.existsSync(DATABASE_FILENAME)) process.exit(1);
if (!process.argv.includes("-B")) {
  let suffix = 0;
  let copyFilename;
  do {
    suffix++;
    copyFilename = DATABASE_FILENAME.replace(".", suffix + ".");
  } while (fs.existsSync(copyFilename));
  fs.copyFileSync(DATABASE_FILENAME, copyFilename);
}

const db = require("better-sqlite3")(DATABASE_FILENAME);

if (!process.argv.includes("-a")) {
  db.prepare("DELETE FROM format1;").run();
  db.prepare("DELETE FROM format2;").run();
}

const insert = db.prepare("INSERT INTO format1 (userident, levelid, levelversion, practice, x, y, percentage) VALUES (?, ?, ?, ?, ?, ?, ?)")

db.transaction(() => {
  for (i = 0; i < DUMMY_COUNT; i++) {
    insert.run(
      random(40),
      LEVELID || Math.floor(Math.random() * 200_000_000),
      Math.floor(Math.random() * 5),
      (Math.random() > .5) * 1,
      Math.random() * 30_000,
      Math.random() * 5_000,
      Math.floor(Math.random() * 102),
    );

    process.stdout.clearLine();
    process.stdout.cursorTo(0);
    process.stdout.write(`Wrote ${i + 1} / ${DUMMY_COUNT}`);
  }
})();