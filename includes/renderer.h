#pragma once
#ifndef RENDERER_H
#define RENDERER_H
#include <glad/glad.h>

struct shader
{
	GLuint vertex_shader;
	GLuint fragment_shader;
};

struct Renderer
{
	GLuint vertex_array;
	GLuint shader_program;
};

int initialize_renderer(struct Renderer* );
void compile_and_log_shaders(struct shader*, int shader_type);
int load_shader_from_file(struct shader* shaders, const char* vertex_shader_path, const char* fragment_shader_path);

#endif
