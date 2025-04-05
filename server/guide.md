<!-- 
  https://i.kym-cdn.com/photos/images/newsfeed/002/515/832/ee7.jpg 
  opens .md
  looks inside
  html

  no but the server just carries this over to the html being sent, and its much easier for me to write this guide in markdown than html all the way
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

# Creator's Guide <span style="font-size:50%">to [DeathMarkers](https://geode-sdk.org/mods/freakyrobot.deathmarkers)</span><img src="/banner.webp">

You've probably landed here because you are interested in using DeathMarkers to **improve** the gameplay/experience in the levels you create. **Great!** That's exactly what the mod was made to do.

As you (probably) know by now, this mod collects **every player's deaths** with a heck of a lot of information. With these statistics, you can find out more about how players experience your level and iron out any **unintended unfair/blind jumps**.

This stuff is not self-explanatory, though, which is why this document walks you through **the interface(s)** the mod offers to creators, as well as a guide on **interpreting** the data to make an impact on your level quality.

<div class="links">
<a target="_blank" href="https://github.com/MaSp005/deathmarkers"><img src="/github.webp"></a>
<a target="_blank" href="https://geode-sdk.org/mods/freakyrobot.deathmarkers"><img src="/geode.webp"></a>
<a target="_blank" href="https://discord.gg/hzDFNaNgCf"><img src="/discord.webp"></a>
</div>

## Table of contents

> [Last updated](https://github.com/MaSp005/deathmarkers/commits/main/server/guide.md): Mar 17, 2025<br>For mod version: v1.0.0

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

## General Tips

I am by no means a gameplay messiah, but the following are some tips for better gameplay I have picked up over time for creators. They intentionally do not use examples from real levels, in an attempt to provide tips that apply to a wider range of gameplay styles.

> Bear in mind, every level you create is yours to make decisions about. GD levels are an art form, there is no right or wrong when building them. However, level creation is also a practice of game design, and game design focuses on making players have fun. To make a successful level, both aspects need to have significant thought put into them. The following are some tips in the "fun gameplay" angle of level creation.

**Know how to handle gameplay issues.** If true issues in gameplay are found, creators often try to deal with them by making it auto, adding more safeguards or adding jump indicators everywhere. This is often referred to as "babying the player" and frowned upon. To properly rectify issues in your level, you'll need to know what is causing the spike in deaths. As the creator of a level, this is difficult to figure out, as you know everything about the level. Players, however, do not. Thus, they are much more prone to die when the upcoming gameplay is hard to identify. This can be due to a number of reasons, but most commonly:

- **Gameplay/Decoration is not distinguishable.** Some screen elements are not clearly identifiable as gameplay or decoration. Combatting this is dependent on the style of your level, but most importantly, gameplay elements and decoration should have a noticeable contrast in colour. (Also, jump indicators and spikes are to be distinguishable at a glance, even on their own before having seen the other.)

- **Unpredictable transitions.** When a transition from one part to another does not show upcoming gameplay until it is too late to react to it. Most of the time, this can be rectified by:

  - showing a simplified visual of the upcoming gameplay (e.g. when the screen goes dark, show the player and minimal information necessary to time the first input of the upcoming part),
  - drawing more attention to parts of the screen that contain the next bits of gameplay (e.g. keeping gameplay-deco contrast higher in the first few seconds of the new part), or
  - delaying input until the player has had enough time to evaluate the first input (e.g. in a ship section, placing a landing pad after the transition for the player to rest on).

- **J-Blocks.** If, after a very short black or blue orb landing, the player should *not* jump, but the fall was so short that the click from the orb is still ongoing when they land, they are likely frustrated if the landing surface does not prevent them from jumping with a J-Block. However, if the player *is* supposed to jump right after, and they are forced to perform two quick clicks due to a J-Block preventing them from holding through the second input, (which are invisible, by the way) you should either remove the J-Blocks, or (if you need the second click for sync) replace the subsequent jump with another kind of input.

- **Convoluted gameplay.** Most gameplay boils down to a simple pattern of inputs, like you would see in highway scrolling rhythm games. How this gameplay is portrayed and communicated in GD is the very core of gameplay creation. Gameplay is commonly considered "convoluted" if the player's movements are much more complicated than they need to be, for example, being thrown around multiple chains of jump pads between jumps or quickly changing directions and gamemode. Such designs obscure the actual required inputs and make the level harder to understand, let alone sightread. Many creators end up placing more and more obvious jump indicators to dictate the player's inputs directly, while the gameplay structures themselves lose relevance. Of course, higher energy songs require higher energy gameplay and, thus, quicker and more complex movements. To remedy such problems, you can, of course, simplify the gameplay, OR create supporting indicators like path of flight highlighters which help with understanding the path a player will take in cases where the structuring itself is not clear about this.

**Do not take death markers too seriously.** People dying to a particular spot may be due to a wide range of reasons, some out of your control. As of now, the mod does not have the right features to allow you to inspect possible reasons for a given high death count location (though it will in the future). Deaths may be due to anything. Use your best judgement for whether you want to address them or not. **Most importantly: Listen to player feedback.**

> This section always open for discussion on the [discord](https://discord.gg/ZsXeKEwZqA).

</main>

> **LEGAL STUFF**<br>See [my homepage](https://masp005.dev/legal) regarding hosting and contact.<br>This page does not collect any data.<br>As for the mod it promotes, see the [GitHub repository](https://github.com/MaSp005/deathmarkers).
<script src="/zoom.js"></script>
</body></html>