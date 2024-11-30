const DATABASE_FILENAME = "example.db";
const PORT = 8048;

const ARRANGEMENTS = [
  `A_L_I`,
  `A_I_L`,
  `I_L_A`,
  `I_A_L`,
  `L_I_A`,
  `L_A_I`
];

const db = require("better-sqlite3")(DATABASE_FILENAME);
const expr = require("express");
const app = expr();
const crypto = require("crypto");

function createUserIdent(accountid, accountname, levelid) {
  let source = ARRANGEMENTS[accountid % ARRANGEMENTS.length];

  source = source.replace("L", levelid)
    .replace("I", accountid)
    .replace("A", accountname);

  return crypto.createHash("sha1").update(source).digest("hex");
}

db.prepare("CREATE TABLE IF NOT EXISTS deaths (" +
  "userident CHAR(40) NOT NULL," +
  "levelid INT UNSIGNED NOT NULL," +
  "x DOUBLE NOT NULL," +
  "y DOUBLE NOT NULL," +
  "percentage SMALLINT UNSIGNED NOT NULL," +
  "coins TINYINT DEFAULT 0," +
  "itemdata DOUBLE DEFAULT 0" +
  ")").run();

app.get("/deathmarkers/list", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  const deaths = db.prepare("SELECT x,y FROM deaths WHERE levelid = ?;").all(req.query.levelid);
  res.json(deaths.map(d => ([d.x, d.y])));
});

app.get("/deathmarkers/analysis", (req, res) => {
  if (!req.query.levelid) return res.sendStatus(400);
  const deaths = db.prepare("SELECT userident,x,y,percentage,coins,itemdata FROM deaths WHERE levelid = ?;").all(req.query.levelid);
  res.json(deaths);
});

app.post("/deathmarkers/submit", expr.raw(), (req, res) => {
  try {
    req.body = JSON.parse(req.body);
  } catch {
    return res.status(400).send("Wrongly formatted JSON");
  }
  try {
    if (!req.body.levelid) return res.status(400).send("levelid is not supplied");
    if (!req.body.userident) {
      if (!req.body.playername || !req.body.accountid)
        return res.status(400).send("neither userident nor playername and accountid were supplied");
      req.body.userident = createUserIdent(req.body.accountid, req.body.playername, req.body.levelid);
    }
    if (!req.body.x || !req.body.y) return res.sendStatus(400);
    if (!req.body.percentage) return res.sendStatus(400);
    if (!req.body.coins) {
      req.body.coins =
        Number(!!req.body.coin1) |
        Number(!!req.body.coin2) << 1 |
        Number(!!req.body.coin3) << 2;
    }
    if (!req.body.itemdata) req.body.itemdata = 0;
  } catch {
    return res.status(400).send("Unexpected error when parsing request");
  }

  try {
    db.prepare("INSERT INTO deaths (userident, levelid, x, y, percentage, coins, itemdata) VALUES " +
      `(${req.body.userident}, ${req.body.levelid}, ${req.body.x}, ${req.body.y}, ${req.body.percentage}, ${req.body.coins}, ${req.body.itemdata});`
    ).run();
  } catch {
    return res.status(500).send("Error writing to the database. May be due to wrongly formatted input. Try again.");
  }
})

app.listen(PORT, () => { console.log("Listening on :" + PORT) });