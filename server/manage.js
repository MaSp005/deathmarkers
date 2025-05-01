if (process.argv.includes("--help") || process.argv.includes("-h")) {
  console.log(`
Command Line application managing the database. Only works with Postgres, intended for production, optimized for weeding out spam records.
    `);
  process.exit(0);
}

const DATABASE = require("./config.json").DATABASE.POSTGRES;
const { Pool } = require("pg");

const db = new Pool(DATABASE);
const term = require("terminal-kit").terminal;

term.grabInput();
term.clear();

let exclude = false;
let phase = 0;
let selected = [0, 0];
let offset = [0];
let levels, users;

function padRight(str, len) {
  return str + Array(len - str.length).fill(" ").join("");
}

async function getLevels(exclude) {
  let query = `SELECT a.levelid, COUNT(*) AS c${exclude ? "" : ", COUNT(b.levelid)"} FROM format1 AS a ${exclude ? "WHERE a.levelid NOT IN (SELECT levelid FROM known)" :
      "LEFT JOIN known AS b ON a.levelid = b.levelid"
    } GROUP BY a.levelid ORDER BY c DESC;`;
  return (await db.query({
    text: query,
    rowMode: "array"
  })).rows;
}

async function getUsers(levelId) {
  let query = `SELECT userident, COUNT(*) AS c FROM format1 WHERE levelid=$1 GROUP BY userident ORDER BY c DESC;`;
  return (await db.query({
    text: query,
    values: [levelId],
    rowMode: "array"
  })).rows;
}

async function updateLevels() {
  offset[0] = Math.max(0, selected[0] - 5);
  term.moveTo(0, 1);
  levels.slice(offset[0], offset[0] + term.height).forEach((l, i) => {
    let text = `${padRight(l[0].toString(), 10)} | ${padRight(l[1].toString(), 6)}`;
    let call = term;
    if (i + offset[0] == selected[0]) call = call.inverse;
    if (l[2] && l[2] != "0") call = call.brightGreen;
    call(text);
    term.moveTo(0, i + 2);
  });
  term("                   ");
}

async function updateUsers() {
  offset[1] = Math.max(0, selected[1] - 5);
  term.moveTo(22, 1);
  users.slice(offset[1], offset[1] + term.height).forEach((u, i) => {
    let text = `${u[0].slice(0, 8)} | ${padRight(u[1].toString(), 6)}`;
    if (i + offset[0] != selected[1]) term(text);
    else term.inverse(text);
    term.moveTo(22, i + 2);
  });
  term("                 ");
}

async function clearUsers() {
  term.moveTo(22, 2);
  Array(term.height).fill(0).forEach((_, i) => {
    term.moveTo(22, i + 2);
    term("                 ");
  });
}

term.on('key', function (key, matches, data) {
  switch (key) {
    case 'UP':
      selected[phase]--;
      phase ? updateUsers() : updateLevels();
      break;
    case 'DOWN':
      selected[phase]++;
      phase ? updateUsers() : updateLevels();
      break;
    case 'LEFT':
      if (phase) phase--;
      if (phase == 0) clearUsers();
      break;
    case 'ENTER':
    case 'RIGHT':
      if (!levels) break;
      getUsers(levels[selected[0]][0]).then(u => {
        users = u;
        phase++;
        selected[phase] = 0;
        updateUsers();
      });
      break;
    case 'y':
      let query = `INSERT INTO known VALUES ($1) ON CONFLICT DO NOTHING;`;
      db.query({
        text: query,
        values: [levels[selected[0]][0]],
        rowMode: "array"
      }).then(() => {
        if (exclude) levels.splice(selected[0], 1);
        else levels[selected[0]][2] = "1";
        updateLevels();
      });
      break;
    case 'e':
      exclude = !exclude;
    // intentionally no break
    case 'r':
      getLevels(exclude).then(l => {
        levels = l;
        phase = 0;
        selected[phase] = 0;
        updateLevels();
      });
      break;
    case 'q':
    case 'CTRL_C':
      term.clear();
      process.exit(0);
    default:
      break;
  }
});

(async () => {
  const setupQuery = "CREATE TABLE IF NOT EXISTS known (levelid SERIAL UNIQUE)";
  await db.query({ text: setupQuery });

  levels = await getLevels(exclude);
  updateLevels();
  // console.log(await getLevels(false));
})();