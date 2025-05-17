const DATABASE_FILENAME = require("../config.json").DATABASE.FILENAME;

const db = require("better-sqlite3")(DATABASE_FILENAME);

try {
  db.transaction(() => {
    db.exec(require("fs").readFileSync('./database/sqlite-schema.sql', 'utf8'));
  })();
} catch (e) {
  console.error("Error preparing database:");
  console.error(e);
  process.exit(1);
}

module.exports = {

  list: async (levelId, isPlatformer, inclPractice) => {

    let columns = isPlatformer ? "x,y" : "x,y,percentage";
    let where = "WHERE levelid = @levelId"
      + (isPlatformer ? " AND percentage < 101" : "")
      + (inclPractice ? "" : " AND practice = 0");
    let query = `SELECT ${columns} FROM format1 ${where} ` +
      `UNION SELECT ${columns} FROM format2 ${where};`;

    return {
      deaths: db.prepare(query)
        .raw(true)
        .all({ levelId }),
      columns
    };

  },

  analyze: async (levelId, columns) => {

    return db.prepare(`SELECT ${columns} FROM format1 WHERE levelid = ?;`)
      .raw(true).all(levelId);

  },

  register: async (format, data) => {

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