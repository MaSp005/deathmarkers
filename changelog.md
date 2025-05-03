# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

This document only concerns the changelog of the **Mod**, not the server.

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

- Fix the game crashing when the editor pause menu is opened a second time if Relocate Build Tools is not installed. ([#1](https://github.com/MaSp005/deathmarkers/issues/1))

## [1.0.1] - 2025-04-18

### Changed

- Fetching Deaths now uses a faster method of transmission

## [1.0.0] - 2025-03-17

- Initial release featuring:
  - Online Death collection
  - Markers & Histogram in-game
  - Simple Markers & Groups in Editor
