#include "maths_funcs.h"
#include "gl_utils.h"
#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define _USE_MATH_DEFINES
#include <math.h>
#define GL_LOG_FILE "gl.log"
#define VERTEX_SHADER_FILE "test_vs.glsl"
#define FRAGMENT_SHADER_FILE "test_fs.glsl"
#define MESH_FILE "cube.dae"

/* keep track of window size for things like the viewport and the mouse
cursor */
int g_gl_width = 640;
int g_gl_height = 480;
GLFWwindow* g_window = NULL;

bool load_mesh(const char* file_name, GLuint* vao, int* point_count)
{
	const aiScene* scene = aiImportFile(file_name, aiProcess_Triangulate);

	if (!scene)
	{
		fprintf(stderr, "ERROR, reading mesh %s\n", file_name);
		return false;
	}
	printf("%i cameras\n", scene->mNumCameras);
	printf("%i lights\n", scene->mNumLights);
	printf("%i materials\n", scene->mNumMaterials);
	printf("%i meshes\n", scene->mNumMeshes);
	printf("%i textures\n", scene->mNumTextures);

	const aiMesh* mesh = scene->mMeshes[0];
	printf("%i vertices in mesh[0]\n", mesh->mNumVertices);

	*point_count = mesh->mNumVertices;

	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	GLfloat* points = NULL;
	GLfloat* normals = NULL;
	GLfloat* texcoords = NULL;

	if (mesh->HasPositions())
	{
		points = (GLfloat*)malloc(*point_count * 3 * sizeof(GLfloat));
		for (int i = 0; i < *point_count; i++)
		{
			const aiVector3D* vp = &(mesh->mVertices[i]);
			points[i * 3] = (GLfloat)vp->x;
			points[i * 3 + 1] = (GLfloat)vp->y;
			points[i * 3 + 2] = (GLfloat)vp->z;
		}
	}
	if (mesh->HasNormals()) {
		normals = (GLfloat*)malloc(*point_count * 3 * sizeof (GLfloat));
		for (int i = 0; i < *point_count; i++) {
			const aiVector3D* vn = &(mesh->mNormals[i]);
			normals[i * 3] = (GLfloat)vn->x;
			normals[i * 3 + 1] = (GLfloat)vn->y;
			normals[i * 3 + 2] = (GLfloat)vn->z;
		}
	}
	if (mesh->HasTextureCoords(0)) {
		texcoords = (GLfloat*)malloc(*point_count * 2 * sizeof (GLfloat));
		for (int i = 0; i < *point_count; i++) {
			const aiVector3D* vt = &(mesh->mTextureCoords[0][i]);
			texcoords[i * 2] = (GLfloat)vt->x;
			texcoords[i * 2 + 1] = (GLfloat)vt->y;
		}
	}

	if (mesh->HasPositions())
	{
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData
			(
			GL_ARRAY_BUFFER,
			3 * (*point_count) * sizeof(GLfloat),
			points,
			GL_STATIC_DRAW
			);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);
		free(points);
	}
	if (mesh->HasNormals())
	{
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData
			(
			GL_ARRAY_BUFFER,
			3 * (*point_count) * sizeof(GLfloat),
			normals,
			GL_STATIC_DRAW
			);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(1);
		free(normals);
	}
	if (mesh->HasTextureCoords(0))
	{
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData
			(
			GL_ARRAY_BUFFER,
			2 * (*point_count) * sizeof(GLfloat),
			texcoords,
			GL_STATIC_DRAW
			);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(2);
		free(texcoords);
	}
	if (mesh->HasTangentsAndBitangents())
	{
	}

	aiReleaseImport(scene);
	printf("mesh loaded\n");

	return true;
}

const char* GL_type_to_string(GLenum type) {
	switch (type) {
	case GL_BOOL: return "bool";
	case GL_INT: return "int";
	case GL_FLOAT: return "float";
	case GL_FLOAT_VEC2: return "vec2";
	case GL_FLOAT_VEC3: return "vec3";
	case GL_FLOAT_VEC4: return "vec4";
	case GL_FLOAT_MAT2: return "mat2";
	case GL_FLOAT_MAT3: return "mat3";
	case GL_FLOAT_MAT4: return "mat4";
	case GL_SAMPLER_2D: return "sampler2D";
	case GL_SAMPLER_3D: return "sampler3D";
	case GL_SAMPLER_CUBE: return "samplerCube";
	case GL_SAMPLER_2D_SHADOW: return "sampler2DShadow";
	default: break;
	}
	return "other";
}

/* print errors in shader compilation */
void _print_shader_info_log(GLuint shader_index) {
	int max_length = 2048;
	int actual_length = 0;
	char log[2048];
	glGetShaderInfoLog(shader_index, max_length, &actual_length, log);
	printf("shader info log for GL index %i:\n%s\n", shader_index, log);
}

