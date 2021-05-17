#include <stdint.h>
#include "../includes/chip8.h"
#include "../includes/renderer.h"
#include <string.h>
#include <assert.h>
#include <GLFW/glfw3.h>

extern void handle_key_press(GLFWwindow*, chip8_emulator*);


// Our emulator should first fetch one instruction (2 byte) from the loaded rom (whole rom is loaded at once in memory of chip8)
// And then it should decode it // Don't know difference between decoding and executing though
// And finally execute it ..

void initialize_chip8_emulator(chip8_emulator* chip8, const char* rom_path)
{
	// First load the sprite data of 0-F in the memory at address 0x100. Space less than 0x200 is reserved
	for (int i = 0; i <= 0xFFF; ++i)
		chip8->memory[i] = 0;

	// Run the fixed rom already available
	chip8->time_accumulate = 0;
	chip8->program_counter = 0x200;
	chip8->instruction_register = 0;
	chip8->sound_register = 0;
	chip8->delay_register = 0;
	chip8->current_instruction;
	// Initialize all the 8 bit registers
	for (int i = 0; i < 16; ++i)
		chip8->registers.reg[i] = 0;

	chip8->should_render = false;
	chip8->stack_pointer = 0xF00;
	chip8->recent_key = 0;
	load_rom(chip8, rom_path);
	write_sprite_data(chip8);

	// Initialize the sound here 
	chip8->wav_file = cs_load_wav("./includes/airlock.wav");
	chip8->play_sound = cs_make_def(&chip8->wav_file);
}

void write_sprite_data(chip8_emulator* chip8)
{
	unsigned char temp[] = { 0xF0, 0x90, 0x90, 0x90, 0xF0,
							0x20, 0x60, 0x20, 0x20, 0x70,
							0xF0, 0x10, 0xF0, 0x80, 0xF0,
							0xF0, 0x10, 0xF0, 0x10, 0xF0,
							0x90, 0x90, 0xF0, 0x10, 0x10,
							0xF0, 0x80, 0xF0, 0x10, 0xF0,
							0xF0, 0x80, 0xF0, 0x90, 0xF0,
							0xF0, 0x10, 0x20, 0x40, 0x40,
							0xF0, 0x90, 0xF0, 0x90, 0xF0,
							0xF0, 0x90, 0xF0, 0x10, 0xF0,

							0xF0, 0x90, 0xF0, 0x90, 0x90,
							0xE0, 0x90, 0xE0, 0x90, 0xE0,
							0xF0, 0x80, 0x80, 0x80, 0xF0,
							0xE0, 0x90, 0x90, 0x90, 0xE0,
							0xF0, 0x80, 0xF0, 0x80, 0xF0,
							0xF0, 0x80, 0xF0, 0x80, 0x80 };

	int err = memcpy_s(chip8->memory + 0x100, 80, temp, 80);
}

void render_sprite(chip8_emulator* chip8, struct frameBuffer* frame_buffer, int reg_x, int reg_y, int sprite_height)
{
	if (reg_x > 16 || reg_y > 16)
	{
		fprintf(stderr, "\nInvalid register given :-> reg_x -> %d and reg_y -> %d.", reg_x, reg_y);
		return;
	}

	if (sprite_height >= 16 || sprite_height < 0)
	{
		fprintf(stderr, "\nInvalid sprite size : %d.", sprite_height);
		return;
	}
	// So each sprite is 8 pixels wide and about 15 pixels in height 
	// So what we really do is we read each byte, sample its bit seperately and bitwise xor with frame_buffer 
	// the starting byte of the sprite is located by chip8_emulators instruction register

	uint8_t current_byte;

	uint8_t x_start = chip8->registers.reg[reg_x];
	x_start = x_start % frame_buffer->width;

	uint8_t y_start = chip8->registers.reg[reg_y];
	y_start = y_start % frame_buffer->height;
	bool flipped = false;
	for (int counter = 0; counter < sprite_height; ++counter)
	{
		current_byte = chip8->memory[chip8->instruction_register + counter];
		for (int i = 0; i < 8; ++i)
		{
			// Extracts each bit from the byte 
			uint8_t value = (current_byte >> (7 - i)) & 0x1;
			uint16_t index = (y_start * (frame_buffer->width) + (x_start + i) % frame_buffer->width);

			if (!flipped)
			{
				if (frame_buffer->array[index] == 1 && value == 1)
					flipped = true;
			}

			if (value == frame_buffer->array[index])
				frame_buffer->array[index] = 0;
			else
				frame_buffer->array[index] = 1;
		}
		y_start++;
		if (y_start >= frame_buffer->height)
			y_start -= frame_buffer->height;
	}

	if (flipped)
		chip8->registers.reg[0xF] = 1;
	else
		chip8->registers.reg[0xF] = 0;

}

