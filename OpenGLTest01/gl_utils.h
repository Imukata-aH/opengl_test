#ifndef _GL_UTILS_H_
#define _GL_UTILS_H_

#include <stdarg.h> // used by log functions to have variable number of args
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library

#define GL_LOG_FILE "gl.log"

extern int g_gl_width;
extern int g_gl_height;
extern GLFWwindow* g_window;

bool start_gl();

bool restart_gl_log();

bool gl_log(const char* message, ...);

/* same as gl_log except also prints to stderr */
bool gl_log_err(const char* message, ...);

void glfw_error_callback(int error, const char* description);

void log_gl_params();

void _update_fps_counter(GLFWwindow* window);

void glfw_window_size_callback(GLFWwindow* window, int width, int height);

#endif