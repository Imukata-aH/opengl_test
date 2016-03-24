#include "maths_funcs.h"
#include "gl_utils.h"
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define GL_LOG_FILE "gl.log"
#define VERTEX_SHADER_FILE "test_vs.glsl"
#define FRAGMENT_SHADER_FILE "test_fs.glsl"
#define MESH_FILE "suzanne.dae"

/* keep track of window size for things like the viewport and the mouse
cursor */
int g_gl_width = 640;
int g_gl_height = 480;
GLFWwindow* g_window = NULL;

int main() {
	assert(restart_gl_log());
	assert(start_gl());

	/* tell GL to only draw onto a pixel if the shape is closer to the viewer*/
	glEnable(GL_DEPTH_TEST); /* enable depth-testing */
	glDepthFunc(GL_LESS); /* depth-testing interprets a smaller value as"closer" */
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
	glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
	glViewport(0, 0, g_gl_width, g_gl_height);

	// load the mesh using assimp
	GLuint cube_vao;
	int cube_point_count;
	assert(load_mesh(MESH_FILE, &cube_vao, &cube_point_count));

	/* load shaders from files here */
	GLuint shader_programme = create_programme_from_files(VERTEX_SHADER_FILE, FRAGMENT_SHADER_FILE);

	// make view matrix
	vec3 cam_pos = vec3(0.0f, -300.0f, 0.0f);
	vec3 target_pos = vec3( 0.0f, 0.0f, 0.0f );
	vec3 up_vec = vec3(0.0f, 1.0f, 0.0f);
	mat4 viewMat = look_at(cam_pos, target_pos, up_vec);
	// make projection matrix
	float near = 0.1f;
	float far = 1000.0f;
	float fov = 67.0f * ONE_DEG_IN_RAD;
	float aspect = (float)g_gl_width / (float)g_gl_height;
	mat4 projMat = perspective(fov, aspect, near, far);
	// make projection-view Matrix
	mat4 pvMat = projMat * viewMat;

	// set matrices to shader uniform variables
	GLint view_mat_location = glGetUniformLocation(shader_programme, "view");
	glUseProgram(shader_programme);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, viewMat.m);
	GLint proj_mat_location = glGetUniformLocation(shader_programme, "proj");
	glUseProgram(shader_programme);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, projMat.m);

	// ‰ŠúˆÊ’u‚ÌŒˆ’è‚Æmodels—ñ‚Ìuniform location‚ÌŽæ“¾
	mat4 model_matrix = translate(identity_mat4(), vec3(0.5f, 0.0f, 0.0f));
	int model_location = glGetUniformLocation(shader_programme, "model");
	assert(model_location > -1);
	
	float cam_speed = 3.0f;
	float cam_yaw = 0.0f;
	float cam_yaw_speed = 5.0f;

	float model_speed = 1.0f;
	float model_last_position = 0.0f;
	double previous_seconds = glfwGetTime();

	while (!glfwWindowShouldClose(g_window)) {
		double current_seconds = glfwGetTime();
		double elapsed_seconds = current_seconds - previous_seconds;
		previous_seconds = current_seconds;
		update_fps_counter(g_window);
		
		/* wipe the drawing surface clear */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_gl_width, g_gl_height);
		glUseProgram(shader_programme);
		
		// udpate model matrix
		model_matrix.m[12] = elapsed_seconds * model_speed + model_last_position;
		model_last_position = model_matrix.m[12];
		if (fabs(model_last_position) > 1.0)
			model_speed = -model_speed;
		glUniformMatrix4fv(model_location, 1, GL_FALSE, model_matrix.m);

		// draw mesh
		glBindVertexArray(cube_vao);
		glDrawArrays(GL_TRIANGLES, 0, cube_point_count);

		/* update other events like input handling */
		glfwPollEvents();

		// control keys
		bool cam_moved = false;
		if (glfwGetKey(g_window, GLFW_KEY_A)) {
			cam_pos.v[0] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_D)) {
			cam_pos.v[0] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_PAGE_UP)) {
			cam_pos.v[1] += cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_PAGE_DOWN)) {
			cam_pos.v[1] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_W)) {
			cam_pos.v[2] -= cam_speed * elapsed_seconds;
			cam_moved = true;
		}
		if (glfwGetKey(g_window, GLFW_KEY_S)) {
			cam_pos.v[2] += cam_speed * elapsed_seconds;
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
			mat4 T = translate(identity_mat4(), vec3(-cam_pos.v[0], -cam_pos.v[1], -cam_pos.v[2]));
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
