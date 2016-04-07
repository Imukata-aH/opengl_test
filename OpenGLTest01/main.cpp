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
#define MESH_FILE "suzanne_skeleton.dae" //"suzanne_bone.dae" //"suzanne.dae"

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
	GLuint monkey_vao;
	mat4 monkey_bone_offset_matrices[MAX_BONES];
	Skeleton_Node* monkey_skeleton;
	int monkey_point_count = 0;
	int monkey_bone_count = 0;
	assert(load_mesh(MESH_FILE, &monkey_vao, &monkey_point_count, monkey_bone_offset_matrices, &monkey_bone_count, &monkey_skeleton));
	printf("%s bone count: %i\n", MESH_FILE, monkey_bone_count);

	// bone位置確認用のバッファ作成とボーン位置行列の表示
	float bone_positions[3 * 256];
	int c = 0;
	for (int i = 0; i < monkey_bone_count; i++)
	{
		print(monkey_bone_offset_matrices[i]);

		bone_positions[c++] = -monkey_bone_offset_matrices[i].m[12];
		bone_positions[c++] = -monkey_bone_offset_matrices[i].m[13];
		bone_positions[c++] = -monkey_bone_offset_matrices[i].m[14];
	}
	GLuint bones_vao;
	glGenVertexArrays(1, &bones_vao);
	glBindVertexArray(bones_vao);
	GLuint bones_vbo;
	glGenBuffers(1, &bones_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, bones_vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		3 * monkey_bone_count * sizeof(float),
		bone_positions,
		GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	/* load shaders from files here */
	GLuint shader_programme = create_programme_from_files(VERTEX_SHADER_FILE, FRAGMENT_SHADER_FILE);
	GLuint bones_shader_programme = create_programme_from_files("bones_vs.glsl", "bones_fs.glsl");

	// make view matrix
	vec3 cam_pos = vec3(0.0f, 0.0f, 300.0f);
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
	glUseProgram(shader_programme);
	mat4 model_matrix = identity_mat4();
	int model_location = glGetUniformLocation(shader_programme, "model");
	glUniformMatrix4fv(model_location, 1, GL_FALSE, model_matrix.m);
	// view
	GLint view_mat_location = glGetUniformLocation(shader_programme, "view");
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, viewMat.m);
	// proj
	GLint proj_mat_location = glGetUniformLocation(shader_programme, "proj");
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, projMat.m);
	// bone matrices　OpenGLの実装では、uniform変数の配列のlocation値は連続とは限らないので、それぞれのインデックスに対してlocationを探索する
	int bone_matrices_locations[MAX_BONES];
	char name[64];
	for (int i = 0; i < MAX_BONES; i++)
	{
		sprintf(name, "bone_matrices[%i]", i);
		bone_matrices_locations[i] = glGetUniformLocation(shader_programme, name);
		glUniformMatrix4fv(bone_matrices_locations[i], 1, GL_FALSE, identity_mat4().m);	// 単位行列で初期化
	}

	// ボーン位置を表示するためのシェーダのUniform変数に値をセット
	glUseProgram(bones_shader_programme);
	int bones_view_mat_location = glGetUniformLocation(bones_shader_programme, "view");
	glUniformMatrix4fv(bones_view_mat_location, 1, GL_FALSE, viewMat.m);
	int bones_proj_mat_location = glGetUniformLocation(bones_shader_programme, "proj");
	glUniformMatrix4fv(bones_proj_mat_location, 1, GL_FALSE, projMat.m);
	
	float cam_speed = 3.0f;
	float cam_yaw = 0.0f;
	float cam_yaw_speed = 5.0f;

	float model_speed = 1.0f;
	float model_last_position = 0.0f;
	double previous_seconds = glfwGetTime();

	//左のお耳をぐーるぐる
	float bone_theta = 0.0f;
	float bone_rot_speed = 50.0f;
	mat4 left_ear_mat = identity_mat4();
	int left_ear_bone_id = 2;	// 左耳のボーンidは2番目なので

	while (!glfwWindowShouldClose(g_window)) {
		double current_seconds = glfwGetTime();
		double elapsed_seconds = current_seconds - previous_seconds;
		previous_seconds = current_seconds;
		update_fps_counter(g_window);
		
		/* wipe the drawing surface clear */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_gl_width, g_gl_height);
		
		// draw mesh
		// udpate model matrix
		glUseProgram(shader_programme);
		/*model_matrix.m[12] = elapsed_seconds * model_speed + model_last_position;
		model_last_position = model_matrix.m[12];
		if (fabs(model_last_position) > 1.0)
			model_speed = -model_speed;
		glUniformMatrix4fv(model_location, 1, GL_FALSE, model_matrix.m);*/

		glEnable(GL_DEPTH_TEST);
		glBindVertexArray(monkey_vao);
		glDrawArrays(GL_TRIANGLES, 0, monkey_point_count);

		// ボーン位置を描画
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_PROGRAM_POINT_SIZE);
		glUseProgram(bones_shader_programme);
		glBindVertexArray(bones_vao);
		glDrawArrays(GL_POINTS, 0, monkey_bone_count);
		glDisable(GL_PROGRAM_POINT_SIZE);

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
		if (glfwGetKey(g_window, 'Z'))
		{
			bone_theta += bone_rot_speed * elapsed_seconds;
			left_ear_mat = 
				inverse(monkey_bone_offset_matrices[left_ear_bone_id]) *
				rotate_z_deg(identity_mat4(), bone_theta) *
				monkey_bone_offset_matrices[left_ear_bone_id];		//ボーン位置中心に回転するよう、ボーン位置のオフセットを使う
			glUseProgram(shader_programme);
			glUniformMatrix4fv(bone_matrices_locations[left_ear_bone_id], 1, GL_FALSE, left_ear_mat.m);
		}
		if (glfwGetKey(g_window, 'X'))
		{
			bone_theta -= bone_rot_speed * elapsed_seconds;
			left_ear_mat =
				inverse(monkey_bone_offset_matrices[left_ear_bone_id]) *
				rotate_z_deg(identity_mat4(), bone_theta) *
				monkey_bone_offset_matrices[left_ear_bone_id];		//ボーン位置中心に回転するよう、ボーン位置のオフセットを使う
			glUseProgram(shader_programme);
			glUniformMatrix4fv(bone_matrices_locations[left_ear_bone_id], 1, GL_FALSE, left_ear_mat.m);
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