void fetch_instruction(chip8_emulator* chip8)
{
	// This function ain't doing anything fancy .. just read next 2 bytes and store in current_instruction 
	if (chip8->program_counter > 0xFFF)
	{
		fprintf(stderr, "Program counter going out of the memory.. . So can't fetch any further instruction ");
		return;
	}

	chip8->current_instruction = 0;
	chip8->current_instruction = chip8->memory[chip8->program_counter + 1];
	chip8->current_instruction |= ((uint16_t)chip8->memory[chip8->program_counter]) << 8;
	chip8->program_counter += 2;
}

void decode_and_execute(chip8_emulator* chip8, struct frameBuffer* frame_buffer, GLFWwindow* window)
{
	// Now a lot of commands
	uint8_t first_hex;
	first_hex = chip8->current_instruction >> 12;

	chip8->should_render = false;
	// fprintf(stderr,"\nExecuting instruction -> %04x.", chip8->current_instruction);
	switch (first_hex)
	{
	case 0: switch (chip8->current_instruction & 0x0FFF)
	{
	case 0x00E0: reset_framebuffer(frame_buffer); // Clear the screen 
		chip8->should_render = true;
		break;
	case 0x00EE: // return from the subroutine
		// Retrieve the program counter which was stored in the network order
	{
		uint16_t PC = 0;
		PC = chip8->memory[chip8->stack_pointer + 2];
		PC = (PC << 8) | chip8->memory[chip8->stack_pointer + 1];
		chip8->program_counter = PC;
		chip8->stack_pointer += 2;
	}
		break;
	default:
		fprintf(stderr, "\nInvalid instruction starting with 0");
		assert(!"Invalid");
	}
		  break;
	case 1:
	{
		// sets the program counter to 0nnn
		chip8->program_counter = chip8->current_instruction & 0xFFF;
	}
	break;

	case 2:
	{
		/*chip8->memory[chip8->stack_pointer] = chip8->current_instruction & 0xFFF;
		chip8->stack_pointer -= 1;*/
		// stack pointer will use two successive memory and store the address in network order 
		// it should call the subroutine .. to call subroutine the pc is set to top of stack 
		chip8->memory[chip8->stack_pointer] = (chip8->program_counter & 0xFF00) >> 8;
		chip8->memory[chip8->stack_pointer - 1] = chip8->program_counter & 0x00FF;
		chip8->stack_pointer -= 2;
		chip8->program_counter = chip8->current_instruction & 0x0FFF;
	}
	break;

	case 3:
	{
		uint8_t reg = (chip8->current_instruction & 0x0F00) >> 8;
		if (chip8->registers.reg[reg] == (chip8->current_instruction & 0xFF))
			chip8->program_counter += 2;
	}
	break;

	case 4:
	{
		uint8_t reg = (chip8->current_instruction & 0x0F00) >> 8;
		if (chip8->registers.reg[reg] != (chip8->current_instruction & 0xFF))
			chip8->program_counter += 2;
	}
	break;

	case 5:
	{
		uint8_t reg_x, reg_y;
		reg_x = (chip8->current_instruction & 0x0F00) >> 8;
		reg_y = (chip8->current_instruction & 0x00F0) >> 4;
		if (chip8->registers.reg[reg_x] == chip8->registers.reg[reg_y])
			chip8->program_counter += 2;
	}
	break;

	case 6:
	{
		uint8_t reg = (chip8->current_instruction & 0x0F00) >> 8;
		chip8->registers.reg[reg] = chip8->current_instruction & 0x00FF;
	}
	break;

	case 7:
	{
		uint8_t reg = (chip8->current_instruction & 0x0F00) >> 8;
		chip8->registers.reg[reg] += chip8->current_instruction & 0x00FF;
	}
	break;

	case 8:
	{
		uint8_t reg_x = (chip8->current_instruction & 0x0F00) >> 8;
		uint8_t reg_y = (chip8->current_instruction & 0x00F0) >> 4;
		uint8_t last_byte = chip8->current_instruction & 0x000F;
		switch (last_byte)
		{
		case 0:
			chip8->registers.reg[reg_x] = chip8->registers.reg[reg_y];
			break;
		case 1:
			chip8->registers.reg[reg_x] |= chip8->registers.reg[reg_y];
			break;
		case 2:
			chip8->registers.reg[reg_x] &= chip8->registers.reg[reg_y];
			break;
		case 3:
			chip8->registers.reg[reg_x] ^= chip8->registers.reg[reg_y];
			break;
		case 4:
		{
			uint16_t sum = chip8->registers.reg[reg_x] + chip8->registers.reg[reg_y];
			if (sum > UINT8_MAX)
				chip8->registers.reg[0xF] = 1;
			else
				chip8->registers.reg[0xF] = 0;
			// Ok .. unsigned number overflow is not undefined 
			chip8->registers.reg[reg_x] += chip8->registers.reg[reg_y];
		}
		break;
		case 5:
		{
			if (chip8->registers.reg[reg_x] > chip8->registers.reg[reg_y])
				chip8->registers.reg[0xF] = 1;
			else
				chip8->registers.reg[0xF] = 0;
			chip8->registers.reg[reg_x] -= chip8->registers.reg[reg_y];
		}
		break;
		case 6:
		{
			chip8->registers.reg[0xF] = chip8->registers.reg[reg_x] & 0x0001;
			chip8->registers.reg[reg_x] >>= 1;
		}
		break;
		case 7:
		{
			if (chip8->registers.reg[reg_y] > chip8->registers.reg[reg_x])
				chip8->registers.reg[0xF] = 1;
			else
				chip8->registers.reg[0xF] = 0;
			chip8->registers.reg[reg_x] = chip8->registers.reg[reg_y] - chip8->registers.reg[reg_x];
		}
		break;
		case 0xE:
		{
			chip8->registers.reg[0xF] = (chip8->registers.reg[reg_x] & 0x80) >> 7;
			chip8->registers.reg[reg_x] <<= 1;
		}
		break;
		default:
			fprintf(stderr, "\nUnknown instructions set... Last nibble of 8xy is invalid ..%x.", last_byte);
			assert(!"Invalid");

		}
		break;
	}
	break;
	case 9:
	{
		if ((chip8->current_instruction & 0x000F) != 0x0)
		{
			fprintf(stderr, "\nInvalid instruction 9xy%x.", chip8->current_instruction & 0x000F);
			break;
		}
		uint8_t reg_x = (chip8->current_instruction & 0x0F00) >> 8;
		uint8_t reg_y = (chip8->current_instruction & 0x00F0) >> 4;
		if (chip8->registers.reg[reg_x] != chip8->registers.reg[reg_y])
			chip8->program_counter += 2;
	}
	break;
	case 0xA:
	{
		chip8->instruction_register = chip8->current_instruction & 0xFFF;
	}
	break;
	case 0xB:
	{
		chip8->program_counter = (chip8->current_instruction & 0xFFF) + chip8->registers.reg[0x0];
	}
	break;
	case 0xC:
	{
		clock_t random = clock();
		uint8_t random_byte = random & 0xFF;
		uint8_t reg = (chip8->current_instruction & 0x0F00) >> 8;
		chip8->registers.reg[reg] = random_byte & (chip8->current_instruction & 0xFF);
	}
	break;
	case 0xD:
	{
		uint8_t sprite_size = chip8->current_instruction & 0x000F;
		uint8_t reg_x = (chip8->current_instruction & 0x0F00) >> 8;
		uint8_t reg_y = (chip8->current_instruction & 0x00F0) >> 4;
		render_sprite(chip8, frame_buffer, reg_x, reg_y, sprite_size);
		chip8->should_render = true;
	}
	break;

	// Next up we need to do some keymapping 
	case 0xE:
	{
		uint8_t reg_x = (chip8->current_instruction & 0x0F00) >> 8;
		uint16_t last_byte = chip8->current_instruction & 0x00FF;
		switch (last_byte)
		{
		case 0x9E:
			//if (chip8->key_pressed[chip8->registers.reg[reg_x]])
			//{
			//	chip8->key_pressed[chip8->registers.reg[reg_x]] = 0; // Reset the key 
			//	chip8->program_counter += 2; 
			//}
			//break;
			if (chip8->recent_key < 17)
			{
				if (chip8->recent_key == chip8->registers.reg[reg_x])
				{
					chip8->program_counter += 2;
					chip8->recent_key = 17;
				}
			}
			break;
		case 0xA1:
			/*if (!chip8->key_pressed[chip8->registers.reg[reg_x]])
				chip8->program_counter += 2;*/
			if (chip8->recent_key != chip8->registers.reg[reg_x])
			{
				chip8->program_counter += 2;
			}

			break;
		default:
			fprintf(stderr, "\nUnknown instruction 0xE%3x", chip8->current_instruction & 0xFFF);
			assert(!"Invalid");

		}
	}

	break;
	case 0xF:
	{
		uint8_t reg_x = (chip8->current_instruction & 0x0F00) >> 8;
		uint8_t last_byte = chip8->current_instruction & 0x00FF;

		switch (last_byte)
		{
		case 0x07:
			chip8->registers.reg[reg_x] = chip8->delay_register;
			break;
		case 0x0A:
			// Delay the timing process 
		{
			// This function needs a complete rewrite
			//fprintf(stderr, "\nKey press might be required .. ");
			//// Clear all the key_pressed 

			//for (int i = 0; i < 16; ++i)
			//	chip8->key_pressed[i] = 0;

			//handle_key_press(chip8, window);
			//// Run the infinite loop which exits on any key pressed
			//int key = 0;
			//while (1)
			//{
			//	fprintf(stderr, "\nWaiting for key press...");
			//	if (chip8->key_pressed[key])
			//		break;
			//	key++;
			//	if (key >= 16)
			//		key -= 16;
			//}
			//// Store the obtained key into the register Vx
			//chip8->registers.reg[reg_x] = key;
			chip8->recent_key = 17;
			while (chip8->recent_key == 17)
			{
				fprintf(stderr, "\nWaiting for delaying time key press...");
				glfwPollEvents();
				handle_key_press(window, chip8);
			}
			chip8->registers.reg[reg_x] = chip8->recent_key;

		}
		break;
		case 0x15:
			chip8->delay_register = chip8->registers.reg[reg_x];
			break;
		case 0x18:
			chip8->sound_register = chip8->registers.reg[reg_x];
			break;
		case 0x1E:
			chip8->instruction_register += chip8->registers.reg[reg_x];
			break;
		case 0x29:
		{
			// Location of sprite of digit Vx
			// Our digit location are stored at address 0x100
			uint16_t location = 0x100 + chip8->registers.reg[reg_x] * 5;
			chip8->instruction_register = location;
		}
			break;
		case 0x33:
		{
			uint8_t value = chip8->registers.reg[reg_x];
			// write BCD code of the value starting at location I
			chip8->memory[chip8->instruction_register] = value / (uint8_t)100;
			value = value % 100;
			chip8->memory[chip8->instruction_register + 1] = value / (uint8_t)10;
			value = value % 10;
			chip8->memory[chip8->instruction_register + 2] = value;
		}
		break;
		case 0x55:
		{
			for (uint8_t i = 0; i <= reg_x; ++i)
			{
				chip8->memory[chip8->instruction_register + i] = chip8->registers.reg[i];
			}
		}
		break;
		case 0x65:
		{
			for (uint8_t i = 0; i <= reg_x; ++i)
			{
				chip8->registers.reg[i] = chip8->memory[chip8->instruction_register + i];
			}
		}
		break;
		default:
			fprintf(stderr, "\nInvalid instruction 0xF%03x.", chip8->instruction_register & 0x0FFF);
			assert(!"Invalid");

			break;
		}
	}
	}
	// implement delay here 
	/*while (chip8->delay_register >= 5)
	{
		for (int i = 0; i < 1000000; ++i);
		chip8->delay_register -= 1;
	}*/
}

