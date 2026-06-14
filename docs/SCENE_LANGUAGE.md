# PhyxRadpp Scene Description Language {#SCENE_LANGUAGE}

`PhyxRadpp` uses a custom text-based scene description language. The language is parsed at runtime, allowing you to build and modify 3D environments without needing to recompile the C++ engine.

---

## 1. Basics & Syntax
* **Comments:** Any line starting with `#` is a comment and will be ignored by the parser.
* **Semicolons:** Top-level declarations (variables, materials, cameras, shapes) must end with a semicolon `;`.
* **Whitespace:** Spaces, tabs, and newlines are ignored.

### Data Types
* **Floats:** Standard floating-point numbers (e.g., `1.0`, `-0.5`, `3.14`).
* **Vectors:** 3D coordinates enclosed in square brackets: `[x, y, z]`.
* **Colors:** RGB values enclosed in angle brackets: `<r, g, b>`. Color components are normalized between `0.0` and `1.0`.

### Variables
You can define floating-point variables to reuse values throughout your scene:

```text
float pi_over_2 = 1.5707963;
float my_scale = 2.5;
```

## 2. Environment and Camera

### Background

Defines the global sky color of the scene.

```text
background(<r, g, b>);
```

### Camera

You can define exactly one camera per scene. The camera can be perspective or orthogonal. 

- **Perspective Camera Parameters:** (type, aspect_ratio, distance, transformation)
- **Orthogonal Camera Parameters:** (type, aspect_ratio, transformation)

*Note: aspect ratio, distance, and transformations are optional and will fall back to engine defaults if omitted.*

```text
# Example: Perspective camera moved back 10 units and up 2 units
camera(perspective, 1.0, 3.0, translation([-10.0, 0.0, 2.0]));
```

## 3. Materials and Pigments

A material dictates how light interacts with a surface. 
It is composed of a BRDF (Bidirectional Reflectance Distribution Function) and an optional Emitted Radiance (for glowing objects like lamps).

### Pigments

Pigments define the visual pattern of a surface.

- `uniform(<r, g, b>)` : A solid color.
- `checkered(<r1, g1, b1>, <r2, g2, b2>, steps)` : A 3D checkerboard pattern.
- `image("path/to/image.pfm")` : A high-dynamic-range image texture mapping.

### BRDFs

- `diffuse(pigment)` : A rough, matte surface that scatters light in all directions. 
- `specular(pigment)` : A smooth, mirror-like surface that reflects light perfectly.

### Defining Materials

Materials must be named so they can be attached to shapes later.

```text
# Syntax: material name(brdf, [emitted_radiance]);

# A matte blue material
material matte_blue(diffuse(uniform(<0.0, 0.0, 1.0>)));

# A shiny red mirror
material red_mirror(specular(uniform(<1.0, 0.0, 0.0>)));

# A glowing white lamp (uses the optional second pigment parameter)
material light_bulb(diffuse(uniform(<1.0, 1.0, 1.0>)), uniform(<10.0, 10.0, 10.0>));
```

## 4. Transformations

Transformations allow you to move, rotate, and scale objects and cameras in 3D space. They are chained together using the \* operator.

- `identity` : No transformation, objects stay positioned in the default location.
- `translation(\[x, y, z\])` : Moves the object.
- `scaling(\[x, y, z\])` : Resizes the object.
- `rot\_x(angle)` : Rotates around the X-axis (in radians).
- `rot\_y(angle)` : Rotates around the Y-axis (in radians).
- `rot\_z(angle)` : Rotates around the Z-axis (in radians).

*Important: Transformations are applied right-to-left. translation(...) \* scaling(...) will scale the object first, then move it.*

## 5. Shapes and Geometry

Once a material is defined, you can apply it to geometric primitives. The transformation matrix is optional.

**Syntax:** `shape_type(transformation, material_name);` or `shape_type(material_name);

### Built-in primitives

```text
# A default sphere at the origin (0,0,0)
sphere(matte_blue);

# A scaled and moved plane
plane(translation([0.0, -1.0, 0.0]) * scaling([5.0, 5.0, 5.0]), checkered_mat);

# A rotated cube
cube(rot_y(1.57), matte_blue);
```

### 3D Meshes (.obj)

You can load complex 3D geometry from `.obj` files. 
The mesh automatically builds a Bounding Volume Hierarchy (BVH) to optimize rendering speed.

**Syntax**: `mesh("filepath", material_name, transformation);`

```text
# Load the Utah Teapot
mesh("mesh/utah_teapot.obj", red_mirror, scaling([0.4, 0.4, 0.4]));
```