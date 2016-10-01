# Auxiliary OpenGL functions header library

All functionality is found as functions in the aux_opengl.h header file. To
ensure portability the header does not include any OpenGL headers itself. That
must be done by the developer before including this library.

For example:
```
#include <GLES2/gl2.h>
#include "aux_opengl.h"
```

This library can check OpenGL error and print them on *stdout* if you define the
macro **AUXGL_DEBUG** before including this library. Like this:
```
#include <GLES2/gl2.h>
#define AUXGL_DEBUG
#include "aux_opengl.h"
```

The camera function relies on **sinf** and **cosf** from the standard *math.h*
header. Depending on your target system, this means you might need to link
against the math library. With a GCC compiler this is done by passing the
argument **-lm** to the linker.

## Features
 * Shader inline source macros, see *AUXGL_STRING*.
 * Shader loading, see *auxGlProgram*.
 * Model view projection (MVP) 3D camera matrix calculation, see 3D
   *auxGlMvpCamera*.
 * MVP orthogonal 2D matrix calculation, see *auxGlMvpOrtho*.
 * Optional error checking agaist the OpenGL library, see **AUXGL_DEBUG**.

## Roadmap
 * Documentation.
 * Samples.
 * Texture render target setup.

# Copyright notice (zlib license)
Copyright (c) 2016 Martin Sandgren

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

 1. The origin of this software must not be misrepresented; you must not claim
	that you wrote the original software. If you use this software in a product,
	an acknowledgment in the product documentation would be appreciated but is
	not required.

 2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

 3. This notice may not be removed or altered from any source distribution.

