gcc ./src/main.c -I./glad/include -I./GLFW/include ./glad/src/glad.c ./src/renderer.c ./GLFW/src/context.c -D_GLFW_WIN32 ./GLFW/src/window.c ./GLFW/src/init.c ./GLFW/src/input.c -std=c17 ./src/chip8.c ./GLFW/src/win32_thread.c ./GLFW/src/win32_window.c ./GLFW/src/win32_monitor.c ./GLFW/src/vulkan.c ./GLFW/src/monitor.c ./GLFW/src/win32_init.c ./GLFW/src/win32_time.c ./GLFW/src/win32_joystick.c -lkernel32 -luser32 ./GLFW/src/wgl_context.c ./GLFW/src/egl_context.c ./GLFW/src/osmesa_context.c -lopengl32 -lgdi32 -o chip8