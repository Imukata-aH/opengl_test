#version 410

in vec3 position;
uniform mat4 proj, view;

void main() {
	gl_PointSize = 10.0;
	gl_Position = proj * view * vec4(position, 1.0);
}