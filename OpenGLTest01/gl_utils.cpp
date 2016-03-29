#include "gl_utils.h"
#include "maths_funcs.h"
#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#define MAX_SHADER_LENGTH 262144

/*-----------------------GL Information Logger-----------------------------*/

/* start a new log file. put the time and date at the top */
bool restart_gl_log() {
	time_t now;
	char* date;
	FILE* file = fopen(GL_LOG_FILE, "w");

	if (!file) {
		fprintf(
			stderr,
			"ERROR: could not open GL_LOG_FILE log file %s for writing\n",
			GL_LOG_FILE
			);
		return false;
	}
	now = time(NULL);
	date = ctime(&now);
	fprintf(file, "GL_LOG_FILE log. local time %s", date);
	fprintf(file, "build version: %s %s\n\n", __DATE__, __TIME__);
	fclose(file);
	return true;
}

/* add a message to the log file. arguments work the same way as printf() */
bool gl_log(const char* message, ...) {
	va_list argptr;
	FILE* file = fopen(GL_LOG_FILE, "a");
	if (!file) {
		fprintf(
			stderr,
			"ERROR: could not open GL_LOG_FILE %s file for appending\n",
			GL_LOG_FILE
			);
		return false;
	}
	va_start(argptr, message);
	vfprintf(file, message, argptr);
	va_end(argptr);
	fclose(file);
	return true;
}

/* same as gl_log except also prints to stderr */
bool gl_log_err(const char* message, ...) {
	va_list argptr;
	FILE* file = fopen(GL_LOG_FILE, "a");
	if (!file) {
		fprintf(
			stderr,
			"ERROR: could not open GL_LOG_FILE %s file for appending\n",
			GL_LOG_FILE
			);
		return false;
	}
	va_start(argptr, message);
	vfprintf(file, message, argptr);
	va_end(argptr);
	va_start(argptr, message);
	vfprintf(stderr, message, argptr);
	va_end(argptr);
	fclose(file);
	return true;
}

/* we will tell GLFW to run this function whenever it finds an error */
void glfw_error_callback(int error, const char* description) {
	gl_log_err("GLFW ERROR: code %i msg: %s\n", error, description);
}

/* we will tell GLFW to run this function whenever the window is resized */
void glfw_window_size_callback(GLFWwindow* window, int width, int height) {
	g_gl_width = width;
	g_gl_height = height;
	printf("width %i height %i\n", width, height);
	/* update any perspective matrices used here */
}

/* we can use a function like this to print some GL capabilities of our adapter
to the log file. handy if we want to debug problems on other people's computers
*/
void log_gl_params() {
	int i;
	int v[2];
	unsigned char s = 0;
	GLenum params[] = {
		GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
		GL_MAX_CUBE_MAP_TEXTURE_SIZE,
		GL_MAX_DRAW_BUFFERS,
		GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
		GL_MAX_TEXTURE_IMAGE_UNITS,
		GL_MAX_TEXTURE_SIZE,
		GL_MAX_VARYING_FLOATS,
		GL_MAX_VERTEX_ATTRIBS,
		GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
		GL_MAX_VERTEX_UNIFORM_COMPONENTS,
		GL_MAX_VIEWPORT_DIMS,
		GL_STEREO,
	};
	const char* names[] = {
		"GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS",
		"GL_MAX_CUBE_MAP_TEXTURE_SIZE",
		"GL_MAX_DRAW_BUFFERS",
		"GL_MAX_FRAGMENT_UNIFORM_COMPONENTS",
		"GL_MAX_TEXTURE_IMAGE_UNITS",
		"GL_MAX_TEXTURE_SIZE",
		"GL_MAX_VARYING_FLOATS",
		"GL_MAX_VERTEX_ATTRIBS",
		"GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS",
		"GL_MAX_VERTEX_UNIFORM_COMPONENTS",
		"GL_MAX_VIEWPORT_DIMS",
		"GL_STEREO",
	};
	gl_log("GL Context Params:\n");
	// integers - only works if the order is 0-10 integer return types
	for (i = 0; i < 10; i++) {
		int v = 0;
		glGetIntegerv(params[i], &v);
		gl_log("%s %i\n", names[i], v);
	}
	// others
	v[0] = v[1] = 0;
	glGetIntegerv(params[10], v);
	gl_log("%s %i %i\n", names[10], v[0], v[1]);
	glGetBooleanv(params[11], &s);
	gl_log("%s %i\n", names[11], (unsigned int)s);
	gl_log("-----------------------------\n");
}

double previous_seconds;
int frame_count;

/* we will use this function to update the window title with a frame rate */
void update_fps_counter(GLFWwindow* window) {
	double current_seconds;
	double elapsed_seconds;
	char tmp[128];

	current_seconds = glfwGetTime();
	elapsed_seconds = current_seconds - previous_seconds;
	if (elapsed_seconds > 0.25) {
		previous_seconds = current_seconds;

		double fps = (double)frame_count / elapsed_seconds;
		sprintf(tmp, "opengl @ fps: %.2f", fps);
		glfwSetWindowTitle(window, tmp);
		frame_count = 0;
	}
	frame_count++;
}

