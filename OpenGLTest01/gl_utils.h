#ifndef _GL_UTILS_H_
#define _GL_UTILS_H_

#include <stdarg.h> // used by log functions to have variable number of args
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library

#define GL_LOG_FILE "gl.log"

extern int g_gl_width;
extern int g_gl_height;
extern GLFWwindow* g_window;

/*--------------------GL Information Logger---------------------------*/
bool start_gl();
bool restart_gl_log();
bool gl_log(const char* message, ...);
/* same as gl_log except also prints to stderr */
bool gl_log_err(const char* message, ...);
void glfw_error_callback(int error, const char* description);
void log_gl_params();
void update_fps_counter(GLFWwindow* window);
void glfw_window_size_callback(GLFWwindow* window, int width, int height);

/*--------------------GL Shader Util and Debug---------------------------*/
const char* GL_type_to_string(GLenum type);
void print_shader_info_log(GLuint shader_index);
void print_programme_info_log(GLuint sp);
bool is_valid(GLuint sp);
void print_all(GLuint sp);
bool parse_file_into_str(const char* file_name, char* shader_str, int max_len);
bool create_shader(const char* file_name, GLuint* shader, GLenum type);
bool create_programme(GLuint vs, GLuint fs, GLuint* programme);
bool is_programme_valid(GLuint sp);
GLuint create_programme_from_files(const char* vs_filename, const char* fs_filename);

/*--------------------3D Object File Importer---------------------------*/
bool load_mesh(const char* file_name, GLuint* vao, int* point_count);

#endif