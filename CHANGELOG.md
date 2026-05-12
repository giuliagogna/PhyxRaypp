# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Added flag `--algorithm` to choose between OnOffRenderer or FlatRenderer
- Renderer struct with OnOffRenderer and FlatRenderer
- Material struct
- BRDF struct
- Image Pigment
- Checkered Pigment
- Uniform Pigment

## [0.2.0] - 2026-05-06

### Added
- Command line interface with `pfm2png` and `demo` subcommands in main
- `Shape` (Sphere, Plane) and `World` structs for scene management

### Fixed
- Bug in `ImageTracer::fire_ray` [#5]

## [0.1.0] - 2026-04-01

### Added
- First release of the code