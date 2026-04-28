# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

This document only concerns the changelog of the **Mod**, not the server.

## [1.5.5] - 2026-04-28

### Changed

- Upgrade to Geode v5.6.1
- Changed the default Server URL to `https://deathmarkers.lol/`
  - Automatically migrate if old one is still set

## [1.5.4] - 2026-02-16

### Changed

- Upgrade to Geode 5.0.0 (GD 2.2081)

## [1.5.3] - 2025-10-08

### Changed

- Upgrade to Geode v4.9.0

### Fixed

- No longer crash after 30 seconds if submission of deaths fails. ([#12](https://github.com/MaSp005/deathmarkers/issues/12))

## [1.5.2] - 2025-09-19

### Fixed

- No longer crash on death when using "always show" and "normal only" in practice mode ([#10](https://github.com/MaSp005/deathmarkers/issues/10))
- Suppress warning about changed settings when markers will never show
- No longer save local deaths to "0" file on local levels

## [1.5.1] - 2025-09-06

### Changed

- Cross-platform support fix

### Fixed

- Controls on "Darkener in Editor" setting are now as intended
- Markers no longer render as ghosts if the setting is disabled

## [1.5.0] - 2025-09-01

### Added

- Option for Ghost Cubes ([#9](https://github.com/MaSp005/deathmarkers/issues/9))
  - These only work in local mode and only start usage upon the setting being enabled

### Changed

- Upgrade to Geode v4.8.0
- Calculate your identification on the client

### Fixed

- The first known death of a level now renders correctly
- All markers are now drawn when changing to "Always show" while only a limited number are shown at the moment

## [1.4.0] - 2025-08-12

### Added

- New option variant of local deaths, only activating if playing a demon level
- New options for customizing when the game shall draw markers
- Settings button in the pause menu
- Option to show markers when the game is paused

### Changed

- Upgrade to Geode v4.7.0
- Use Geode's BasedButton for the editor button (allows generic retexturing)
- Properly handle 429 submission response, extend retry time & count

### Fixed

- Marker of the latest death is now properly highlighted, even if markers are shown all the time
- Practice mode is now properly submitted for each death

## [1.3.2] - 2025-06-11

### Changed

- Batch submissions

### Fixed

- Markers turning upside-down in mirror portals

## [1.3.1] - 2025-05-25

### Changed

- Made spam warning scrollable to prevent it from blocking the screen ([#6](https://github.com/MaSp005/deathmarkers/pull/6))

## [1.3.0] - 2025-05-25

### Added

- Ability to hide practice mode deaths ([#5](https://github.com/MaSp005/deathmarkers/issues/5))
- Setting to adjust API URL
- Spam prevention and warning within the mod

## [1.2.1] - 2025-05-04

### Changed

- Upgrade to Geode v4.4.0
- No longer render out-of-view markers

## [1.2.0] - 2025-05-03

### Added

- macOS support ([#3](https://github.com/MaSp005/deathmarkers/pull/3))
- iOS support
- Option to load deaths from local storage instead of the server

### Changed

- Make markers rotate and zoom with the camera

## [1.1.0] - 2025-04-23

### Added

- Adjustable darkener in the editor
- Ability to show deaths only on a new best
- Setting to adjust fade-in time of markers

### Changed

- Reorganize Mod's settings menu

### Fixed

- Fix deaths not being submitted in some circumstances
- Fix the histogram sometimes not showing at level start despite "Always show" being enabled
- Fix drawing states in practice with "Always show" enabled

## [1.0.3] - 2025-04-20

### Fixed

- Fix submitting deaths and rendering markers when Globed players die. ([#2](https://github.com/MaSp005/deathmarkers/issues/2))

### Changed

- Adjust mid-range colors on death histogram to be more saturated

## [1.0.2] - 2025-04-20

### Fixed

- Fix the game crashing when the editor pause menu is opened a second time if Relocate Build Tools is not installed ([#1](https://github.com/MaSp005/deathmarkers/issues/1))

## [1.0.1] - 2025-04-18

### Changed

- Fetching Deaths now uses a faster method of transmission

## [1.0.0] - 2025-03-17

- Initial release featuring:
  - Online Death collection
  - Markers & Histogram in-game
  - Simple Markers & Groups in Editor
