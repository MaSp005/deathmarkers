if (process.argv.includes("--help") || process.argv.includes("-h")) {
  console.log(`
  -a           Appends to the existing Database instead of replacing all records
  -B           Does not back up the database before writing (recommended with -a)
  -g           Just counts number of entries per level id in the database, does nothing else
  -c [count]   Determined count of dummy deaths to add (2000 by default)
  -l [levelid] Changes levelid on every added record (otherwise random per entry)
  -i [fn]      Database Filename (deaths.db by default)
  WARNING: Does not support abbreviated Flags (like -aB) im too lazy to implement them
    `);
  process.exit(0);
}

const fs = require("fs");
const DATABASE_FILENAME = process.argv.includes("-i") ?
  process.argv[process.argv.indexOf("-i") + 1] : "deaths.db";
const DUMMY_COUNT =
  process.argv.includes("-c") ?
    parseInt(process.argv[process.argv.indexOf("-c") + 1]) :
    2000;
const LEVELID =
  process.argv.includes("-l") ?
    parseInt(process.argv[process.argv.indexOf("-l") + 1]) :
    0;

if (process.argv.includes("-g")) {
  const db = require("better-sqlite3")(DATABASE_FILENAME);

  let [max, total] = db.prepare("SELECT max(levelid), count(*) FROM format1")
    .raw().get();
  max = Math.ceil(Math.log10(max)) + 1;
  let size = fs.statSync(DATABASE_FILENAME).size / (1 << 20);

  db.prepare("SELECT levelid, count(*) as num FROM format1\
    GROUP BY levelid ORDER BY num DESC")
    .raw().all().forEach(r => {
      if (LEVELID && LEVELID != r[0]) return;
      // Arguably easier to early return here than modify the query and allat
      console.log(`${r[0].toString().padEnd(max, " ")} ${r[1]}`);
    });
  console.log(`Total: ${total}, filesize: ` +
    (size < 1 ? size * (1 << 10) + "kB" : size + "MB"));

  process.exit(0);
}

const alphabet = "0123456789abcdef";
const random = l => new Array(l).fill(0)
  .map(_ => alphabet[Math.floor(Math.random() * alphabet.length)])
  .join("");

if (!process.argv.includes("-B") && fs.existsSync(DATABASE_FILENAME)) {
  let suffix = 0;
  let copyFilename;
  do {
    suffix++;
    copyFilename = DATABASE_FILENAME.replace(".", suffix + ".");
  } while (fs.existsSync(copyFilename));
  fs.copyFileSync(DATABASE_FILENAME, copyFilename);
}

const db = require("better-sqlite3")(DATABASE_FILENAME);
db.exec(fs.readFileSync('schema.sql', 'utf8'));

if (!process.argv.includes("-a")) {
  db.prepare("DELETE FROM format1;").run();
  db.prepare("DELETE FROM format2;").run();
}

const insert = db.prepare("INSERT INTO format1 VALUES\
  (@userident, @levelid, @levelversion, @practice, @x, @y, @percentage)");

db.transaction(() => {
  for (i = 0; i < DUMMY_COUNT; i++) {
    insert.run({
      userident: random(40),
      levelid: LEVELID || Math.floor(Math.random() * 200_000_000),
      levelversion: Math.floor(Math.random() * 5),
      practice: (Math.random() > .5) * 1,
      x: Math.random() * 30_000,
      y: Math.random() * 5_000,
      percentage: Math.floor(Math.random() * 102),
    });

    process.stdout.clearLine();
    process.stdout.cursorTo(0);
    process.stdout.write(`Wrote ${i + 1} / ${DUMMY_COUNT}`);
  }
})();