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

# Creator's Guide <span style="font-size:50%">to [DeathMarkers](https://github.com/MaSp005/deathmarkers)</span><img src="/banner.webp">

You've probably landed here because you are interested in using DeathMarkers to **improve** the gameplay/experience in the levels you create. **Great!** That's exactly what the mod was made to do.

As you (probably) know by now, this mod collects **every player's deaths** with a heck of a lot of information. With these statistics, you can find out more about how players experience your level and iron out any **unintended unfair/blind jumps**.

This stuff is not self-explanatory, though, which is why this document walks you through **the interface(s)** the mod offers to creators, as well as a guide on **interpreting** the data to make an impact on your level quality.

<div class="links">
<a target="_blank" href="https://github.com/MaSp005/deathmarkers"><img src="https://raw.githubusercontent.com/geode-sdk/geode/faedd885374b45d649913eafa8ac7461eb2815b2/loader/resources/github.png"></a>
<a target="_blank" href="https://geode-sdk.org/mods/freakyrobot.deathmarkers"><img src="https://raw.githubusercontent.com/geode-sdk/geode/faedd885374b45d649913eafa8ac7461eb2815b2/loader/resources/logos/geode-logo.png"></a>
<a target="_blank" href="https://discord.gg/hzDFNaNgCf"><img src="/discord.webp"></a>
</div>

## Table of contents

> Last updated: Feb 13, 2025<br>For mod version: v1.0.0

<?>TOC

## Interface

![Death Marker button in the editor pause menu](/editor-pause-button.webp)

In the editor's pause menu, you'll find a button on the bottom row with the death marker icon. When hitting it, the mod will load all available deaths from the loaded level. A popup will appear, telling you how many deaths were fetched.

Be wary, it uses the level's saved online ID, so if you copy a copied level, it only takes the **original** level ID and makes the deaths of the actual level you copied inaccessible. In such cases, only the creator of the level can view its deaths. (I can't do anything about this, it's how GD stores level IDs)

![Demonstration of death markers in the editor of Deadlocked, with several groups indicating some of them being in close vicinity](/editor-markers-groups.webp)

Once loaded, you'll see every death rendered in your level, with some group circles highlighting stacks of deaths in close proximity. These groups include more or less deaths when you zoom out or in. They make it easier to see when many deaths lay on the exact same spot, and give you a sense of scale when many deaths occur in a small vicinity.

<!-- TODO: Abandon Vocab section, make entire sections for each topic directly
## Vocabulary

For the rest of the guide, we'll need to lay some ground rules about **vocabulary**. How each of these work together will be discussed later. This list is grouped by thematic relevance.

A **death** is the event of a player dying to an obstacle in the level. A **death location** is the location of that death and a **death marker** consists of said location along with other data. These are collected and can be displayed.

![Explanatory graphic on new/matched bests and setbacks](/bests-setbacks.webp)

A **new best** is a death in which the player reached their new highest progress. This implies that they have never seen that location before. A **matched best** is a death in which the player dies in the vicinity of their current best. 

A **setback** is a death in which the player dies significantly earlier than their current best. Setbacks can be split into two groups: **new setbacks** are the first time that a player has died at that location, but they have previously beat it the first time they encountered it. **Old setbacks** are players dying to a location which was a new best (or new setback) previously, despite having passed it before.

**Sightreading** is the practice of entirely (and confidently) predicting the upcoming gameplay by sight (or rhythmic anticipation) alone. Blind button spamming with no understanding of the gameplay is not sightreading. **Blind Jumps** are required inputs that are not reasonably able to be sightread, e.g. a jump instantly after landing a long fall or a transition that does not give enough time to examine the gameplay coming up.

![Overlay of two paths across a green-orb chain, demonstrating that the timing on the first orb impacts the trajectory and causing a sequence to be impossible](/anticipated-jumps.webp)

**Anticipated jumps** are a combination of inputs in which the first input can be hit at any timing with no direct consequences, and will still allow the second input to happen, but limit the timing window to one where death is guaranteed. These combinations can be of any length, where a slightly missed timing on the first input guarantees death at least by the last input.

-->

## Upcoming

As of now, the mod only (but eagerly) *tracks* a lot of information about player deaths, but does not yet display it to creators. This is being actively worked on, but I want you to enjoy the benefits of seeing just death locations, as they are right now, first!

If you want to help make this whole dream a bit more of a reality, either by contributing directly or by helping iron out issues, be sure to take a look at the [GitHub repository](https://github.com/MaSp005/deathmarkers) of the project. <span class="love"></span>

</main>

> **LEGAL STUFF**<br>See [my homepage](https://masp005.dev/legal) regarding hosting and contact.<br>This page does not collect any data.<br>As for the mod, see the [GitHub repository](https://github.com/MaSp005/deathmarkers).
<script src="/zoom.js"></script>
</body></html>