void load_rom(chip8_emulator* chip8, const char* rom_path)
{
	FILE* fp;
	if (fopen_s(&fp, rom_path, "rb"))
	{
		fprintf(stderr, "\nFailed to open %s.", rom_path);
		return;
	}

	fseek(fp, 0, SEEK_END);
	long bytes = ftell(fp);
	fprintf(stderr, "\nTotal bytes read were > %ld.", bytes);
	assert(bytes + 0x200 < 0xFFF);
	fseek(fp, 0, SEEK_SET);

	// it will load rom into the chip8 internal memory
	int k = fread(chip8->memory + 0x200, sizeof(uint8_t), bytes, fp);
	fprintf(stderr, "\nActually read bytes were %d.\n", k);
	fclose(fp);
}

void tick(chip8_emulator* chip8, float deltaTime)
{
	//fprintf(stderr, "\nDelta Time is : %0.5f and delay timer is %hu.", deltaTime * 60,chip8->delay_register);
	chip8->time_accumulate += deltaTime;
	if (chip8->time_accumulate >= 1.0f / 60)
	{
		if (chip8->delay_register != 0)
			chip8->delay_register--;
		if (chip8->sound_register != 0)
		{
			cs_play_sound(chip8->sound_context, chip8->play_sound);
			chip8->sound_register--;
			fprintf(stderr, "Output from sound register is : %hu.", chip8->sound_register);
		}
		chip8->time_accumulate -= 1.0f / 60;
	}
	if (chip8->time_accumulate < 0.2f)
		cs_mix(chip8->sound_context);

	// fprintf(stderr, "\nFPS is -> %f.", 1 / deltaTime);
}
