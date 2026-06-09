# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Full OBJ mesh files support: triangular and non-triangular meshes (stored as triangular), UV map textures
- Add scene files .txt
- Restructuring of main
- Parser implementation
- Lexer implementation
- OBJ mesh files support for points and normals only (no texture)
- Mesh shape, optimized via BVH-SAH

--------

## [0.3.0] - 2026-05-20

### Added
- Rendering algorithm Pathtracer
- Antialiasing, added flag `--antialiasing` to activate and choose the level
- Cube shape
- SpecularBRDF, DiffusiveBRDF
- Added flag `--algorithm` to choose between OnOffRenderer or FlatRenderer
- Renderer struct with OnOffRenderer and FlatRenderer
- Material, BRDF, Pigments (Image, Checkered, Uniform) struct
- PCG random generator algorithm
- Refined main architecture

## [0.2.0] - 2026-05-06

### Added
- Command line interface with `pfm2png` and `demo` subcommands in main
- `Shape` (Sphere, Plane) and `World` structs for scene management

### Fixed
- Bug in `ImageTracer::fire_ray` [#5]

## [0.1.0] - 2026-04-01

### Added
- First release of the code