/* print errors in shader linking */
void _print_programme_info_log(GLuint sp) {
	int max_length = 2048;
	int actual_length = 0;
	char log[2048];
	glGetProgramInfoLog(sp, max_length, &actual_length, log);
	printf("program info log for GL index %i:\n%s", sp, log);
}

/* validate shader */
bool is_valid(GLuint sp) {
	int params = -1;

	glValidateProgram(sp);
	glGetProgramiv(sp, GL_VALIDATE_STATUS, &params);
	printf("program %i GL_VALIDATE_STATUS = %i\n", sp, params);
	if (GL_TRUE != params) {
		_print_programme_info_log(sp);
		return false;
	}
	return true;
}

/* print absolutely everything about a shader - only useful if you get really
stuck wondering why a shader isn't working properly */
void print_all(GLuint sp) {
	int params = -1;
	int i;

	printf("--------------------\nshader programme %i info:\n", sp);
	glGetProgramiv(sp, GL_LINK_STATUS, &params);
	printf("GL_LINK_STATUS = %i\n", params);

	glGetProgramiv(sp, GL_ATTACHED_SHADERS, &params);
	printf("GL_ATTACHED_SHADERS = %i\n", params);

	glGetProgramiv(sp, GL_ACTIVE_ATTRIBUTES, &params);
	printf("GL_ACTIVE_ATTRIBUTES = %i\n", params);

	for (i = 0; i < params; i++) {
		char name[64];
		int max_length = 64;
		int actual_length = 0;
		int size = 0;
		GLenum type;
		glGetActiveAttrib(sp, i, max_length, &actual_length, &size, &type,
			name);
		if (size > 1) {
			int j;
			for (j = 0; j < size; j++) {
				char long_name[64];
				int location;

				sprintf(long_name, "%s[%i]", name, j);
				location = glGetAttribLocation(sp, long_name);
				printf("  %i) type:%s name:%s location:%i\n",
					i, GL_type_to_string(type), long_name, location);
			}
		}
		else {
			int location = glGetAttribLocation(sp, name);
			printf("  %i) type:%s name:%s location:%i\n",
				i, GL_type_to_string(type), name, location);
		}
	}

	glGetProgramiv(sp, GL_ACTIVE_UNIFORMS, &params);
	printf("GL_ACTIVE_UNIFORMS = %i\n", params);
	for (i = 0; i < params; i++) {
		char name[64];
		int max_length = 64;
		int actual_length = 0;
		int size = 0;
		GLenum type;
		glGetActiveUniform(sp, i, max_length, &actual_length, &size, &type,
			name);
		if (size > 1) {
			int j;
			for (j = 0; j < size; j++) {
				char long_name[64];
				int location;

				sprintf(long_name, "%s[%i]", name, j);
				location = glGetUniformLocation(sp, long_name);
				printf("  %i) type:%s name:%s location:%i\n",
					i, GL_type_to_string(type), long_name, location);
			}
		}
		else {
			int location = glGetUniformLocation(sp, name);
			printf("  %i) type:%s name:%s location:%i\n",
				i, GL_type_to_string(type), name, location);
		}
	}

	_print_programme_info_log(sp);
}

/* copy a shader from a plain text file into a character array */
bool parse_file_into_str(const char* file_name, char* shader_str, int max_len
	) {
	FILE* file = fopen(file_name, "r");
	if (!file) {
		gl_log_err("ERROR: opening file for reading: %s\n", file_name);
		return false;
	}
	size_t cnt = fread(shader_str, 1, max_len - 1, file);
	if (cnt >= max_len - 1) {
		gl_log_err("WARNING: file %s too big - truncated.\n", file_name);
	}
	if (ferror(file)) {
		gl_log_err("ERROR: reading shader file %s\n", file_name);
		fclose(file);
		return false;
	}
	// append \0 to end of file string
	shader_str[cnt] = 0;
	fclose(file);
	return true;
}

