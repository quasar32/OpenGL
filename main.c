#include <math.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION

#include <cglm/cglm.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <stb_image.h>

#define SCR_WIDTH 800
#define SCR_HEIGHT 600
#define ASCEPT_RATIO (SCR_WIDTH / (float) SCR_HEIGHT)

#define SENSTIVITY 0.001F

#define VAP_STRIDE (5 * sizeof(float))
#define VAP_OFFSET(off) ((void *) (off * sizeof(float))) 

#define VAPI_POSITION 0
#define VAPI_COORD 1

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, l, h) MIN(MAX(v, l), h)

static vec3 g_cam_pos = {0.0F, 0.0F, 3.0F}; 
static vec3 g_cam_front = {0.0F, 0.0F, -1.0F}; 
static vec3 g_cam_up = {0.0F, 1.0F, 0.0F};
static vec3 g_cam_forw = {0.0F, 0.0F, -1.0F};

static float g_dt = 0.0F;

static void glfw_error(int code, const char *str) 
{
	fprintf(stderr, "glfw error (%d): %s\n", code, str);
	exit(1);
} 

static void resize_callback(GLFWwindow *window, int width, int height) 
{
	glViewport(0, 0, width, height); 
}

static void mouse_callback(GLFWwindow *wnd, double dpos_x, double dpos_y) 
{
	static int is_init;
	static float last_x, last_y;
	static float yaw = -M_PI / 2;
	static float pitch;

	float pos_x = dpos_x;
	float pos_y = dpos_y;
	float off_x;
	float off_y;
	
	float c; 
	float s;

	if (!is_init) {
		last_x = pos_x;
		last_y = pos_y;
		is_init = 1;
	}

	off_x = pos_x - last_x;
	off_y = last_y - pos_y;
	off_x *= SENSTIVITY;
	off_y *= SENSTIVITY;
	last_x = pos_x;
	last_y = pos_y;

	yaw += off_x; 
	pitch += off_y;

	pitch = CLAMP(pitch, -1.55F, 1.55F);

	c = cosf(yaw);
	s = sinf(yaw);

	g_cam_front[0] = c * cosf(pitch);
	g_cam_front[1] = sinf(pitch);
	g_cam_front[2] = s * cosf(pitch);
	glm_vec3_normalize(g_cam_front);

	g_cam_forw[0] = c; 
	g_cam_forw[2] = s; 
	glm_vec3_normalize(g_cam_forw);
}

static void read_all_as_str(const char *path, char *str, size_t size) 
{
	size_t read;
	FILE *f;

	read = 0;
	f = fopen(path, "r");
	if (f) {
		read = fread(str, 1, size - 1, f); 
		fclose(f);
	}
	str[read] = '\0';
}

static void prog_print(const char *msg, int prog) 
{
	char err[1024];
	glGetProgramInfoLog(prog, sizeof(err), NULL, err); 
	fprintf(stderr, "gl error: %s: %s\n", msg, err);
}

static void prog_printf(const char *fmt, int prog, ...) 
{
	va_list ap;
	char msg[1024];

	va_start(ap, prog);
	vsprintf_s(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	prog_print(msg, prog);
}

static GLuint compile_shader(GLenum type, const char *path) 
{
	GLuint shader;
	char buf[4096];
	const char *src;
	int success;

	shader = glCreateShader(type);
	read_all_as_str(path, buf, sizeof(buf));
	src = buf;
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		prog_printf("\"%s\" compilation failed", shader, path);
	}
	return shader;
}

static GLuint link_shaders(int vs, int fs) 
{
	GLuint prog;
	int success;

	prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if (!success) {
		prog_print("program link failed", prog);
	}
	glDeleteShader(vs);
	glDeleteShader(fs);
	return prog;
}

static GLuint create_program(void) 
{
	GLuint vs, fs;
	vs = compile_shader(GL_VERTEX_SHADER, "vertex.vs");
	fs = compile_shader(GL_FRAGMENT_SHADER, "fragment.fs"); 
	return link_shaders(vs, fs);
}

