/*
* Copyright (c) 2026 Giulia Gogna, Riccardo Piazza.
 *
 * Licensed under the EUPL, Version 1.2 or – as soon they will be approved by
 * the European Commission - subsequent versions of the EUPL (the "Licence");
 * You may not use this work except in compliance with the Licence.
 * You may obtain a copy of the Licence at:
 *
 * https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the Licence is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing permissions and
 * limitations under the Licence.
 */

// This file is the only that includes the implementation of the stb libraries,
// so that we don't have to include them in every file that uses them.
// The stb libraries are header-only, so we need to define the implementation in one file.

// GG: is it really necessary given that we will only use this in HDRImage?

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>