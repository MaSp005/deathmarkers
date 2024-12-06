This document explains the data formats of server communication and the database.

# Database

The server works with [`better-sqlite3`](https://www.npmjs.com/package/better-sqlite3) and thus has only one database and no login.

## Table `DEATHS`

| Column Name | Data Type | Constraints | Description |
|-|-|-|-|
| userident | CHAR(40) | NOT NULL | A unique identificator of player & level. See [§ userident](#userident) |
| levelid | INT | UNSIGNED NOT NULL | The id of the level.
| levelversion | TINYINT | UNSIGNED DEFAULT 0 | Version number of the level. |
| practice | BOOLEAN | DEFAULT 0 | If the death was done in practice. |
| x | FLOAT(10,2) | NOT NULL | The x-position of the player at the time of death. |
| y | FLOAT(10,2) | NOT NULL | The y-position of the player at the time of death. |
| percentage | SMALLINT | UNSIGNED NOT NULL | For normal levels, the percentage of the player 0-100.<br>For platformer levels, the time of death in seconds.
| coins | TINYINT | DEFAULT 0 | Bitfield:<br>`1 << 0` = 1st coin,<br>`1 << 1` = 2nd coin,<br>`1 << 2` = 3rd coin. |
| itemdata | DOUBLE | DEFAULT 0 | The value of item 0 at the time of death. Can be used by level creators to encode specific information about the player. |

When a player finishes the level, an entry with `percentage = 100` is added. Although this is not a death in the sense of the word, it serves an analytical purpose for detecting if a given player (based on `userident`) has ended up beating the level, as well as discovering potential unintended secret ways.

## `userident`

The `userident` is used to group together deaths from an individual player playing a specific level. This is anonymized in order to make grouping across levels difficult.

> Since the level id is known for a specific userident, it is possible to brute-force to figure out the username that was encoded. If we add another parameter to the identificator, said parameter can in turn be brute-forced if the player name is known (e.g. from tracking down a specific death entry of a streamer), so we cannot use sensitive data for it. Instead, we'll add the *User ID* of the player. Although this means it can still be brute-forced, it is infinitely more tedious than trying every possible string of letters for the account name, as the Username and ID have to match up and be queried from the GD Servers to create a valid `userident`.

In total, the three identification components are: **Account Name**, **Level ID**, **User ID**.

The arrangement of the three depends on the numerical value of the **User ID modulo 6**.

Acc-ID mod 6 | Arrangement
-|-
0 | `[account name]_[level id]_[user id]`
1 | `[account name]_[user id]_[level id]`
2 | `[user id]_[level id]_[account name]`
3 | `[user id]_[account name]_[level id]`
4 | `[level id]_[user id]_[account name]`
5 | `[level id]_[account name]_[user id]`

### Example

- Player: **RobTop** (Acc-ID **71**)
- Level: Bloodbath, ID **10565740**

`71 % 6 = 5`
⇒ `[level id]_[account name]_[account id]`
⇒ `10565740_RobTop_71`
⇒ `a64d21f4231096a3e0064a84f1ebf5567a40258d`

### But what about people playing without an account?

Not getting in the database. That's it. The mod won't even try to submit anything if it knows that there is no account.

## API

**Base URL**: `https://deathmarkers.masp005.dev/`

Every request and response body is in the format **JSON**.

### GET `/list`

Parameter(s):
- `levelid`: The ID of the level requested.
- Optional: `levelversion` (int): Filters to only the specified version if provided.

Delivers: `[[float, float], [...], ...]` (Pairs of [x, y] coordinates)

Example: `https://deathmarkers.masp005.dev/list?levelid=10565740`
⇒ `[[52.12, 9241.23], ...]`

This endpoint is intended for a regular playthrough to display Mario Maker-style death pins.

### GET `/analysis`

Parameter(s):
- `levelid` (string): The ID of the level requested.
- Optional: `levelversion` (int): Filters to only the specified version if provided.

Delivers: `[{"userident":string, "levelversion":int, "practice":bool, "x":float, "y":float, "percentage":int, "coins":int, "itemdata": double}, {...}]`
See [§ Table DEATHS](#table-deaths) for information on each property.

Example: `https://deathmarkers.masp005.dev/analysis?levelid=10565740`
⇒ `[{"userident":"89a7s2...", "levelversion":2, "practice":true, "x":52.12, "y":9241.23, "percentage":12, "coins":0, "itemdata": 0},...]`

This endpoint is intended for level creators to analyse the deaths in their level to improve the gameplay. It is not restricted to the actual level creator to allow anyone to learn from others' levels

### POST `/submit`

> This parameter in particular is not meant for public access. It should only be used by the mod and is only documented for contributors.

Body Data:
- `levelid` (string): The ID of the level requested.
- Optional: `levelversion` (int): Version number of the level. Assumed to be 0 if omitted.
- Optional: `practice` (bool): Whether you were in practice. Assumed to be false if omitted.
- One of:
  1. `userident` (string): See [§ userident](#userident)
  2. - `playername` (string): Player Name
     - `userid` (int): Player's User ID
     The hash will be calculated by the server.
- `x` (float): x-position
- `y` (float): y-position
- `percentage` (int): For normal levels, the percentage of the player 0-100. For platformer levels, the time of death in seconds.
- Optional: One of:
  1. `coins` (int): Integer Bitfield, see above.
  2. - `coin1` (boolean): Whether coin 1 was collected.
     - `coin2` (boolean): Whether coin 2 was collected.
     - `coin3` (boolean): Whether coin 3 was collected.
     Each of these are optional, assuming `false` if omitted.
- Optional: `itemdata` (double): Value of Item ID 0.

Delivers: `400`, `500` or `204` Status. `400` responses supply a human-readable error source.