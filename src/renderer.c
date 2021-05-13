#define _CRT_SECURE_NO_WARNINGS

#include "../includes/renderer.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_X_POINTS 64
#define MAX_Y_POINTS 32

typedef struct Point
{
	float x, y;
} Point;

Point map_screen_ndc(Point scr)
{
	Point ndc;
	ndc.x = 3.0f / 4 * (scr.x - 32) / 32.0f;
	ndc.y = -3.0f / 4 * (scr.y - 16) / 16.0f;
	return ndc;
}


int initialize_renderer(struct Renderer* render_engine, struct frameBuffer* frame_buffer, int width, int height)
{
	// List of vertices need be modified now 
	// float vertices[] = { 0.5f,0.5f,0.5f,-0.5f,-0.5f,-0.5f,-0.5f,0.5 };
	// So the chip8 screen size is 64*32 pixel .. we are going to use a square for a pixel so we will have 65 points in x direction and 33 points in y direction

	struct shader chip8_shader;
	int err = load_shader_from_file(&chip8_shader, "./src/chip8_vertex.vs", "./src/chip8_fragment.fs");
	assert(err != -1);

	// calculate all the points that will be used to render the chip8 emulator
	float* vertices = malloc(2 * sizeof(float) * (MAX_X_POINTS + 1) * (MAX_Y_POINTS + 1));

	// It is the aspect ratio that will be used for uniform border and importantly making the pixel square
	float aspect_ratio = (float)width / height;

	// lets store it in column major order for now 
	int vertex_index = 0;
	for (int y = 0; y < MAX_Y_POINTS + 1; ++y)
	{
		for (int x = 0; x < MAX_X_POINTS + 1; ++x)
		{
			Point p = map_screen_ndc((Point) { x, y});
			vertices[vertex_index++] = p.x ;
			vertices[vertex_index++] = p.y * aspect_ratio/2;
		}
	}

	// Vertices have been serialized .. Now we need to fill element buffer array 

	GLuint VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float) * (MAX_X_POINTS + 1) * (MAX_Y_POINTS + 1), vertices, GL_STATIC_DRAW);

	// This ordering depends upon whether we want to render GL_TRIANGLES or GL_QUADS
	// Seems like GL_QUADS is deprecated .. so going with GL_TRIANGLES and indexed array
	GLuint* index_order = malloc(sizeof(GLuint) * (64 * 32 * 6)); // 6 index for each quads (2 triangles) and total quads ~ total pixels

	// Fixing the ordering might be the hard part
	vertex_index = 0; // Now used as element index
	for (int y = 0; y < MAX_Y_POINTS; ++y)
	{
		for (int x = 0; x < MAX_X_POINTS; ++x)
		{
			index_order[vertex_index++] = y * (MAX_X_POINTS+1) + x;
			index_order[vertex_index++] = y * (MAX_X_POINTS+1) + (x + 1);
			index_order[vertex_index++] = (y + 1) * (MAX_X_POINTS+1) + (x + 1);

			index_order[vertex_index++] = y * (MAX_X_POINTS + 1) + x;
			index_order[vertex_index++] = (y + 1) * (MAX_X_POINTS + 1) + (x + 1);
			index_order[vertex_index++] = (y + 1) * (MAX_X_POINTS+1)+x;

		}
	}

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * MAX_X_POINTS * MAX_Y_POINTS * sizeof(GLuint), index_order, GL_STATIC_DRAW);

	GLuint shaderProgram;
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, chip8_shader.vertex_shader);
	glAttachShader(shaderProgram, chip8_shader.fragment_shader);

	glLinkProgram(shaderProgram);
	GLuint success = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		fprintf(stderr, "\nFailed to link shaderProgram : %s.", infoLog);
		return -1;
	}

	// Delete the shader programs
	glDeleteShader(chip8_shader.fragment_shader);
	glDeleteShader(chip8_shader.vertex_shader);

	// Now change of vertex attrib pointer
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);
	// Unbind the vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
	render_engine->scr_vertex_array = VAO;
	render_engine->shader_program = shaderProgram;

	// Lastly initialize the framebuffer
	frame_buffer->array = malloc(sizeof(GLuint) * (MAX_X_POINTS) * (MAX_Y_POINTS));

	for (int i = 0; i < MAX_X_POINTS * MAX_Y_POINTS; ++i)
		frame_buffer->array[i] = 0;

	frame_buffer->height = MAX_Y_POINTS;
	frame_buffer->width = MAX_X_POINTS;

	for (int i = 0; i < MAX_X_POINTS * MAX_Y_POINTS; ++i)
		frame_buffer->array[i] = 0;

	// Initialized with not possible value

	// Initialize the second vertex array 
	glGenVertexArrays(1, &render_engine->border_vertex_array);
	glBindVertexArray(render_engine->border_vertex_array);

	float k = aspect_ratio / 2;
	float border_vertices[] = { -0.775f, 0.80f*k, 0.775f, 0.80f*k, 0.775f, -0.80f*k, -0.775f, 0.80f*k, 0.775f, -0.8f*k, -0.775f, -0.8f*k };
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(border_vertices), border_vertices, GL_STATIC_DRAW);

	// No need of element buffer 
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, NULL);
	glEnableVertexAttribArray(0);

	free(vertices);

	glBindVertexArray(0);
	return 0;
}



