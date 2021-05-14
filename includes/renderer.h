#pragma once
#ifndef RENDERER_H
#define RENDERER_H
#include <glad/glad.h>
#include <time.h>
#include <stdint.h>

struct shader
{
	GLuint vertex_shader;
	GLuint fragment_shader;
};

struct Renderer
{
	GLuint scr_vertex_array;
	GLuint border_vertex_array;
	GLuint shader_program;
};

struct frameBuffer
{
	GLuint *array;
	GLuint width;
	GLuint height;
};


int initialize_renderer(struct Renderer*, struct frameBuffer*, int ,int);
void compile_and_log_shaders(struct shader*, int shader_type);
int load_shader_from_file(struct shader* shaders, const char* vertex_shader_path, const char* fragment_shader_path);
void rendering_loop(struct Renderer* render_engine, struct frameBuffer* frame_buffer);
void reset_framebuffer(struct frameBuffer*);

#endif
