const DATABASE = require("../config.json").DATABASE.POSTGRES;
const { Pool } = require("pg");

const db = new Pool(DATABASE);

try {
  (async () => {
    const setupQuery = require("fs").readFileSync('./database/postgres-schema.sql', 'utf8');
    await db.query({ text: setupQuery });
  })();
} catch (e) {
  console.error("Error preparing database:");
  console.error(e);
  process.exit(1);
}

module.exports = {

  list: async (levelId, isPlatformer) => {

    let columns = isPlatformer ? "x,y" : "x,y,percentage";
    let query = isPlatformer ?
      `SELECT ${columns} FROM format1 WHERE levelid == $1 UNION
    SELECT ${columns} FROM format2 WHERE levelid == $1` :
      `SELECT ${columns} FROM format1 WHERE levelid == $1 AND percentage < 101 UNION
    SELECT ${columns} FROM format2 WHERE levelid == $1 AND percentage < 101;`;

    return {
      deaths: (await db.query({
        text: query,
        values: [levelId],
        rowMode: "array"
      })).rows,
      columns
    };

  },

  analyze: async (levelId, columns) => {

    return await (db.query({
      text: `SELECT ${columns} FROM format1 WHERE levelid = $1;`,
      values: [levelId],
      rowMode: "array"
    })).rows;

  },

  register: async (format, data) => {

    switch (format) {
      case 1:
        await db.query({
          text: "INSERT INTO format1 VALUES ($1, $2, $3, $4, $5, $6, $7)",
          values: [data.userident, data.levelid, data.levelversion, data.practice, data.x, data.y, data.percentage]
        });
        break;
      case 2:
        await db.query({
          text: "INSERT INTO format2 VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)",
          values: [data.userident, data.levelid, data.levelversion, data.practice, data.x, data.y, data.percentage, data.coins, data.itemdata]
        });
        break;
    }

  }

}