int main() {
	GLfloat points[] = {
		0.0f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f
	};
	GLfloat colors[] = {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	};
	GLfloat matrix[] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.0f, 0.0f, 1.0f
	};
	int params = -1;

	assert(restart_gl_log());
	assert(start_gl());

	/* tell GL to only draw onto a pixel if the shape is closer to the viewer*/
	glEnable(GL_DEPTH_TEST); /* enable depth-testing */
	glDepthFunc(GL_LESS); /* depth-testing interprets a smaller value as
						  "closer" */

	/*GLuint points_vbo;
	glGenBuffers(1, &points_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), points,
		GL_STATIC_DRAW);

	GLuint colors_vbo;
	glGenBuffers(1, &colors_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), colors, GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);*/

	GLuint cube_vao;
	int cube_point_count;
	assert(load_mesh(MESH_FILE, &cube_vao, &cube_point_count));

	/* load shaders from files here */

	char vertex_shader[1024 * 256];
	char fragment_shader[1024 * 256];
	assert(parse_file_into_str("test_vs.glsl", vertex_shader, 1024 * 256));
	assert(parse_file_into_str("test_fs.glsl", fragment_shader, 1024 * 256));

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* p = (const GLchar*)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);

	/* check for shader compile errors - very important! */

	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
		_print_shader_info_log(vs);
		return 1; /* or exit or something */
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar*)fragment_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);

	/* check for compile errors */
	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
		_print_shader_info_log(fs);
		return 1; /* or exit or something */
	}

	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	/* check for shader linking errors - very important! */
	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf(
			stderr,
			"ERROR: could not link shader programme GL index %i\n",
			shader_programme
			);
		_print_programme_info_log(shader_programme);
		return 1;
	}
	print_all(shader_programme);
	assert(is_valid(shader_programme));

	int model_location = glGetUniformLocation(shader_programme, "model");
	assert(model_location > -1);
	
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	float speed = 1.0f;
	float last_position = 0.0f;
	double previous_seconds = glfwGetTime();

	// make projection matrix
	float near = 0.1f;
	float far = 100.0f;
	float fov = 67.0f * ONE_DEG_IN_RAD;
	float aspect = (float)g_gl_width / (float)g_gl_height;
	float range = tan(fov * 0.5f) * near;
	float Sx = (2.0f * near) / (range * aspect + range * aspect);
	float Sy = near / range;
	float Sz = -(far + near) / (far - near);
	float Pz = -(2.0f * far * near) / (far - near);

	mat4 proj_mat = mat4 (
		Sx, 0.0f, 0.0f, 0.0f,
		0.0f, Sy, 0.0f, 0.0f,
		0.0f, 0.0f, Sz, -1.0f,
		0.0f, 0.0f, Pz, 0.0f
	);

	float cam_speed = 1.0f;
	float cam_yaw_speed = 10.0f;
	float cam_pos[] = { 0.0f, 0.0f, 2.0f };
	float cam_yaw = 0.0f;
	mat4 T = translate(identity_mat4(), vec3(-cam_pos[0], -cam_pos[1], -cam_pos[2]));
	mat4 R = rotate_y_deg(identity_mat4(), -cam_yaw);
	mat4 view_mat = R * T;

	GLint view_mat_location = glGetUniformLocation(shader_programme, "view");
	GLint proj_mat_location = glGetUniformLocation(shader_programme, "proj");
	glUseProgram(shader_programme);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view_mat.m);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, proj_mat.m);


	while (!glfwWindowShouldClose(g_window)) {
		double current_seconds = glfwGetTime();
		double elapsed_seconds = current_seconds - previous_seconds;
		previous_seconds = current_seconds;

		_update_fps_counter(g_window);
		
		/* wipe the drawing surface clear */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_gl_width, g_gl_height);

		glUseProgram(shader_programme);
		
		matrix[12] = elapsed_seconds * speed + last_position;
		last_position = matrix[12];
		if (fabs(last_position) > 1.0)
			speed = -speed;


		glUniformMatrix4fv(model_location, 1, GL_FALSE, matrix);
		glBindVertexArray(cube_vao);
		/* draw points 0-3 from the currently bound VAO with current in-use
		shader */
		glDrawArrays(GL_TRIANGLES, 0, cube_point_count);
		/* update other events like input handling */
		glfwPollEvents();

		// control keys
		bool cam_moved = false;
		if (glfwGetKey(g_window, GLFW_KEY_A)) {
			cam_pos[0] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_D)) {
			cam_pos[0] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_PAGE_UP)) {
			cam_pos[1] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_PAGE_DOWN)) {
			cam_pos[1] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_W)) {
			cam_pos[2] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_S)) {
			cam_pos[2] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_LEFT)) {
			cam_yaw += cam_yaw_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_RIGHT)) {
			cam_yaw -= cam_yaw_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (cam_moved)
		{
			mat4 T = translate(identity_mat4(), vec3(-cam_pos[0], -cam_pos[1], -cam_pos[2]));
			mat4 R = rotate_y_deg(identity_mat4(), -cam_yaw);
			mat4 view_mat = R * T;
			glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view_mat.m);
		}
		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(g_window, 1);
		}
		/* put the stuff we've been drawing onto the display */
		glfwSwapBuffers(g_window);
	}

	/* close GL context and any other GLFW resources */
	glfwTerminate();
	return 0;
}
