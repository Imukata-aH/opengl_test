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

mat4 g_local_anim[MAX_BONES];

// �X�P���g���\�����ċA�I�ɒH���āA�{�[���̃A�j���[�V�����s��̔z��𐶐�����
void skeleton_animate(
	Skeleton_Node* node,
	mat4 parent_mat,
	mat4* bone_offset_mats,
	mat4* bone_animation_mats){

	assert(node);

	mat4 our_mat = parent_mat;			// �{�[���̍ŏI�I�ȃg�����X�t�H�[���s��
	mat4 local_anim = identity_mat4();	// ���̃t���[���ɂ����邱�̃{�[���ɑ΂���A�j���[�V�����s��

	int bone_i = node->bone_index;
	if (bone_i > -1)
	{
		mat4 bone_offset = bone_offset_mats[bone_i];
		mat4 inv_bone_offset = inverse(bone_offset);
		local_anim = g_local_anim[bone_i];

		our_mat = parent_mat * inv_bone_offset * local_anim * bone_offset;
		bone_animation_mats[bone_i] = our_mat;
	}
	for (int i = 0; i < node->num_children; i++)
	{
		skeleton_animate(
			node->children[i],
			our_mat,
			bone_offset_mats,
			bone_animation_mats
			);
	}
}

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
	Skeleton_Node* monkey_skeleton_root;
	int monkey_point_count = 0;
	int monkey_bone_count = 0;
	assert(load_mesh(MESH_FILE, &monkey_vao, &monkey_point_count, monkey_bone_offset_matrices, &monkey_bone_count, &monkey_skeleton_root));
	printf("%s bone count: %i\n", MESH_FILE, monkey_bone_count);

	mat4 monkey_bone_animation_mats[MAX_BONES];
	for (int i = 0; i < MAX_BONES; i++) {
		monkey_bone_animation_mats[i] = identity_mat4();
		monkey_bone_offset_matrices[i] = identity_mat4();
		monkey_bone_animation_mats[i] = identity_mat4();
		g_local_anim[i] = identity_mat4();
	}

	// bone�ʒu�m�F�p�̃o�b�t�@�쐬�ƃ{�[���ʒu�s��̕\��
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
	// bone matrices�@OpenGL�̎����ł́Auniform�ϐ��̔z���location�l�͘A���Ƃ͌���Ȃ��̂ŁA���ꂼ��̃C���f�b�N�X�ɑ΂���location��T������
	int bone_matrices_locations[MAX_BONES];
	char name[64];
	for (int i = 0; i < MAX_BONES; i++)
	{
		sprintf(name, "bone_matrices[%i]", i);
		bone_matrices_locations[i] = glGetUniformLocation(shader_programme, name);
		glUniformMatrix4fv(bone_matrices_locations[i], 1, GL_FALSE, identity_mat4().m);	// �P�ʍs��ŏ�����
	}

	// �{�[���ʒu��\�����邽�߂̃V�F�[�_��Uniform�ϐ��ɒl���Z�b�g
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

	// �X�P���g���A�j���[�V�����p�p�����[�^
	float bone_theta = 0.0f;
	float bone_y = 0.0f;
	float bone_rot_speed = 50.0f;

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

		// �{�[���ʒu��`��
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
		bool monkey_moved = false;
		if (glfwGetKey(g_window, 'Z')){
			bone_theta += bone_rot_speed * elapsed_seconds;
			g_local_anim[1] = rotate_z_deg(identity_mat4(), bone_theta);
			g_local_anim[2] = rotate_z_deg(identity_mat4(), -bone_theta);
			monkey_moved = true;
		}
		if (glfwGetKey(g_window, 'X')){
			bone_theta -= bone_rot_speed * elapsed_seconds;
			g_local_anim[1] = rotate_z_deg(identity_mat4(), bone_theta);
			g_local_anim[2] = rotate_z_deg(identity_mat4(), -bone_theta);
			monkey_moved = true;
		}
		if (glfwGetKey(g_window, 'C')){
			bone_y += 0.5f * elapsed_seconds;
			g_local_anim[0] = translate(identity_mat4(), vec3(0.0f, bone_y, 0.0f));
			monkey_moved = true;
		}
		if (glfwGetKey(g_window, 'V')){
			bone_y -= 0.5f * elapsed_seconds;
			g_local_anim[0] = translate(identity_mat4(), vec3(0.0f, bone_y, 0.0f));
			monkey_moved = true;
		}
		if (monkey_moved)
		{
			skeleton_animate(
				monkey_skeleton_root,
				identity_mat4(),
				monkey_bone_offset_matrices,
				monkey_bone_animation_mats);
			glUseProgram(shader_programme);
			glUniformMatrix4fv(
				bone_matrices_locations[0],
				monkey_bone_count,
				GL_FALSE,
				monkey_bone_animation_mats[0].m);
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
