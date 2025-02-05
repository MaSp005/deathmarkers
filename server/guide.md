<!-- 
  https://i.kym-cdn.com/photos/images/newsfeed/002/515/832/ee7.jpg 
  opens .md
  looks inside
  html

  no but the server.js just carries this over to the html being sent, and its much easier for me to write this guide in markdown than html all the way
-->
<!DOCTYPE html>
<html><head>
  <title>DeathMarkers Creator Guide</title>
  <link rel="stylesheet" href="/style.css">
  <meta charset="utf-8">
  <meta http-equiv="X-UA-Compatible" content="IE=edge">
  <meta name="description" content="">
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head><body>
<main>

# Creator's Guide <span style="font-size:50%">to [DeathMarkers](https://github.com/MaSp005/deathmarkers)</span><img src="/banner.png">
You've probably landed here because you are interested in using DeathMarkers to **improve** the gameplay/experience in the levels you create. **Great!** That's exactly what the mod was made to do.

As you (probably) know by now, this mod collects **every player's deaths** with a heck of a lot of information. With these statistics, you can find out more about how players experience your level and iron out any **unintended unfair/blind jumps**.

This stuff is not self-explanatory, though, which is why this document walks you through **the interface(s)** the mod offers to creators, as well as a guide on **interpreting** the data to make an impact on your level quality.

## Table of contents

> Last updated: Dec 18, 2024<br>Changes: uh

<?>TOC

## Interface

## Vocabulary

For the rest of the guide, we'll need to lay some ground rules about **vocabulary**. How each of these work together will be discussed later. This list is grouped by thematic relevance.

A **death** is the event of a player dying to an obstacle in the level. A **death location** is the location of that death and a **death marker** consists of said location along with other data. These are collected and can be displayed.

![Explanatory graphic on new/matched bests and setbacks](front/bests-setbacks.webp)

A **new best** is a death in which the player reached their new highest progress. This implies that they have never seen that location before. A **matched best** is a death in which the player dies in the vicinity of their current best. 

A **setback** is a death in which the player dies significantly earlier than their current best. Setbacks can be split into two groups: **new setbacks** are the first time that a player has died at that location, but they have previously beat it the first time they encountered it. **Old setbacks** are players dying to a location which was a new best (or new setback) previously, despite having passed it before.

**Sightreading** is the practice of entirely (and confidently) predicting the upcoming gameplay by sight (or rhythmic anticipation) alone. Blind button spamming with no understanding of the gameplay is not sightreading. **Blind Jumps** are required inputs that are not reasonably able to be sightread, e.g. a jump instantly after landing a long fall or a transition that does not give enough time to examine the gameplay coming up.

**Anticipated jumps** are a combination of inputs in which the first input can be hit at any timing with no direct consequences, and will still allow the second input to happen, but limit the timing window to one where death is guaranteed. These combinations can be of any length, where a slightly missed timing on the first input guarantees death at least by the last input.

<!-- TODO: Explanatory graphic on Anticipated Jumps (maybe even a gif) -->

</main>

> **LEGAL STUFF**<br>See [my homepage](https://masp005.dev/legal) regarding hosting and contact. This page does not collect any data. As for the mod, see the [GitHub repository](https://github.com/MaSp005/deathmarkers).
</body></html>