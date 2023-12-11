#define _CRT_SECURE_NO_DEPRECATE
#include<iostream>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
// 17August2020:Some problem with my key inut and delay register i guess. Only pong and maze works well 
// 17August2020 UPDATE: VF=0 1 caused problem in maze and tetris!!...not delay!!...but some delay stuff there in tetris
using namespace std;

unsigned char mem[4096];				// Memory heap
unsigned char Vreg[16];					// 16 geeral registers
unsigned char& VF = Vreg[15];
unsigned char Dreg, Sreg;				// 8 BIT Delay and Timer special registers and stack pointer
unsigned short PC,I,SP;					//Program counter I register 16 bits I register lower 12 bits used

unsigned char fontset[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


class Screen: public olc::PixelGameEngine
{
public:
	
	Screen()
	{
		sAppName = "CHIP8";
	}

private:
	bool cycle = false;
	bool dotmode=true;
	bool waitkey = false;
	float fTargetFrameTime = 1.0f / 600.0f; // Virtual FPS of 100fps
	float fAccumulatedTime = 0.0f;
	olc::Sprite* sprite;

public:
	
	// Decoding opcodes
	void decode(unsigned short opcode)
	{
		switch (opcode & 0xF000)
		{
		case 0x0000:
			switch (opcode & 0x000F)
			{
			case 0x0000: // 0x00E0: Clears the screen
				Screen::Clear(olc::BLACK);
				PC += 2;
				break;

			case 0x000E: // 0x00EE: Returns from subroutine
				PC = (mem[SP - 1]<< 8) | mem[SP];
				SP -= 2;
				PC += 2;				//pc += 2;		// Don't forget to increase the program counter!
				break;

			default:
				cout<<"Unknown opcode [0x0000]:"<< int( opcode)<<endl;
			}
			break;

		case 0x1000: // 0x1NNN: Jumps to address NNN
			PC = 0x0FFF & opcode;

			break;

		case 0x2000: // 0x2NNN: Calls subroutine at NNN.
			SP += 1;
			mem[SP] = PC >> 8;
			SP += 1;
			mem[SP] = PC & 0x00FF;					// cant you do type conversion
			PC = 0x0FFF & opcode;
			break;

		case 0x3000: // 0x3XNN: Skips the next instruction if VX equals NN
			Vreg[(opcode & 0x0F00) >> 8] ^ (opcode & 0x00FF) ? PC+=2 : PC += 4;	//PC+=2 or 1?
			break;

		case 0x4000: // 0x4XNN: Skips the next instruction if VX doesn't equal NN
			Vreg[(opcode & 0x0F00) >> 8] ^ (opcode & 0x00FF) ? PC += 4 : PC+=2;	//PC+=2 or 1?
			break;

		case 0x5000: // 0x5XY0: Skips the next instruction if VX equals VY.
			Vreg[(opcode & 0x0F00) >> 8] ^ Vreg[(opcode & 0x00F0) >> 4] ? PC+=2 : PC += 4;
			break;

		case 0x6000: // 0x6XNN: Sets VX to NN.
			Vreg[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
			PC += 2;
			break;

		case 0x7000: // 0x7XNN: Adds NN to VX.
			Vreg[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
			PC += 2;
			break;

		case 0x8000:
			switch (opcode & 0x000F)
			{
			case 0x0000: // 0x8XY0: Sets VX to the value of VY
				Vreg[(opcode & 0x0F00) >> 8] = Vreg[(opcode & 0x00F0) >> 4];
				PC += 2;
				break;

			case 0x0001: // 0x8XY1: Sets VX to "VX OR VY"
				Vreg[(opcode & 0x0F00) >> 8] |= Vreg[(opcode & 0x00F0) >> 4];
				PC += 2;
				break;

			case 0x0002: // 0x8XY2: Sets VX to "VX AND VY"
				Vreg[(opcode & 0x0F00) >> 8] &= Vreg[(opcode & 0x00F0) >> 4];
				PC += 2;
				break;

			case 0x0003: // 0x8XY3: Sets VX to "VX XOR VY"
				Vreg[(opcode & 0x0F00) >> 8] ^= Vreg[(opcode & 0x00F0) >> 4];
				PC += 2;
				break;

			case 0x0004: // 0x8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't					
				(Vreg[(opcode & 0x0F00) >> 8] + Vreg[(opcode & 0x00F0) >> 4]) > 0x00FF ? VF = 1 : VF = 0;	// check overflow Vf set to 1 or ff
				Vreg[(opcode & 0x0F00) >> 8] += Vreg[(opcode & 0x00F0) >> 4];
				PC += 2;
				break;

			case 0x0005: // 0x8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't
				(Vreg[(opcode & 0x0F00) >> 8] >= Vreg[(opcode & 0x00F0) >> 4]) ? VF = 1 : VF = 0;
				Vreg[(opcode & 0x0F00) >> 8] -= Vreg[(opcode & 0x00F0) >> 4];
				PC += 2;
				break;

			case 0x0006: // 0x8XY6: Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift
				Vreg[(opcode & 0x0F00) >> 8] & 0x01 ? VF = 1 : VF = 0;
				Vreg[(opcode & 0x0F00) >> 8]=Vreg[(opcode & 0x0F00) >> 8] >> 1;
				PC += 2;
				break;

			case 0x0007: // 0x8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't
				(Vreg[(opcode & 0x0F00) >> 8] < Vreg[(opcode & 0x00F0) >> 4]) ? VF = 1 : VF = 0;
				Vreg[(opcode & 0x0F00) >> 8] = Vreg[(opcode & 0x00F0) >> 4] - Vreg[(opcode & 0x0F00) >> 8];
				PC += 2;
				break;

			case 0x000E: // 0x8XYE: Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift
				Vreg[(opcode & 0x0F00) >> 8] & 0x01 ? VF = 1: VF = 0;
				Vreg[(opcode & 0x0F00) >> 8] =Vreg[(opcode & 0x0F00) >> 8] << 1;
				PC += 2;
				break;

			default:
				printf("Unknown opcode [0x8000]: 0x%X\n", opcode);
			}
			break;

		case 0x9000: // 0x9XY0: Skips the next instruction if VX doesn't equal VY
			(Vreg[(opcode & 0x0F00) >> 8] ^ Vreg[(opcode & 0x00F0) >> 4]) ? PC += 4 : PC+=2;
			break;

		case 0xA000: // ANNN: Sets I to the address NNN
			I = opcode & 0x0FFF;
			PC += 2;
			break;

		case 0xB000: // BNNN: Jumps to the address NNN plus V0
			PC = (opcode & 0x0FFF) + Vreg[0];
			break;

		case 0xC000: // CXNN: Sets VX to a random number and NN
			Vreg[(opcode & 0x0F00) >> 8] = (rand() % 0xFF)&(opcode & 0x00ff);
			PC += 2;
			break;

		case 0xD000: // DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. 
					 // Each row of 8 pixels is read as bit-coded starting from memory location I; 
					 // I value doesn't change after the execution of this instruction. 
					 // VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, 
					 // and to 0 if that doesn't happen
		{
			drawSprite(opcode & 0x000F, Vreg[(opcode & 0x0F00) >> 8], Vreg[(opcode & 0x00F0) >> 4]);
			PC += 2;
		}
		break;

		case 0xE000:
			switch (opcode & 0x00FF)
			{
			case 0x009E: // EX9E: Skips the next instruction if the key stored in VX is pressed
				waitkey = true;			// logic: keep decoding this until key hit if key hits then pc+=2
				for (unsigned char i = 1; i <= 6 && waitkey; i++)
					if (GetKey(olc::Key(i)).bHeld) { if (Vreg[(opcode & 0x0f00) >> 8] == i + 9) { PC += 4;  waitkey = false; break; }  } // doubt
				for (unsigned char i = 27; i <= 36 && waitkey; i++)
					if (GetKey(olc::Key(i)).bHeld) { if (Vreg[(opcode & 0x0f00) >> 8] == i - 27) { PC += 4;  waitkey = false; break; }  }
				if (waitkey)PC += 2;

				//if (waitkey == false)PC += 2;
				/*waitkey = true;			// logic: keep decoding this until key hit if key hits then pc+=2
				for (unsigned char i = 1; i <= 6 && waitkey; i++)
					if (GetKey(olc::Key(i)).bPressed) { if (Vreg[(opcode & 0x0f00) >> 8] == i + 9)PC += 4; else PC += 2; waitkey = false; break; } // doubt
				for (unsigned char i = 27; i <= 36 && waitkey; i++)
					if (GetKey(olc::Key(i)).bPressed) { if (Vreg[(opcode & 0x0f00) >> 8] == i - 27) PC += 4; else PC += 2; waitkey = false; break; }
				/*
				if (Vreg[(opcode & 0x0F00) >> 8] > 9)if (GetKey(olc::Key(Vreg[(opcode & 0x0F00) >> 8] - 9)).bPressed)PC += 4;
				else if (Vreg[(opcode & 0x0F00) >> 8] < 9)if (GetKey(olc::Key(Vreg[(opcode & 0x0F00) >> 8] + 27)).bPressed)PC += 4;
				else PC += 2;*/
				break;

			case 0x00A1: // EXA1: Skips the next instruction if the key stored in VX isn't pressed
				waitkey = true;			// logic: keep decoding this until key hit if key hits then pc+=2
				for (unsigned char i = 1; i <= 6 && waitkey; i++)
					if (GetKey(olc::Key(i)).bHeld) { if (Vreg[(opcode & 0x0f00) >> 8] == i + 9) { PC += 2;  waitkey = false; } break; } // doubt
				for (unsigned char i = 27; i <= 36 && waitkey; i++)
					if (GetKey(olc::Key(i)).bHeld) { if (Vreg[(opcode & 0x0f00) >> 8] == i - 27) { PC += 2; waitkey = false; } break; }
				if(waitkey)PC += 4;
				/*if (Vreg[(opcode & 0x0F00) >> 8] > 9)if (GetKey(olc::Key(Vreg[(opcode & 0x0F00) >> 8] - 9)).bPressed)PC += 2;
				else if (Vreg[(opcode & 0x0F00) >> 8] < 9)if (GetKey(olc::Key(Vreg[(opcode & 0x0F00) >> 8] + 27)).bPressed)PC += 2;
				else PC += 4;*/
				break;

			default:
				printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
			}
			break;

		case 0xF000:
			switch (opcode & 0x00FF)
			{
			case 0x0007: // FX07: Sets VX to the value of the delay timer
				Vreg[(opcode & 0x0F00) >> 8] = Dreg;
				PC += 2;
				break;

			case 0x000A: // FX0A: A key press is awaited, and then stored in VX		
			{
				waitkey = true;			// logic: keep decoding this until key hit if key hits then pc+=2
				for (char i = 1; i <= 6 && waitkey; i++)
					if (GetKey(olc::Key(i)).bHeld) { Vreg[(opcode & 0x0f00) >> 8] = i + 9; PC += 2; waitkey = false; break; } // doubt
				for (char i = 27; i <= 36 && waitkey; i++)
					if (GetKey(olc::Key(i)).bHeld) { Vreg[(opcode & 0x0f00) >> 8] = i - 27; PC += 2; waitkey = false; break; }
				//if (waitkey == false)PC += 2; //necessary?
			}
			break;

			case 0x0015: // FX15: Sets the delay timer to VX

				Dreg = Vreg[(opcode & 0x0F00) >> 8];
				PC += 2;

				break;

			case 0x0018: // FX18: Sets the sound timer to VX
				Sreg = Vreg[(opcode & 0x0F00) >> 8];
				PC += 2;
				break;

			case 0x001E: // FX1E: Adds VX to I
				(I + Vreg[(opcode & 0x0F00) >> 8]) > 0xFFF ? VF = 1 : VF = 0;
				I = I + Vreg[(opcode & 0x0F00) >> 8];
				PC += 2;
				break;

			case 0x0029: // FX29: Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font
				I = 80 + (5 * (Vreg[(opcode & 0x0F00) >> 8]));
				PC += 2;
				break;

			case 0x0033: // FX33: Stores the Binary-coded decimal representation of VX at the addresses I, I plus 1, and I plus 2
				mem[I] = Vreg[(opcode & 0x0F00) >> 8] / 100;
				mem[I + 1] = (Vreg[(opcode & 0x0F00) >> 8] / 10) % 10;
				mem[I + 2] = Vreg[(opcode & 0x0F00) >> 8] % 10;
				PC += 2;
				break;

			case 0x0055: // FX55: Stores V0 to VX in memory starting at address I					
				for (unsigned char i = 0; i <= ((opcode & 0xF00) >> 8); i++)
					mem[I + i] = Vreg[i];
				PC += 2;
				// On the original interpreter, when the operation is done, I = I + X + 1.
				I += ((opcode & 0x0F00) >> 8) + 1;

				break;

			case 0x0065: // FX65: Fills V0 to VX with values from memory starting at address I					
				for (unsigned char i = 0; i <= ((opcode & 0xF00) >> 8); i++)
					Vreg[i] = mem[I + i];
				PC += 2;
				// On the original interpreter, when the operation is done, I = I + X + 1.
				I += ((opcode & 0x0F00) >> 8) + 1;
				break;

			default:
				printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
			}
			break;

		default:
			printf("Unknown opcode: 0x%X\n", opcode);
		}
	}

	void drawSprite(unsigned char n,unsigned char Vx, unsigned char Vy) 
{

	for (int x = 0; x < n; x++)
	{
		unsigned char temp = mem[I + x];
		for (int y = 0; y < 8; y++) {
			if (((temp >>(7-y)) & 0x01) == 1)
			{
				if ((GetDrawTarget()->GetPixel((y + Vx) % 64, (x + Vy) % 32)) == olc::WHITE)
				{
					SetPixelMode(olc::Pixel::MASK);
					Draw((y + Vx) % 64, (x + Vy) % 32, olc::BLACK);
					SetPixelMode(olc::Pixel::NORMAL);
					VF = 1;
				}
				else {
					SetPixelMode(olc::Pixel::MASK);
					Draw((y + Vx) % 64, (x + Vy) % 32, olc::WHITE);
					SetPixelMode(olc::Pixel::NORMAL);
					VF = 0;
				}
			}
			else VF = 0;
		}
	}
}
	void loadgame(const char *game) {
		FILE *fgame;

		fgame = fopen(game, "rb");

		if (NULL == fgame) {
			fprintf(stderr, "Unable to open game: %s\n", game);
			exit(42);
		}

		fread(&mem[0x200], 1, 4096, fgame);

		fclose(fgame);
	}
	bool OnUserCreate() override
	{
		Screen::Clear(olc::WHITE);
		loadgame("Pong.ch8");
		PC = 0x200;
		SP = 0xEA0;
		I = 0;
		for (int i = 0; i < 80; ++i)			// load fontset
			mem[i+80] = fontset[i];
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		//fAccumulatedTime += fElapsedTime;
		//if (fAccumulatedTime >= fTargetFrameTime)
		//{
		//	fAccumulatedTime -= fTargetFrameTime;
		//	fElapsedTime = fTargetFrameTime;
		//}
		//else
		//	return true; // Don't do anything this frame

		//if (GetKey(olc::Key::X).bHeld) {
		//	printf("opcode=%X \n", (mem[PC] << 8) | mem[PC + 1]);
		//	printf("Decoding... \n");
		//	decode((mem[PC] << 8) | mem[PC + 1]);
		//	if (Sreg > 0) {
		//		if (Sreg == 1) {/*Beep*/ }
		//		--Sreg;
		//	}
		//
		//	if (Dreg > 0) {
		//		--Dreg;
		//	}
		//	printf("After Decode: PC=0x%X \n", PC);
		//	for (int i = 0; i < 16; i++)
		//		printf("Vreg[%d]=0x%X \n", i, Vreg[i]);
		//	printf("I=0x%X \n",I);
		//	printf("SP=0x%X \n",SP);
		//	printf("Dreg=0x%X \n", Dreg);
		//	printf("____________________________________________________\n");
		//}
		//else if(GetKey(olc::Key::S).bPressed){
		//	printf("opcode=%X \n", (mem[PC] << 8) | mem[PC + 1]);
		//	printf("Decoding... \n");
		//	decode((mem[PC] << 8) | mem[PC + 1]);
		//	if (Sreg > 0) {
		//		if (Sreg == 1) {/*Beep*/ }
		//		--Sreg;
		//	}
		//
		//	if (Dreg > 0) {
		//		--Dreg;
		//	}
		//	printf("After Decode: PC=0x%X \n", PC);
		//	for (int i = 0; i < 16; i++)
		//		printf("Vreg[%d]=0x%X \n", i, Vreg[i]);
		//	printf("I=0x%X \n", I);
		//	printf("SP=0x%X \n", SP);
		//	printf("Dreg=0x%X \n", Dreg);
		//	printf("____________________________________________________\n");
		//}
		//return true;

		decode((mem[PC] << 8) | mem[PC + 1]);
		// printf("opcode=%X \n", (mem[PC] << 8) | mem[PC + 1]);
		if (Sreg > 0) {
			if (Sreg == 1) {/*Beep*/ }
			--Sreg;
		}

		if (Dreg > 0) {
			--Dreg;
		}
		return true;

		
		
	}
};


int main()
{
	Screen demo;
	if (demo.Construct(64, 32, 4, 4,false,true))
		demo.Start();
	// for(int i=0x318;i<=0x35C;i+=2)
	// 	printf("opcode[%X]= %X \n",i, mem[i]<<8 | mem[i+1]);
	// printf("PC= %X", PC);
	return 0;
	
	
}