bool start_gl()
{
	const GLubyte* renderer;
	const GLubyte* version;

	gl_log("starting GLFW %s", glfwGetVersionString());

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	/* We must specify 3.2 core if on Apple OS X -- other O/S can specify
	anything here. I defined 'APPLE' in the makefile for OS X */
#ifdef APPLE
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
	/*GLFWmonitor* mon = glfwGetPrimaryMonitor ();
	const GLFWvidmode* vmode = glfwGetVideoMode (mon);
	g_window = glfwCreateWindow (
	vmode->width, vmode->height, "Extended GL Init", mon, NULL
	);*/

	g_window = glfwCreateWindow(g_gl_width, g_gl_height, "Shader Program", NULL, NULL);
	if (!g_window) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}
	glfwSetWindowSizeCallback(g_window, glfw_window_size_callback);
	glfwMakeContextCurrent(g_window);

	glfwWindowHint(GLFW_SAMPLES, 4);

	/* start GLEW extension handler */
	glewExperimental = GL_TRUE;
	glewInit();

	/* get version info */
	renderer = glGetString(GL_RENDERER); /* get renderer string */
	version = glGetString(GL_VERSION); /* version as a string */
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);
	gl_log("renderer: %s\nversion: %s\n", renderer, version);
	log_gl_params();

	return true;
}


/*--------------------GL Shader Util and Debug---------------------------*/
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
void print_shader_info_log(GLuint shader_index) {
	int max_length = 2048;
	int actual_length = 0;
	char log[2048];
	glGetShaderInfoLog(shader_index, max_length, &actual_length, log);
	printf("shader info log for GL index %i:\n%s\n", shader_index, log);
}

/* print errors in shader linking */
void print_programme_info_log(GLuint sp) {
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
		print_programme_info_log(sp);
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

	print_programme_info_log(sp);
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

bool create_shader(const char* file_name, GLuint* shader, GLenum type)
{
	gl_log("creating shader form %s...\n", file_name);
	char shader_string[MAX_SHADER_LENGTH];
	assert(parse_file_into_str(file_name, shader_string, MAX_SHADER_LENGTH));
	*shader = glCreateShader(type);
	const GLchar* p = (const GLchar*)shader_string;
	glShaderSource(*shader, 1, &p, NULL);
	glCompileShader(*shader);

	// check for compile errors
	int params = -1;
	glGetShaderiv(*shader, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		gl_log_err("ERROR: GL shader index %i dit not compile\n", *shader);
		print_shader_info_log(*shader);
		return false;
	}
	gl_log("shader compiled. index %i", *shader);
	return true;
}

bool is_programme_valid(GLuint sp)
{
	glValidateProgram(sp);
	GLint params = -1;
	glGetProgramiv(sp, GL_VALIDATE_STATUS, &params);
	if (GL_TRUE != params)
	{
		gl_log_err("program %i GL_VALIDATE_STATUS = GL_FALSE\n", sp);
		print_programme_info_log(sp);
		return false;
	}
	gl_log("program %i GL_VALIDATE_STATUS = GL_TRUE\n", sp);
	return true;
}

bool create_programme(GLuint vs, GLuint fs, GLuint* programme)
{
	*programme = glCreateProgram();
	gl_log(
		"created programme %u, attaching shaders %u and %u...\n",
		*programme,
		vs,
		fs
	);
	glAttachShader(*programme, vs);
	glAttachShader(*programme, fs);
	// link the shader programme. if binding input attributes, do that before link 
	// (attribute(in)変数のバインドのためにGLSLのlocationキーワードが使えない場合。glBindAttribLocation()をLinkの前に行う)
	glLinkProgram(*programme);
	GLint params = -1;
	glGetProgramiv(*programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params)
	{
		gl_log_err("ERROR: could not link shader programme GL index %u\n", *programme);
		print_programme_info_log(*programme);
		return false;
	}
	assert(is_programme_valid(*programme));
	
	glDeleteShader(vs);
	glDeleteShader(fs);

	return true;
}

GLuint create_programme_from_files(const char* vs_filename, const char* fs_filename)
{
	GLuint vs, fs, programme;
	assert(create_shader(vs_filename, &vs, GL_VERTEX_SHADER));
	assert(create_shader(fs_filename, &fs, GL_FRAGMENT_SHADER));
	assert(create_programme(vs, fs, &programme));
	return programme;
}

/*--------------------3D Object File Importer---------------------------*/
mat4 convert_assimp_matrix(aiMatrix4x4 m)
{
	return mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		m.a4, m.b4, m.c4, m.d4);
}


bool load_mesh(
	const char* file_name,
	GLuint* vao,
	int* point_count,
	mat4* bone_offset_mats,
	int* bone_count)
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
	if (mesh->HasBones())
	{
		*bone_count = (int)mesh->mNumBones;
		// bone names. max 256 bones, maax name length 64.
		char bonenames[256][64];
		for (int b_i = 0; b_i < *bone_count; b_i++)
		{
			const aiBone* bone = mesh->mBones[b_i];
			strcpy(bonenames[b_i], bone->mName.data);
			printf("bonenames[%i] = %s\n", b_i, bonenames[b_i]);
			bone_offset_mats[b_i] = convert_assimp_matrix(bone->mOffsetMatrix);
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