if (process.argv.includes("--help") || process.argv.includes("-h")) {
  console.log(`
  -b      Benchmark select features in action
  -i [fn] Database Filename
    `);
  process.exit(0);
}

const DATABASE_FILENAME = process.argv.includes("-i") ?
  process.argv[process.argv.indexOf("-c") + 1] : "deaths.db";
const PORT = 8048;
const BUFFER_SIZE = 500;

const alphabet = "ABCDEFGHIJOKLMNOPQRSTUVWXYZabcdefghijoklmnopqrstuvwxyz0123456789";
const random = l => new Array(l).fill(0).map(_ => alphabet[Math.floor(Math.random() * alphabet.length)]).join("");

const GUIDE_FILENAME = "./guide.md";

const benchmark = (() => {
  if (!process.argv.includes("-b")) return () => { };
  console.log("\u001b[1;31mBenchmarking mode activated. Do not use in production.\u001b[0m");
  let active = "";
  return name => {
    if (active) {
      console.timeEnd(active);
      active = "";
    }
    else console.time(active = (name || "default"));
  }
})();

const db = require("better-sqlite3")(DATABASE_FILENAME);
const expr = require("express");
const app = expr();
const crypto = require("crypto");
const fs = require("fs");
const md = require("markdown-it")({ html: true, breaks: true })
  .use(require('markdown-it-named-headings'));
const { Readable } = require("stream");

app.use(expr.static("front"));

let guideHtml = "";
let guideLastRead = 0;
renderGuide();

function csvStream(array, columns, map = r => r) {
  return new Readable({
    read() {
      let buffer = [columns];
      for (const row of array) {
        buffer.push(map(row).join(","));

        if (buffer.length >= BUFFER_SIZE) {
          this.push(buffer.join("\n") + "\n");
          buffer = [];
        }
      }
      this.push(buffer.join("\n"));
      this.push(null);
    }
  })
}

function renderGuide() {
  console.log("Rerendering guide...");
  benchmark("guideRender");
  let markdown = fs.readFileSync(GUIDE_FILENAME, "utf8");
  markdown = markdown.replace(/<!--.*?-->\n?/gs, ""); // Remove comments
  let chapters = markdown.split("\n")
    .filter(x => x.startsWith("##"))
    .map(x => x.slice(2).trimEnd().replace(" ", "")); // Identify headings

  // Index Chapters by heading depth and render nested <ol>s
  let levels = [0];
  let last = 0;
  chapters = chapters.map((x, i) => {
    let c = x.search(/[^#]/);
    x = x.slice(c);
    if (c > last) levels[c] = 0;
    if (c < last) levels.splice(c + 1, Infinity);
    if (!levels[c]) levels[c] = 0;
    last = c;
    return `${"\t".repeat(c)}${++levels[c]}. [${x}]` +
      `(#${x.toLowerCase().replaceAll(" ", "-")})`;
  });
  markdown = markdown.replace("<?>TOC",
    chapters.join("\n"));

  guideHtml = md.render(markdown);
  // markdown preview requires directory, running server hosts files on root
  guideHtml = guideHtml.replace(/src=".*?front\//g, "src=\"");
  // Replace newlines and whitespace between HTML tags
  guideHtml = guideHtml.replace(/>\s+</g, "><");
  guideLastRead = fs.statSync(GUIDE_FILENAME).mtime;
  benchmark();
}

function createUserIdent(userid, username, levelid) {
  let source = `${username}_${userid}_${levelid}`;

  return crypto.createHash("sha1").update(source).digest("hex");
}

try {
  db.exec("BEGIN TRANSACTION;");
  db.exec(fs.readFileSync('schema.sql', 'utf8'));
  db.exec("COMMIT;");
} catch (e) {
  db.exec("ROLLBACK;");
  console.error("Error preparing database:");
  console.error(e);
  process.exit(1);
}

app.get("/list", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  if (!/^\d+$/.test(req.query.levelid)) return res.sendStatus(418);
  let levelId = parseInt(req.query.levelid);

  let accept = req.query.response || "csv";
  if (accept != "csv") return res.sendStatus(400);

  let isPlatformer = req.query.platformer == "true";
  let columns = isPlatformer ? "x,y" : "x,y,percentage";
  let query = isPlatformer ?
    `SELECT ${columns} FROM format1 WHERE levelid == @levelId UNION
  SELECT ${columns} FROM format2 WHERE levelid == @levelId` :
    `SELECT ${columns} FROM format1 WHERE levelid == @levelId AND percentage < 101 UNION
  SELECT ${columns} FROM format2 WHERE levelid == @levelId AND percentage < 101;`;

  benchmark("query");
  let deaths = db.prepare(query)
    .raw(true)
    .all({ levelId });
  benchmark();

  res.contentType("text/csv");

  benchmark("serve");
  csvStream(deaths, columns).pipe(res);
  // let csv = columns + "\n" + deaths.map(r => r.join(",")).join("\n");
  // res.send(csv);
  benchmark();
});

app.get("/analysis", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  if (!/^\d+$/.test(req.query.levelid)) return res.sendStatus(418);
  let levelId = parseInt(req.query.levelid);

  let accept = req.query.response || "csv";
  if (accept != "csv") return res.sendStatus(400);

  let columns = "userident,levelversion,practice,x,y,percentage";
  let salt = "_" + random(10);

  benchmark("query");
  let deaths = db.prepare(`SELECT ${columns} FROM format1 WHERE levelid = ?;`).raw(true).all(levelId);
  benchmark();

  res.contentType("text/csv");

  benchmark("serve");
  csvStream(deaths, columns,
    d => ([
      crypto.createHash("sha1")
        .update(d[0] + salt).digest("hex"),
      ...d.slice(1)
    ])
  ).pipe(res);
  benchmark();
});

