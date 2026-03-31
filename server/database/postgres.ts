import { Pool } from "pg";
import format from "pg-format";
import { Database, DeathData, Format, LevelID, MinDeathData, SubmissionMetadata, ValueOf } from "./interface";

const {
  DATABASE_HOST,
  DATABASE_NAME,
  DATABASE_USER,
  DATABASE_PASSWORD,
} = process.env;

if (!DATABASE_HOST || !DATABASE_NAME || !DATABASE_USER || !DATABASE_PASSWORD) {
  console.error("Database connection data not declared in environment");
  process.exit(1);
}

const db = new Pool({
  user: DATABASE_USER,
  database: DATABASE_NAME,
  password: DATABASE_PASSWORD,
  host: DATABASE_HOST,
});

try {
  const setupQuery = require("fs").readFileSync('./database/postgres-schema.sql', 'utf8');
  await db.query({ text: setupQuery });
} catch (e) {
  console.error("Error preparing database:");
  console.error(e);
  process.exit(1);
}

export default class implements Database {

  async list<P extends boolean>(levelId: LevelID, isPlatformer: P, inclPractice: boolean) {

    // @ts-ignore
    const columns: P extends true ? "x,y" : "x,y,percentage" = isPlatformer ? "x,y" : "x,y,percentage";
    const where = "WHERE levelid = $1"
      + (isPlatformer ? " AND percentage < 101" : "");
    const query = `SELECT ${columns} FROM format1 ${where}${inclPractice ? "" : " AND practice = false"} ` +
      `UNION SELECT ${columns} FROM format2 ${where}${inclPractice ? "" : " AND practice = false"};`;

    return {
      deaths: (await db.query<ValueOf<MinDeathData>[], [number]>({
        text: query,
        values: [levelId],
        rowMode: "array"
      })).rows,
      columns
    };

  }

  async analyze<F extends Format>(levelId: LevelID, columns: string) {

    return (await db.query<ValueOf<DeathData<F>>[], [number]>({
      text: `SELECT ${columns} FROM format1 WHERE levelid = $1;`,
      values: [levelId],
      rowMode: "array"
    })).rows;

  }

  async register<F extends Format>(metadata: Pick<SubmissionMetadata, "userident" | "levelid" | "levelversion" | "format"> & { format: F }, deaths: DeathData<F>[]) {

    if (deaths.length == 0) return;

    let values = deaths.map(obj => (
      [
        metadata.userident,
        metadata.levelid,
        metadata.levelversion,
        !!obj.practice,
        obj.x,
        obj.y,
        obj.percentage
      ].concat(metadata.format == 2 ? [
        (obj as DeathData<2>).coins,
        (obj as DeathData<2>).itemdata
      ] : [])
    ));

    // format is checked by caller, can be safely included here
    let query = format(`INSERT INTO format${metadata.format} VALUES %L`, values);
    await db.query(query);

  }

}
