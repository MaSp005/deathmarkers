const DATABASE_FILENAME = "deaths.db";
const PORT = 8048;
// TODO: Differentiate Formats

const alphabet = "ABCDEFGHIJOKLMNOPQRSTUVWXYZabcdefghijoklmnopqrstuvwxyz0123456789";
const random = l => new Array(l).fill(0).map(_ => alphabet[Math.floor(Math.random() * alphabet.length)]).join("");

const GUIDE_FILENAME = "./guide.md";

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
    return `${"\t".repeat(c)}${++levels[c]}. [${x}](#${x.toLowerCase().replaceAll(" ", "-")})`
  });
  markdown = markdown.replace("<?>TOC", chapters.join("\n"));

  guideHtml = md.render(markdown);
  guideHtml = guideHtml.replace(/src=".*?front\//g, "src=\""); // markdown preview requires directory, running server hosts files on root
  guideHtml = guideHtml.replace(/>\s+</g, "><"); // Replace newlines and whitespace between HTML tags
  guideLastRead = fs.statSync(GUIDE_FILENAME).mtime;
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
  practice TINYINT DEFAULT 0,\
  x DOUBLE NOT NULL,\
  y DOUBLE NOT NULL,\
  percentage SMALLINT UNSIGNED NOT NULL\
  );").run();

  // Table for Format 2
  db.prepare("CREATE TABLE IF NOT EXISTS format2 (\
  userident CHAR(40) NOT NULL,\
  levelid INT UNSIGNED NOT NULL,\
  levelversion TINYINT UNSIGNED DEFAULT 0,\
  practice TINYINT DEFAULT 0,\
  x DOUBLE NOT NULL,\
  y DOUBLE NOT NULL,\
  percentage SMALLINT UNSIGNED NOT NULL,\
  coins TINYINT DEFAULT 0,\
  itemdata DOUBLE DEFAULT 0\
  );").run();

  // Indexes for both tables
  db.prepare("CREATE INDEX IF NOT EXISTS format1index ON format1 (\
  levelid\
  );").run();
  db.prepare("CREATE INDEX IF NOT EXISTS format2index ON format2 (\
  levelid\
  );").run();
} catch (e) {
  console.error("Error preparing database:");
  console.error(e);
  process.exit(1);
}

app.get("/list", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  if (!/^\d+$/.test(req.query.levelid)) return res.sendStatus(418);
  let levelId = parseInt(req.query.levelid);

  let accept = req.query.response || "json";
  if (accept != "json" && accept != "csv") return res.sendStatus(400);

  let isPlatformer = req.query.platformer == "true";
  let query = isPlatformer ?
    `SELECT x,y FROM format1 WHERE levelid == ? UNION
    SELECT x,y FROM format2 WHERE levelid == ?;` :
    `SELECT x,y,percentage FROM format1 WHERE levelid == ? AND percentage < 101 UNION
    SELECT x,y,percentage FROM format2 WHERE levelid == ? AND percentage < 101;`;

  let deaths = db.prepare(query)
    .all(levelId, levelId);

  // LEGACY: Serve only CSV to minimize traffic
  if (accept == "json") res.json(deaths.map(d => (isPlatformer ? [d.x, d.y] : [d.x, d.y, d.percentage])));
  else csv.stringify(deaths, { header: true }).pipe(res);
});

app.get("/analysis", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  if (!/^\d+$/.test(req.query.levelid)) return res.sendStatus(418);
  let levelId = parseInt(req.query.levelid);

  let accept = req.query.response || "json";
  if (accept != "json" && accept != "csv") return res.sendStatus(400);

  let deaths = db.prepare("SELECT userident,levelversion,practice,x,y,percentage FROM format1 WHERE levelid = ?;").all(levelId);

  let salt = "_" + random(10);
  deaths.forEach(d => d.userident = crypto.createHash("sha1").update(d.userident + salt).digest("hex"));

  // LEGACY: Serve only CSV to minimize traffic
  if (accept == "json") res.json(deaths);
  else csv.stringify(deaths, { header: true }).pipe(res);
});

app.all("/submit", expr.text({
  type: "*/*"
}), (req, res) => {
  try {
    req.body = JSON.parse(req.body.toString());
  } catch {
    return res.status(400).send("Wrongly formatted JSON");
  }
  let format;
  try {
    format = req.body.format;
    if (typeof format != "number") return res.status(400).send("Format not supplied");
    if (!req.body.levelid) return res.status(400).send("levelid is not supplied");
    if (!req.body.levelversion) req.body.levelversion = 1;
    req.body.practice = !!req.body.practice;
    if (!req.body.userident) {
      if (!req.body.playername || !req.body.userid)
        return res.status(400).send("Neither userident nor playername and userid were supplied");
      req.body.userident = createUserIdent(req.body.userid, req.body.playername, req.body.levelid);
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

  try {
    switch (format) {
      case 1:
        db.prepare("INSERT INTO format1 (userident, levelid, levelversion, practice, x, y, percentage) VALUES (?, ?, ?, ?, ?, ?, ?)")
          .run(
            req.body.userident,
            req.body.levelid,
            req.body.levelversion,
            req.body.practice * 1,
            req.body.x,
            req.body.y,
            req.body.percentage,
          );
        break;
      case 2:
        db.prepare("INSERT INTO format2 (userident, levelid, levelversion, practice, x, y, percentage) VALUES (?, ?, ?, ?, ?, ?, ?)")
          .run(
            req.body.userident,
            req.body.levelid,
            req.body.levelversion,
            req.body.practice * 1,
            req.body.x,
            req.body.y,
            req.body.percentage,
            req.body.coins,
            req.body.itemdata,
          );
        break;
    }
    res.sendStatus(204);
  } catch (e) {
    console.warn(e);
    return res.status(500).send("Error writing to the database. May be due to wrongly formatted input. Try again.");
  }
});

app.get("/", (req, res) => {
  if (fs.statSync(GUIDE_FILENAME).mtime > guideLastRead) renderGuide();
  res.send(guideHtml);
});

app.all("*", (req, res) => {
  res.redirect("/");
});

app.listen(PORT, () => { console.log("Listening on :" + PORT) });