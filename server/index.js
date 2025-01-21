const DATABASE_FILENAME = "deaths.db";
const PORT = 8048;
// TODO: Differentiate Formats

const ARRANGEMENTS = [
  `A_L_I`,
  `A_I_L`,
  `I_L_A`,
  `I_A_L`,
  `L_I_A`,
  `L_A_I`
];
const GUIDE_FILENAME = "./guide.md";

const db = require("better-sqlite3")(DATABASE_FILENAME);
const expr = require("express");
const app = expr();
const crypto = require("crypto");
const fs = require("fs");
const md = require("markdown-it")({ html: true, breaks: true })
  .use(require('markdown-it-named-headings'));

app.use(expr.static("front"));

let guideHtml = "";
let guideLastRead = 0;
renderGuide();

function renderGuide() {
  console.log("Rerendering guide...");
  let markdown = fs.readFileSync(GUIDE_FILENAME, "utf8");
  let chapters = markdown.split("\n")
    .filter(x => x.startsWith("##"))
    .map(x => x.slice(2).trimEnd().replace(" ", ""))

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
  guideHtml = md.render(markdown)
  guideHtml = guideHtml.replace(/src=".*?front\//g, "src=\"");
  guideHtml = guideHtml.replace(/<!--.*?-->/g, "");
  guideLastRead = fs.statSync(GUIDE_FILENAME).mtime;
}

function createUserIdent(userid, username, levelid) {
  let source = ARRANGEMENTS[userid % ARRANGEMENTS.length];

  source = source.replace("L", levelid)
    .replace("I", userid)
    .replace("A", username);

  return crypto.createHash("sha1").update(source).digest("hex");
}

db.prepare("CREATE TABLE IF NOT EXISTS format1 (\
  userident CHAR(40) NOT NULL,\
  levelid INT UNSIGNED NOT NULL,\
  x DOUBLE NOT NULL,\
  y DOUBLE NOT NULL,\
  percentage SMALLINT UNSIGNED NOT NULL\
  )").run();

db.prepare("CREATE TABLE IF NOT EXISTS format2 (\
  userident CHAR(40) NOT NULL,\
  levelid INT UNSIGNED NOT NULL,\
  x DOUBLE NOT NULL,\
  y DOUBLE NOT NULL,\
  percentage SMALLINT UNSIGNED NOT NULL,\
  coins TINYINT DEFAULT 0,\
  itemdata DOUBLE DEFAULT 0\
  )").run();

app.get("/list", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  if (!/^\d+$/.test(req.query.levelid)) return res.sendStatus(418);
  const deaths = db.prepare(
    "SELECT x,y,percentage FROM format1 WHERE levelid = ? UNION\
    SELECT x,y,percentage FROM format2 WHERE levelid = ?;"
  ).all(req.query.levelid, req.query.levelid);
  res.json(deaths.map(d => ([d.x, d.y, d.percentage])));
});

app.get("/analysis", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  if (!/^\d+$/.test(req.query.levelid)) return res.sendStatus(418);
  const deaths = db.prepare("SELECT userident,x,y,percentage,coins,itemdata FROM format1 WHERE levelid = ?;").all(req.query.levelid);
  res.json(deaths);
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

  } catch {
    return res.status(400).send("Unexpected error when parsing request");
  }

  try {
    switch (format) {
      case 1:
        db.prepare("INSERT INTO format1 (userident, levelid, x, y, percentage) VALUES (?, ?, ?, ?, ?)").run(
          req.body.userident,
          req.body.levelid,
          req.body.x,
          req.body.y,
          req.body.percentage,
        );
        break;
      case 2:
        db.prepare("INSERT INTO format2 (userident, levelid, x, y, percentage) VALUES (?, ?, ?, ?, ?)").run(
          req.body.userident,
          req.body.levelid,
          req.body.x,
          req.body.y,
          req.body.percentage,
          req.body.coins,
          req.body.itemdata,
        );
        break;
    }
    res.sendStatus(204);
  } catch {
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