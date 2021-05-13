#define _CRT_SECURE_NO_WARNINGS

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "../includes/renderer.h"
#include "../includes/chip8.h"
#include <time.h>
typedef struct UserPointer
{
	struct Renderer* render_engine;
	struct frameBuffer* frame_buffer;
} UserPointer;

// Borderline around the box might be needed 
// Aspect ratio is the main concern here 

void onSizeChange(GLFWwindow* window, int width, int height)
{
	// Need to update the data sent to the gpu buffer and reinitialize the renderer
	// So basically what we want is : Need two aspect ratios.. 
	//		-> The first one is the window aspect ratio 
	//		-> The second one is the game aspect ratio
	// 		-> The ideal aspect ratio of the game is 2 : 1
	// Its my first time working on these kinda stuffs.. So might take some mathy and some hit and trial expreimentation
	// Its all for the sake of making pixel square nothing much 
	UserPointer* get_ptr = (UserPointer*)glfwGetWindowUserPointer(window);
	glViewport(0, 0, width, height);
	initialize_renderer(get_ptr->render_engine, get_ptr->frame_buffer,width,height);
}

int main(int argc, char** argv)
{
	// Initializes the library
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to load GLFW api. Exiting...");
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1200, 800, "Chip8", NULL, NULL);
	if (!window)
	{
		fprintf(stderr, "Failed to create the window.. Exiting..");
		glfwTerminate();
		return -2;
	}

	glfwSetFramebufferSizeCallback(window, onSizeChange);
	// Make the current context, window's context 
	glfwMakeContextCurrent(window);

	// Load the glad library
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fputs("Error laoding GLAD library.. ", stderr);
		return -3;
	}

	struct Renderer render_engine;
	struct frameBuffer frame_buffer;

	UserPointer ptr;
	ptr.frame_buffer = &frame_buffer;
	ptr.render_engine = &render_engine;

	// Set glfw user pointer
	glfwSetWindowUserPointer(window, &ptr);

	initialize_renderer(&render_engine,&frame_buffer,1200,800);
	chip8_emulator chip8;

	initialize_chip8_emulator(&chip8);
	chip8.registers.reg[0] = 25;
	chip8.registers.reg[1] = 30;
	chip8.instruction_register = 0x132;
	render_sprite(&chip8, &frame_buffer, 0, 1, 5);

	while (!glfwWindowShouldClose(window))
	{
		glUseProgram(render_engine.shader_program);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		fetch_instruction(&chip8);

		rendering_loop(&render_engine, &frame_buffer);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;

}
