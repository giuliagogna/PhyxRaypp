// This file is the only that includes the implementation of the stb libraries, so that we don't have to include them in every file that uses them.
// The stb libraries are header-only, so we need to define the implementation in one file.

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>