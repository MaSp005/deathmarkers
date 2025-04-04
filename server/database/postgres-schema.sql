CREATE TABLE IF NOT EXISTS format1 (
  userident CHAR(40) NOT NULL,
  levelid SERIAL NOT NULL,
  levelversion SMALLINT DEFAULT 0,
  practice BOOLEAN DEFAULT false,
  x FLOAT NOT NULL,
  y FLOAT NOT NULL,
  percentage SMALLINT NOT NULL
);

CREATE TABLE IF NOT EXISTS format2 (
  userident CHAR(40) NOT NULL,
  levelid INTEGER NOT NULL,
  levelversion SMALLINT DEFAULT 0,
  practice BOOLEAN DEFAULT false,
  x FLOAT NOT NULL,
  y FLOAT NOT NULL,
  percentage SMALLINT NOT NULL,
  coins SMALLINT DEFAULT 0,
  itemdata DOUBLE PRECISION DEFAULT 0
);

CREATE INDEX IF NOT EXISTS format1index ON format1 (levelid);
CREATE INDEX IF NOT EXISTS format2index ON format2 (levelid);