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
const csv = require("csv");
const md = require("markdown-it")({ html: true, breaks: true })
  .use(require('markdown-it-named-headings'));

app.use(expr.static("front"));

let guideHtml = "";
let guideLastRead = 0;
renderGuide();

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
  // Table for Format 1
  db.prepare("CREATE TABLE IF NOT EXISTS format1 (\
  userident CHAR(40) NOT NULL,\
  levelid INT UNSIGNED NOT NULL,\
  levelversion TINYINT UNSIGNED DEFAULT 0,\
  practice BIT DEFAULT 0,\
  x DOUBLE NOT NULL,\
  y DOUBLE NOT NULL,\
  percentage SMALLINT UNSIGNED NOT NULL\
  );").run();

  // Table for Format 2
  db.prepare("CREATE TABLE IF NOT EXISTS format2 (\
  userident CHAR(40) NOT NULL,\
  levelid INT UNSIGNED NOT NULL,\
  levelversion TINYINT UNSIGNED DEFAULT 0,\
  practice BIT DEFAULT 0,\
  x DOUBLE NOT NULL,\
  y DOUBLE NOT NULL,\
  percentage SMALLINT UNSIGNED NOT NULL,\
  coins TINYINT DEFAULT 0,\
  itemdata DOUBLE DEFAULT 0\
  );").run();

  // Indexes for both tables
  db.prepare("CREATE INDEX IF NOT EXISTS format1index ON format1 (levelid);").run();
  db.prepare("CREATE INDEX IF NOT EXISTS format2index ON format2 (levelid);").run();
} catch (e) {
  console.error("Error preparing database:");
  console.error(e);
  process.exit(1);
}

app.get("/list", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  if (!/^\d+$/.test(req.query.levelid)) return res.sendStatus(418);
  let levelId = parseInt(req.query.levelid);

  let accept = req.query.response || "csv";
  if (accept != "json" && accept != "csv") return res.sendStatus(400);

  let isPlatformer = req.query.platformer == "true";
  let query = isPlatformer ?
    `SELECT x,y FROM format1 WHERE levelid == ? UNION
  SELECT x,y FROM format2 WHERE levelid == ?;` :
    `SELECT x,y,percentage FROM format1 WHERE levelid == ? AND percentage < 101 UNION
  SELECT x,y,percentage FROM format2 WHERE levelid == ? AND percentage < 101;`;

  benchmark("query");
  let deaths = db.prepare(query)
    .all(levelId, levelId);
  benchmark();

  // LEGACY: Serve only CSV to minimize traffic
  if (accept == "json") res.json(deaths
    .map(d => (isPlatformer ? [d.x, d.y] : [d.x, d.y, d.percentage])));
  else {
    res.contentType("text/csv");
    benchmark("serve");
    csv.stringify(deaths, { header: true }).pipe(res);
    benchmark();
  }
});

app.get("/analysis", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  if (!/^\d+$/.test(req.query.levelid)) return res.sendStatus(418);
  let levelId = parseInt(req.query.levelid);

  let accept = req.query.response || "csv";
  if (accept != "json" && accept != "csv") return res.sendStatus(400);

  benchmark("query");
  let deaths = db.prepare("SELECT userident,levelversion,practice,x,y,percentage FROM format1 WHERE levelid = ?;").all(levelId);
  benchmark();

  benchmark("salt" + deaths.length);
  let salt = "_" + random(10);
  deaths.forEach(d => d.userident = crypto.createHash("sha1")
    .update(d.userident + salt).digest("hex"));
  benchmark();

  // LEGACY: Serve only CSV to minimize traffic
  if (accept == "json") res.json(deaths);
  else {
    res.contentType("text/csv");
    benchmark("serve");
    csv.stringify(deaths, { header: true }).pipe(res);
    benchmark();
  }
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
        db.prepare("INSERT INTO format1 (userident, levelid, levelversion, practice, x, y, percentage)\
          VALUES (@userident, @levelid, @levelversion, @practice, @x, @y, @percentage)")
          .run(req.body);
        break;
      case 2:
        db.prepare("INSERT INTO format2 (userident, levelid, levelversion, practice, x, y, percentage, coins, itemdata)\
          VALUES (@userident, @levelid, @levelversion, @practice, @x, @y, @percentage, @coins, @itemdata)")
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