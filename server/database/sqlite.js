const DATABASE_FILENAME = require("../config.json").DATABASE.FILENAME;

const db = require("better-sqlite3")(DATABASE_FILENAME);

try {
  db.transaction(() => {
    db.exec(require("fs").readFileSync('schema.sql', 'utf8'));
  })();
} catch (e) {
  console.error("Error preparing database:");
  console.error(e);
  process.exit(1);
}

module.exports = {

  list: (levelId, isPlatformer) => {

    let columns = isPlatformer ? "x,y" : "x,y,percentage";
    let query = isPlatformer ?
      `SELECT ${columns} FROM format1 WHERE levelid == @levelId UNION
    SELECT ${columns} FROM format2 WHERE levelid == @levelId` :
      `SELECT ${columns} FROM format1 WHERE levelid == @levelId AND percentage < 101 UNION
    SELECT ${columns} FROM format2 WHERE levelid == @levelId AND percentage < 101;`;

    return {
      deaths: db.prepare(query)
        .raw(true)
        .all({ levelId }),
      columns
    };

  },

  analyze: (levelId, columns) => {

    return db.prepare(`SELECT ${columns} FROM format1 WHERE levelid = ?;`)
      .raw(true).all(levelId);

  },

  register: (format, data) => {

    switch (format) {
      case 1:
        db.prepare("INSERT INTO format1 VALUES (@userident, @levelid, @levelversion, @practice, @x, @y, @percentage)")
          .run(data);
        break;
      case 2:
        db.prepare("INSERT INTO format2 VALUES (@userident, @levelid, @levelversion, @practice, @x, @y, @percentage, @coins, @itemdata)")
          .run(data);
        break;
    }

  }

}