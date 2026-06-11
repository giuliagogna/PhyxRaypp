# PhyxRadpp

PhyxRadpp is a C++23 ray tracer and HDR image processing utility developed as part of the Numerical Simulation for Physics course.

The project provides two main functionalities:

* **HDR image processing**, including conversion from the Portable Float Map (`.pfm`) format to standard LDR image formats such as PNG.
* **Physically-based rendering**, supporting both simple geometric primitives and complex polygonal meshes loaded from OBJ files.

## Main Features

### HDR Image Processing

PhyxRadpp can convert high dynamic range images stored in the `.pfm` format into displayable PNG images by applying:

* image normalization;
* dynamic range compression;
* gamma correction.

### Scene Rendering

Scenes are described through a custom text-based Scene Description Language and rendered at runtime.

The renderer currently supports:

* spheres, planes, cubes and other mathematical primitives;
* triangular meshes imported from OBJ files;
* texture mapping through UV coordinates;
* configurable materials and BRDFs;
* composable geometric transformations;
* acceleration structures based on Bounding Volume Hierarchies (BVH);
* antialiasing implementation to avoid visual artifacts like Moiré fringes.

Multiple rendering algorithms are available:

* **OnOff Renderer** — binary visibility testing;
* **Flat Renderer** — direct shading without light transport simulation;
* **Path Tracer** — Monte Carlo global illumination with Russian Roulette termination.

## Documentation Guide

### Source Code Documentation

This documentation primarily focuses on the internal architecture of the project.

The documented modules include:

* Geometry and mathematical utilities;
* HDR image management;
* Materials, pigments and BRDFs;
* Cameras and ray generation;
* Scene parsing infrastructure;
* Rendering algorithms;
* Shapes definition;
* Mesh loading and BVH acceleration structures.

Use the navigation panel to browse the API documentation of each module.

### Additional Documentation

For compilation instructions, command-line usage examples, rendered images, and project overview, please refer to the GitHub repository README.

- GitHub repository: https://github.com/giuliagogna/PhyxRaypp
- Usage guide and examples: README.md in the repository

For the complete specification of the Scene Description Language, see:
- @ref SCENE_LANGUAGE
