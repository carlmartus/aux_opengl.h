/**
 * Copyright (c) 2016 Martin Sandgren
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim
 * that you wrote the original software. If you use this software in a product,
 * an acknowledgment in the product documentation would be appreciated but is
 * not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <math.h>

#define AUXGL_STRING(S) #S
#define AUXGL_GLSL_VERSION_GL_ES_2_0 "100"
#define AUXGL_GLSL_VERSION_GL_ES_3_0 300
#define AUXGL_GLSL_VERSION_GL_2_0 110
#define AUXGL_GLSL_VERSION_GL_2_1 120
#define AUXGL_GLSL_VERSION_GL_3_0 130
#define AUXGL_GLSL_VERSION_GL_3_1 140
#define AUXGL_GLSL_VERSION_GL_3_2 150
#define AUXGL_GLSL_VERSION_GL_3_3 330
#define AUXGL_GLSL_VERSION_GL_4_0 400
#define AUXGL_GLSL_VERSION_GL_4_1 410
#define AUXGL_GLSL_VERSION_GL_4_2 420
#define AUXGL_GLSL_VERSION_GL_4_3 430

#define AUXGL_GLSL_SOURCE(VER, SOURCE) "#version " VER "\n" #SOURCE
#ifdef AUXGL_DEBUG
#define auxGlCheckError() _auxGlCheckError()
#define auxGlErr(...) fprintf(stderr, "AuxGL: " __VA_ARGS__)
#else
#define auxGlCheckError()
#define auxGlErr(...)
#endif


//==============================================================================
// Error handling
//==============================================================================

#ifdef AUXGL_DEBUG
static void _auxGlCheckError() {
	GLenum e = glGetError();

	if (e != GL_NO_ERROR) {
		switch (e) {
#define ERR(cond) case cond : auxGlErr("OpenGL error %s\n", #cond); return;
			ERR(GL_INVALID_ENUM);
			ERR(GL_INVALID_VALUE);
			ERR(GL_INVALID_OPERATION);
			ERR(GL_OUT_OF_MEMORY);
			default : break;
#undef ERR
		}
	}
}
#endif


//==============================================================================
// Debugging
//==============================================================================

#ifdef AUXGL_DEBUG
static inline void auxGlDebugPrintMatrix(const char *desc, const float *mat) {
	printf("Matrix \"%s\"\n", desc);
#define ROW(R) \
	printf(" %5.3f, %5.3f, %5.3f, %5.3f,\n", \
	mat[R*4+0], mat[R*4+1], mat[R*4+2], mat[R*4+3])

	ROW(0);
	ROW(1);
	ROW(2);
	ROW(3);
#undef ROW
}

static inline void auxGlDebugPrintVector(const char *desc, const float *v) {
	printf("%s %5.3f, %5.3f, %5.3f\n", desc, v[0], v[1], v[2]);
}
#endif


//==============================================================================
// Shaders
//==============================================================================

static int auxGlCheckShader(GLuint id) {

	GLint result = GL_FALSE;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);

	if (result != GL_TRUE) {
		char infoBuf[500];
		GLsizei infoLen;
		glGetShaderInfoLog(id, sizeof(infoBuf)-1, &infoLen, infoBuf);

		auxGlCheckError();
		auxGlErr("GLSL error %s\n", infoBuf);
		return GL_FALSE;
	}

	return GL_TRUE;
}

static GLuint auxGlShader(const char *src, GLenum type) {

	GLuint shad = glCreateShader(type);
	auxGlCheckError();

	glShaderSource(shad, 1, (const char**) &src, (void*) 0);
	glCompileShader(shad);

	if (auxGlCheckShader(shad) != GL_TRUE) return 0;

	auxGlCheckError();
	return shad;
}

static inline GLuint auxGlProgram(const char *srcVert, const char *srcFrag,
		const char **attributes) {

	GLuint shadVert = auxGlShader(srcVert, GL_VERTEX_SHADER);
	if (!shadVert) return 0;

	GLuint shadFrag = auxGlShader(srcFrag, GL_FRAGMENT_SHADER);
	if (!shadFrag) {
		glDeleteShader(shadFrag);
		auxGlErr("Compiling vertex shader\n");
		return 0;
	}

	GLuint program = glCreateProgram();
	if (!program) {
		glDeleteShader(shadVert);
		glDeleteShader(shadFrag);
		auxGlErr("Compiling fragment shader\n");
		return 0;
	}

	glAttachShader(program, shadVert);
	glAttachShader(program, shadFrag);
	auxGlCheckError();

	int attribId = 0;
	if (attributes) {
		while (*attributes) {
			glBindAttribLocation(program, attribId, *attributes);
			attribId++;
			attributes++;
		}
	}

	glLinkProgram(program);
	auxGlCheckError();

	glDeleteShader(shadVert);
	glDeleteShader(shadFrag);

	return program;
}


//==============================================================================
// Projection
//==============================================================================

static void auxGlMvpPerspective(float *mvp,
		float fov, float screenratio, float near, float far) {

	float size = near * tanf(fov * 0.5); 
	float left = -size;
	float right = size;
	float bottom = -size / screenratio;
	float top = size / screenratio;

	mvp[0] = 2 * near / (right - left);
	mvp[1] = 0.0;
	mvp[2] = 0.0;
	mvp[3] = 0.0;

	mvp[4] = 0.0;
	mvp[5] = 2 * near / (top - bottom);
	mvp[6] = 0.0;
	mvp[7] = 0.0;

	mvp[8] = (right + left) / (right - left);
	mvp[9] = (top + bottom) / (top - bottom);
	mvp[10] = -(far + near) / (far - near);
	mvp[11] = -1;

	mvp[12] = 0.0;
	mvp[13] = 0.0;
	mvp[14] = -(2 * far * near) / (far - near);
	mvp[15] = 0.0;
}

static void auxGlCross(float dst[3], const float a[3], const float b[3]) {
	dst[0] = a[1]*b[2] - a[2]*b[1];
	dst[1] = a[2]*b[0] - a[0]*b[2];
	dst[2] = a[0]*b[1] - a[1]*b[0];
}

static void auxGlNormalize(float v[3]) {

	float r = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

	if (r == 0.0f) return;

	r = 1.0f / r;
	v[0] *= r;
	v[1] *= r;
	v[2] *= r;
}

static void auxGlMvpIdentity(float *mat) {
	mat[ 0]=1.f; mat[ 4]=0.f; mat[ 8]=0.f; mat[12]=0.f;
	mat[ 1]=0.f; mat[ 5]=1.f; mat[ 9]=0.f; mat[13]=0.f;
	mat[ 2]=0.f; mat[ 6]=0.f; mat[10]=1.f; mat[14]=0.f;
	mat[ 3]=0.f; mat[ 7]=0.f; mat[11]=0.f; mat[15]=1.f;
}

static void auxGlMatrixMultiply(float *mat, float *a, float *b) {
	int x, y, i=0;
	for (y=0; y<4; y++) {
		for (x=0; x<4; x++) {

			int r = y<<2;
			mat[i] =
				a[r+ 0]*b[x+ 0] +
				a[r+ 1]*b[x+ 4] +
				a[r+ 2]*b[x+ 8] +
				a[r+ 3]*b[x+12];

			i++;
		}
	}
}

static void auxGlMvpLookAt(float *mat,
		const float eye[3], const float at[3], const float up[3]) {

	float forw[3] = {
		at[0] - eye[0],
		at[1] - eye[1],
		at[2] - eye[2],
	};

	auxGlNormalize(forw);
	float side[3];
	auxGlCross(side, up, forw);
	auxGlNormalize(side);

	float nup[3];
	auxGlCross(nup, forw, side);

	float m0[16];
	auxGlMvpIdentity(m0);

	m0[ 0] = side[0];
	m0[ 4] = side[1];
	m0[ 8] = side[2];

	m0[ 1] = nup[0];
	m0[ 5] = nup[1];
	m0[ 9] = nup[2];

	m0[ 2] = -forw[0];
	m0[ 6] = -forw[1];
	m0[10] = -forw[2];

	float m1[16];
	auxGlMvpIdentity(m1);

	m1[12] = -eye[0];
	m1[13] = -eye[1];
	m1[14] = -eye[2];

	auxGlMatrixMultiply(mat, m1, m0);
}

static inline void auxGlMvpCamera(
		float *mvp, float fov, float screenratio, float near, float far,
		const float eye[3], const float at[3], const float up[3]) {

	float persp[16];
	auxGlMvpPerspective(persp, fov, screenratio, near, far);

	float look[16];
	auxGlMvpLookAt(look, eye, at, up);

	auxGlMatrixMultiply(mvp, look, persp);
}

static inline void auxGlMvpOrtho(float *mvp,
		float x0, float y0, float x1, float y1) {

	mvp[1] = mvp[2] = mvp[3] = mvp[4] = mvp[6] = mvp[7] = mvp[8] = mvp[9] = 0.f;

	mvp[ 0] = 2.f / (x1-x0);
	mvp[ 5] = 2.f / (y1-y0);
	mvp[10] = 1.f;
	mvp[15] = 1.f;

	mvp[12] = -(x1+x0)/(x1-x0);
	mvp[13] = -(y1+y0)/(y1-y0);
	mvp[14] = 0.f;
}

