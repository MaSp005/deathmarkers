This document explains the data formats of server communication and the database.

# Formats

The system works with format numbers, starting from 1 as the first version. They differentiate themselves by the organization and quantity of features that can be stored with them.

- **Format 1**
  - Stores: `userident`, `levelid`, `levelversion`, `practice`, `x`, `y`, `percentage`
  - Sends for list: `x`, `y`, `percentage`
  - Sends for analysis: `userident`, `levelversion`, `practice`, `x`, `y`, `percentage`

- **Format 2**
  - Additionally Stores: `coins`, `itemdata`
  - Additionally sends for analysis: `coins`, `itemdata`
  - Currently unused by the mod, but available

> Format 2 is accepted for submissions and treated the same as format 1 for listing, but ignored for analysis

# Database

The server works with [`better-sqlite3`](https://www.npmjs.com/package/better-sqlite3) and thus has only one database and no login.

The database contains one table per format version, in order to store the corresponding incoming deaths in the correct version table, and sending deaths combined from all formats.

## Table `format1`

| Column Name | Data Type | Constraints | Description |
|-|-|-|-|
| userident | CHAR(40) | NOT NULL | A unique identification of player & level. See [§ userident](#userident) |
| levelid | INT | UNSIGNED NOT NULL | The id of the level.
| levelversion | TINYINT | UNSIGNED DEFAULT 1 | Version number of the level. |
| practice | BIT | DEFAULT 0 | If the death was done in practice. |
| x | FLOAT(10,2) | NOT NULL | The x-position of the player at the time of death. |
| y | FLOAT(10,2) | NOT NULL | The y-position of the player at the time of death. |
| percentage | SMALLINT | UNSIGNED NOT NULL | For normal levels, the percentage of the player 0-100, and 101 for a level finish.<br>For platformer levels, the time of death in seconds.

## Table `format2`

| Column Name | Data Type | Constraints | Description |
|-|-|-|-|
| ... | ... | ... | Identical to `format1`, with the addition of: |
| coins | TINYINT | DEFAULT 0 | Bitfield:<br>`1 << 0` = 1st coin,<br>`1 << 1` = 2nd coin,<br>`1 << 2` = 3rd coin. |
| itemdata | DOUBLE | DEFAULT 0 | The value of item 0 at the time of death. Can be used by level creators to encode specific information about the player. |

When a player finishes the level, an entry with `percentage = 100` is added. Although this is not a death in the sense of the word, it serves an analytical purpose for detecting if a given player (based on `userident`) has ended up beating the level, as well as discovering potential unintended secret ways.

## Indexes

To speed up queries for specific levels, the database also employs an index for every table:

`CREATE INDEX IF NOT EXISTS format1index ON format1 (levelid);`
`CREATE INDEX IF NOT EXISTS format1index ON format2 (levelid);`

## `userident`

The `userident` is used to group together deaths from an individual player playing a specific level. This is anonymized in order to make grouping across levels difficult. The three identification components are: **Account Name**, **Level ID**, **User ID**, which are arranged as `[account name]_[user id]_[level id]` and then hashed using the SHA-1 algorithm.

> Yes SHA-1 has been [cryptographically broken](https://blog.mozilla.org/security/2017/02/23/the-end-of-sha-1-on-the-public-web/) and is vulnerable to certain attacks, but come on, this is a fucking death collection mod. Who cares. It also has the smallest output size and storage space is limited. Unless this proves to become an actual danger, it stays.

Since the `userident` is stored with each death and could easily be computed to figure out a specific player's deaths (though not the other way), upon sending the deaths for analysis (when the `userident`s would be exposed), they are **hashed again**, this time with a random **salt** generated per analysis. As a result, the userident stays consistent at grouping together a single player's death, but nobody can identify a specific player's death.

> The server accepts either account name and user id to be sent OR a complete and proper userident. Geometry Dash and Geode cannot natively hash arbitrary strings, and - in my eyes - a dependency for it is unnecessary. The server creates the userident and immediately forgets about the user's data.

### Example

- Player: **RobTop** (User-ID **16**)
- Level: Bloodbath, ID **10565740**

`[account name]_[user id]_[level id]`
⇒ `RobTop_16_10565740`
⇒ `cba4a35e4ee458178b18d4c8ebb836a518b4df4b` ← This is the way it is stored in the database

**Upon analysis:**
Select salt: e.g. `ZqQhF28asA` <- Different every time a level analysis is requested
⇒ `cba4a35e4ee458178b18d4c8ebb836a518b4df4b_ZqQhF28asA`
⇒ `aed5ab073efad9fe738eacb2bebeb174b2ceae6b` ← This is what is sent from /analysis

### But what about people playing without an account?

Not getting in the database. That's it. The mod won't even try to submit anything if it knows that there is no account.

# API

**Base URL**: `https://deathmarkers.masp005.dev/`

Every request and response body is in the format **JSON** (MIME-Type `application/json`).

## GET `/list`

Parameter(s):
- `levelid`: The ID of the level requested.
- `platformer` (boolean): If `false`, ignores entries with percentage > 100. See [§ Table DEATHS](#table-format1).
- Optional: `response`: responds using specified data format, `csv` is the default and only allowed value

Delivers (`text/csv`):
> `x,y,percentage`
> `float,float,int`
> `...`

This endpoint is intended for a regular playthrough to display Mario Maker-style death pins and a bar graph on the percentage.

## GET `/analysis`

Parameter(s):
- `levelid` (string): The ID of the level requested.
- Optional: `response`: responds using specified data format, `csv` is the default and only allowed value

Delivers (`text/csv`):
> `userident,levelversion,practice,x,y,percentage`
> `string,int,0 or 1,float,float,int`
> `...`

See [§ Table DEATHS](#table-format1) for information on each property.

This endpoint is intended for level creators to analyze the deaths in their level to improve the gameplay. It is not restricted to the actual level creator to allow anyone to learn from others' levels.

## POST `/submit`

This parameter in particular is not meant for public access. It should only be used by the mod and is only documented for contributors.

Body Data (`application/json`):
- `format`: (int): Format Version Number, for the following always 1.
- `levelid` (string): The ID of the level requested.
- Optional: `levelversion` (int): Version number of the level. 0 by default.
- Optional: `practice` (bool): Whether the death occurred in practice mode. `false` by default.
- One of:
  1. `userident` (string): See [§ userident](#userident)
  2. - `playername` (string): Player Name
     - `userid` (int): Player's User ID
     The hash will be calculated by the server.
- `x` (float): x-position
- `y` (float): y-position
- `percentage` (int): For normal levels, the percentage of the player 0-100. For platformer levels, the time of death in seconds.
<!--
- Optional: One of:
  1. `coins` (int): Integer Bitfield, see above.
  2. - `coin1` (boolean): Whether coin 1 was collected.
     - `coin2` (boolean): Whether coin 2 was collected.
     - `coin3` (boolean): Whether coin 3 was collected.
     Each of these are optional, assuming `false` if omitted.
- Optional: `itemdata` (double): Value of Item ID 0.
-->

Delivers: `400`, `500` or `204` Status. `400` responses supply a human-readable error source.