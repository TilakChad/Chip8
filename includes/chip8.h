#pragma once
#ifndef CHIP8_H
#define CHIP8_H
#include <stdint.h>
#include "renderer.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct chip8_emulator
{
	uint8_t memory[4096];
	struct { uint8_t reg[16]; } registers; // 16 general purpose registers
	uint16_t instruction_register; // instruction pointer
	uint16_t program_counter; 
	uint8_t sound_register;
	uint8_t delay_register;
	uint16_t stack_pointer; 
	uint16_t current_instruction;

} chip8_emulator;

void initialize_chip8_emulator(chip8_emulator* chip8);
void write_sprite_data(chip8_emulator*);
void render_sprite(chip8_emulator*,struct frameBuffer*, int, int,int);

void fetch_instruction(chip8_emulator*);
void decode_and_execute(chip8_emulator*, struct frameBuffer* );
#endif