app.all("/submit", expr.text({
  type: "*/*"
}), (req, res) => {
  benchmark("parseSubmission");
  try {
    req.body = JSON.parse(req.body.toString());
  } catch {
    return res.status(400).send("Wrongly formatted JSON");
  }
  let format;
  try {
    format = req.body.format;

    if (typeof format != "number") return res.status(400).send("Format not supplied");

    if (typeof req.body.levelid != "number")
      return res.status(400).send("levelid was not supplied or not numerical");

    if (typeof req.body.levelversion != "number") req.body.levelversion = 0;

    req.body.practice = (!!req.body.practice) * 1;

    if (typeof req.body.userident != "string") {
      if (!req.body.playername || !req.body.userid)
        return res.status(400)
          .send("Neither userident nor playername and userid were supplied");

      req.body.userident = createUserIdent(req.body.userid,
        req.body.playername, req.body.levelid);
    } else {
      if (!/^[0-9a-f]{40}$/i.test(req.body.userident))
        return res.status(400).send("userident has incorrect length or illegal characters (should be 40 hex characters)");
    }

    if (typeof req.body.x != "number" || typeof req.body.y != "number") return res.sendStatus(400);

    if (typeof req.body.percentage != "number") return res.sendStatus(400);

    if (format >= 2) {
      if (!req.body.coins) {
        req.body.coins =
          Number(!!req.body.coin1) |
          Number(!!req.body.coin2) << 1 |
          Number(!!req.body.coin3) << 2;
      }
      if (!req.body.itemdata) req.body.itemdata = 0;
    }

  } catch (e) {
    console.warn(e);
    return res.status(400).send("Unexpected error when parsing request");
  }
  benchmark();

  benchmark("insert");
  try {
    switch (format) {
      case 1:
        db.prepare("INSERT INTO format1 VALUES (@userident, @levelid, @levelversion, @practice, @x, @y, @percentage)")
          .run(req.body);
        break;
      case 2:
        db.prepare("INSERT INTO format2 VALUES (@userident, @levelid, @levelversion, @practice, @x, @y, @percentage, @coins, @itemdata)")
          .run(req.body);
        break;
    }
    res.sendStatus(204);
  } catch (e) {
    console.warn(e);
    return res.status(500).send("Error writing to the database. May be due to wrongly formatted input. Try again.");
  }
  benchmark();
});

app.get("/", (req, res) => {
  if (fs.statSync(GUIDE_FILENAME).mtime > guideLastRead) renderGuide();
  res.send(guideHtml);
});

app.all("*", (req, res) => {
  res.redirect("/");
});

app.listen(PORT, () => { console.log("Listening on :" + PORT) });