static void add_attrib(GLuint i, GLint size, GLsizei off) 
{
	glVertexAttribPointer(i, size, GL_FLOAT, GL_FALSE, VAP_STRIDE, VAP_OFFSET(off));
	glEnableVertexAttribArray(i);
}

static GLuint load_texture(GLuint prog, const char *path, GLint i, GLint fmt) 
{
	GLuint tex;
	int width, height, channels;
	stbi_uc *data;
	char uniform[32];
	GLint loc;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	data = stbi_load(path, &width, &height, &channels, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 
				0, fmt, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);
	} else {
		fprintf(stderr, "\"%s\" could not be loaded\n", path);
	}

	sprintf(uniform, "tex%d", i);
	loc = glGetUniformLocation(prog, uniform); 
	glUniform1i(loc, i);

	return tex;
}

static GLFWwindow *init_glfw(void) 
{
	GLFWwindow *wnd;

	glfwSetErrorCallback(glfw_error);
	glfwInit();
	atexit(glfwTerminate);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	wnd = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL", NULL, NULL);
	glfwMakeContextCurrent(wnd);
	glfwSetFramebufferSizeCallback(wnd, resize_callback);
	glfwSetCursorPosCallback(wnd, mouse_callback); 
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		fprintf(stderr, "glad error\n");
		exit(1);
	}
	return wnd; 
}

static float rand_normal_float(void) 
{
	return rand() / (float) RAND_MAX;
}

static float rand_float(float start, float end) 
{
	float dif = end - start;
	return rand_normal_float() * dif + start;
}

static void rand_vec3(vec3 v) 
{
	v[0] = rand_float(-2.0F, 2.0F);
	v[1] = rand_float(-2.0F, 2.0F);
	v[2] = rand_float(-5.0F, 0.0F);
}

static void set_up_cube_positions(vec3 *positions, size_t count) 
{
	size_t n = count;
	vec3 *pos = positions;

	while (n-- > 0) {
		rand_vec3(*pos++);
	}
}

static void send_cube_poisitons(vec3 *positions, size_t count, 
		GLuint prog, GLuint model_loc) 
{
	int i;

	for (i = 0; i < count; i++) {
		vec3 axis = {1.0F, 0.3F, 0.5F}; 
		mat4 model;
		float rot; 

		glm_mat4_identity(model);
		glm_translate(model, positions[i]);
		rot = 0.35F * i;
		glm_rotate(model, rot, axis); 
		glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float *) model);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);
	}
}

static void get_view_projection(mat4 view) 
{
	vec3 cam_center; 
	glm_vec3_add(g_cam_pos, g_cam_front, cam_center);
	glm_lookat(g_cam_pos, cam_center, g_cam_up, view);
}

static void move_cam_z(float speed) 
{
	vec3 delta;
	glm_vec3_scale(g_cam_forw, speed, delta);
	glm_vec3_add(g_cam_pos, delta, g_cam_pos);
}

static void move_cam_x(float speed) 
{
	vec3 delta;
	glm_vec3_cross(g_cam_forw, g_cam_up, delta);
	glm_vec3_normalize(delta);
	glm_vec3_scale(delta, speed, delta);
	glm_vec3_add(g_cam_pos, delta, g_cam_pos);
}

static void process_input(GLFWwindow *wnd) 
{
	float cam_speed = 2.5F * g_dt;
	if (glfwGetKey(wnd, GLFW_KEY_W) == GLFW_PRESS) {
		move_cam_z(cam_speed);
	}
	if (glfwGetKey(wnd, GLFW_KEY_A) == GLFW_PRESS) {
		move_cam_x(-cam_speed);
	}
	if (glfwGetKey(wnd, GLFW_KEY_S) == GLFW_PRESS) {
		move_cam_z(-cam_speed);
	}
	if (glfwGetKey(wnd, GLFW_KEY_D) == GLFW_PRESS) {
		move_cam_x(cam_speed);
	}
}

