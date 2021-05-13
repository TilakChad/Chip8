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

void handle_key_press(GLFWwindow*, chip8_emulator*);

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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
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
	// Set the keycallback function for exiting
	glfwSetKeyCallback(window, key_callback);

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

	const char* rom_path = "D:\\Computer\\C\\Chip-8\\Chip8\\roms\\Stars.ch8";
	initialize_chip8_emulator(&chip8,rom_path);
	double now = 0, then = 0;
	glfwSwapInterval(0);
	int counter = 0;
	double timer = 0;
	float deltaTime = 0;
	while (!glfwWindowShouldClose(window))
	{

		glUseProgram(render_engine.shader_program);
		if (counter >= 700 && timer < 1.0f)
		{
			// Do not execute anything else
		}
		else if (counter >= 700 && timer > 1.0f)
		{
			// reset counter and timer 
			counter = 0; 
			timer = 0;
		}
		else
		{
			counter++;

			fetch_instruction(&chip8);
			decode_and_execute(&chip8, &frame_buffer, window);
			if (chip8.should_render)
			{
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
				rendering_loop(&render_engine, &frame_buffer);
				glfwSwapBuffers(window);

			}
		}
		then = glfwGetTime();
		deltaTime = (double)(then - now);
		timer += deltaTime;
		now = then;
		tick(&chip8, deltaTime);
		handle_key_press(window,&chip8);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void handle_key_press(GLFWwindow* window,  chip8_emulator* chip8)
{
	// basic keys W, A, S and D are mapped with 2, 4, 6 and 8 
	int key = 17;
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		key = 1;
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		key = 2;
	else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		key = 3;
	else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		key = 0xC;
	else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		key = 4;
	else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		key = 5;
	else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		key = 6;
	else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
		key = 0xD;
	else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		key = 7;
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		key = 8;
	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		key = 9;
	else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		key = 0xE;
	else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		key = 0;
	else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		key = 0xA;
	else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
		key = 0xB;
	else if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
		key = 0xF;
	chip8->recent_key = key;
}
