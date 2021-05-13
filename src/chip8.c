#include "../includes/chip8.h"
#include "../includes/renderer.h"
#include <string.h>

// Our emulator should first fetch one instruction (2 byte) from the loaded rom (whole rom is loaded at once in memory of chip8)
// And then it should decode it // Don't know difference between decoding and executing though
// And finally execute it 

void initialize_chip8_emulator(chip8_emulator* chip8)
{
	// First load the sprite data of 0-F in the memory at address 0x100. Space less than 0x200 is reserved
	for (int i = 0; i <= 0xFFF; ++i)
		chip8->memory[i] = 0;

	// Run the fixed rom already available
	
	chip8->program_counter = 0x200;
	chip8->instruction_register = 0;

	// Initialize all the 8 bit registers
	for (int i = 0; i < 16; ++i)
		chip8->registers.reg[i] = 0;

	chip8->stack_pointer = 0xF00;
	write_sprite_data(chip8);
}

void write_sprite_data(chip8_emulator* chip8)
{
	unsigned char temp[] = {0xF0, 0x90, 0x90, 0x90, 0xF0,
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

	// So each sprite is 8 pixels wide and about 15 pixels in height 
	// So what we really do is we read each byte, sample its bit seperately and bitwise xor with frame_buffer 
	// the starting byte of the sprite is located by chip8_emulators instruction register

	uint8_t current_byte;

	uint8_t x_start = chip8->registers.reg[reg_x];
	uint8_t y_start = chip8->registers.reg[reg_y];
	
	bool flipped = false;
	for (int counter = 0; counter < sprite_height; ++counter)
	{	
		current_byte = chip8->memory[chip8->instruction_register+counter];
		for (int i = 0; i < 8; ++i)
		{
			// Extracts each bit from the byte 
			uint8_t value = (current_byte >> (7 - i)) & 0x1;
			uint16_t index = (y_start * (frame_buffer->width) + (x_start+i) % frame_buffer->width);
			
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
		return ; 
	}

	chip8->current_instruction = 0;
	chip8->current_instruction = chip8->memory[chip8->program_counter + 1];
	chip8->current_instruction |= chip8->memory[chip8->program_counter] << 8;
	chip8->program_counter += 2; 
}

void decode_and_execute(chip8_emulator* chip8, struct frameBuffer* frame_buffer)
{
	// Now a lot of commands
	uint8_t first_hex;
	first_hex = chip8->current_instruction >> 12;

	switch (first_hex)
	{
	case 0: switch (chip8->current_instruction)
			{
			case 0x00E0 : reset_framebuffer(frame_buffer); // Clear the screen 
				break;
			case 0x00EE : // return from the subroutine
				chip8->program_counter = chip8->memory[chip8->stack_pointer + 1];
				chip8->stack_pointer += 1;
				break;
			default : 
				fprintf(stderr, "Invalid instruction starting with 0");
			}
	break;
	case 1: 
	{
		// sets the program counter to 0nnn
		chip8->program_counter = chip8->current_instruction & 0xFFF;
	}
	break;

	case 2 : 
	{
		chip8->memory[chip8->stack_pointer] = chip8->current_instruction & 0xFFF;
		chip8->stack_pointer -= 1; 
	}
	break;

	case 3 : 
	{
		uint8_t reg = (chip8->current_instruction & 0x0F00) >> 8;
		if (chip8->registers.reg[reg] == (chip8->current_instruction & 0xFF))
			chip8->program_counter += 2;
	}
	break;

	case 4 : 
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
		uint8_t last_byte = chip8->current_instruction & 0x00FF;
		switch (last_byte)
		{
		case 0 : 
			chip8->registers.reg[reg_x] = chip8->registers.reg[reg_y];
			break;
		case 1 : 
			chip8->registers.reg[reg_x] |= chip8->registers.reg[reg_y];
			break;
		case 2 : 
			chip8->registers.reg[reg_x] &= chip8->registers.reg[reg_y];
			break;
		case 3 :
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
		}
		break;
	}
		
	}
}