int main(void) 
{
	static const float vertices[] = {
		-0.5F, -0.5F, -0.5F,  0.0F,  0.0F, 
		 0.5F, -0.5F, -0.5F,  1.0F,  0.0F, 
		 0.5F,  0.5F, -0.5F,  1.0F,  1.0F, 
		-0.5F,  0.5F, -0.5F,  0.0F,  1.0F, 
		-0.5F, -0.5F,  0.5F,  0.0F,  0.0F, 
		 0.5F, -0.5F,  0.5F,  1.0F,  0.0F, 
		 0.5F,  0.5F,  0.5F,  1.0F,  1.0F, 
		-0.5F,  0.5F,  0.5F,  0.0F,  1.0F, 
		-0.5F,  0.5F,  0.5F,  1.0F,  0.0F, 
		-0.5F,  0.5F, -0.5F,  1.0F,  1.0F, 
		-0.5F, -0.5F, -0.5F,  0.0F,  1.0F, 
		 0.5F,  0.5F,  0.5F,  1.0F,  0.0F, 
		 0.5F, -0.5F, -0.5F,  0.0F,  1.0F, 
		 0.5F, -0.5F,  0.5F,  0.0F,  0.0F, 
		 0.5F, -0.5F, -0.5F,  1.0F,  1.0F, 
		-0.5F,  0.5F,  0.5F,  0.0F,  0.0F 
	};

	static const unsigned int indices[] = {
		 0,  1,  2, 
		 2,  3,  0, 
		 4,  5,  6, 
		 6,  7,  4, 
		 8,  9, 10, 
		10,  4,  8, 
		11,  2, 12, 
		12, 13, 11, 
		10, 14,  5, 
		 5,  4, 10, 
		 3,  2, 11, 
		11, 15,  3, 
	};

	srand(time(NULL));

	GLFWwindow *wnd;	
	GLuint prog;
	GLuint vao, vbo, ebo;
	GLuint tex0, tex1;

	GLuint model_loc;
	GLuint view_loc; 
	GLuint projection_loc;

	vec3 cube_positions[10]; 

	float last_frame;

	mat4 projection;

	wnd = init_glfw();

	/*init gl*/
	glEnable(GL_DEPTH_TEST);
	prog = create_program();

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

	add_attrib(VAPI_POSITION, 3, 0);
	add_attrib(VAPI_COORD, 2, 3);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
		
	glUseProgram(prog);
	tex0 = load_texture(prog, "tex/container.jpg", 0, GL_RGB); 
 	stbi_set_flip_vertically_on_load(1);
	tex1 = load_texture(prog, "tex/awesomeface.png", 1, GL_RGBA);

	/*init data*/
	model_loc = glGetUniformLocation(prog, "model");

	view_loc = glGetUniformLocation(prog, "view");

	projection_loc = glGetUniformLocation(prog, "projection"); 
	glm_mat4_identity(projection);
	glm_perspective(M_PI / 4, ASCEPT_RATIO, 0.1F, 100.0F, projection);
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, (float *) projection);

	set_up_cube_positions(cube_positions, 10);

	/*main loop*/
	last_frame = glfwGetTime();
	while (!glfwWindowShouldClose(wnd)) {
		float cur_frame;
		mat4 view;

		cur_frame = glfwGetTime();
		g_dt = cur_frame - last_frame;
		last_frame = cur_frame; 

		process_input(wnd);

		glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);

		glUseProgram(prog);
		get_view_projection(view);
		glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float *) view);
		glBindVertexArray(vao);
		send_cube_poisitons(cube_positions, 10, prog, model_loc);

		glfwSwapBuffers(wnd);
		glfwPollEvents();
	}

	/*clean up*/
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(prog);
	glfwTerminate();

	return 0;
}
