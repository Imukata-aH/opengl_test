#version 410

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_colour;
layout(location = 3) in int bone_id;

uniform mat4 view, proj, model;
uniform mat4 bone_matrices[64];

out vec3 colour;

void main() {
	//colour = vertex_colour;
	colour = vec3(0.0, 0.0, 0.0);
	if (bone_id == 0){
		colour.r = 1.0;
	}
	else if (bone_id == 1){
		colour.g = 1.0;
	}
	else if (bone_id == 2){
		colour.b = 1.0;
	}

	gl_Position = proj * view * model * bone_matrices[bone_id] * vec4(vertex_position, 1.0);
}