int load_shader_from_file(struct shader* shaders, const char* vertex_shader_path, const char* fragment_shader_path)
{
	FILE* vs, * fs;
	if (!(vs = fopen(vertex_shader_path, "r")))
	{
		fprintf(stderr, "\nFailed to open vertex shader %s.", vertex_shader_path);
		return -1;
	}
	if (!(fs = fopen(fragment_shader_path, "r")))
	{
		fprintf(stderr, "\nFailed to open fragment shader %s.", fragment_shader_path);
		return -1;
	}

	int count = 0;
	while (fgetc(vs) != EOF)
		count++;

	char* vs_source = malloc(sizeof(char) * (count + 1));
	fseek(vs, 0, SEEK_SET);

	int index = 0;
	char ch;
	while ((ch = fgetc(vs)) != EOF)
		vs_source[index++] = ch;

	assert(index <= count);
	printf("\n Index and count are : %d and %d.", index, count);
	vs_source[index] = '\0';

	shaders->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shaders->vertex_shader, 1, &vs_source, NULL);

	// Similarly load the fragment shaders
	count = 0;
	while (fgetc(fs) != EOF) count++;

	fseek(fs, 0, SEEK_SET);
	char* fs_source = malloc(sizeof(char) * (count + 1));
	index = 0;
	while ((ch = fgetc(fs)) != EOF)
		fs_source[index++] = ch;
	assert(index <= count);
	fs_source[index] = '\0';

	shaders->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shaders->fragment_shader, 1, &fs_source, NULL);

	compile_and_log_shaders(shaders, GL_VERTEX_SHADER);
	compile_and_log_shaders(shaders, GL_FRAGMENT_SHADER);
	free(fs_source);
	free(vs_source);
	fclose(fs);
	fclose(vs);
	return 0;
}


void compile_and_log_shaders(struct shader* shaders, int shader_type)
{
	char logInfo[512];
	int success = -1;
	GLuint current_shader = 0;
	if (shader_type == GL_VERTEX_SHADER)
		current_shader = shaders->vertex_shader;
	else if (shader_type == GL_FRAGMENT_SHADER)
		current_shader = shaders->fragment_shader;
	else
	{
		fprintf(stderr, "\nError invalid shader type passed to %s.", __func__);
		return;
	}

	glCompileShader(current_shader);
	glGetShaderiv(current_shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(current_shader, 512, NULL, logInfo);
		if (shader_type == GL_VERTEX_SHADER)
			fprintf(stderr, "\nVertex shader compilation failed : %s.", logInfo);
		else
			fprintf(stderr, "\nFragment shader compilation failed. : %s.", logInfo);
		return;
	}

	if (shader_type == GL_VERTEX_SHADER)
		fprintf(stderr, "\nVertex shader compilation passed.");
	else
		fprintf(stderr, "\nFragment shader compilation passed.");
}

void rendering_loop(struct Renderer* render_engine, struct frameBuffer* frame_buffer)
{
	// Using 1D array as 2D array 
	// render border first 
	glBindVertexArray(render_engine->border_vertex_array);
	glUniform1i(glGetUniformLocation(render_engine->shader_program, "pixel"), 4);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(render_engine->scr_vertex_array);

	for (unsigned int y = 0; y < frame_buffer->height; ++y)
	{
		for (unsigned int x = 0; x < frame_buffer->width; ++x)
		{
			glUniform1i(glGetUniformLocation(render_engine->shader_program, "pixel"), *(frame_buffer->array + y * (MAX_X_POINTS)+x));
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * (y * MAX_X_POINTS + x) * 6));
		}
	}
	
}

void reset_framebuffer(struct frameBuffer* frame_buffer)
{
	for (int i = 0; i < MAX_X_POINTS * MAX_Y_POINTS; ++i)
		frame_buffer->array[i] = 0;
}