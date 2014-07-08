#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <iomanip>
#include <random>
#include <utility>
#include "opcodeProcessing.h"

// move this somewhere useful later
// take a list of ints and do nothing
void swallow(initializer_list<int>)
{
}

// and this
#ifdef NDEBUG
// do nothing if NDEBUG is defined
template <typename... Args>
void dbgprint(Args &&...)
{
}
#else
// basically do cout << arg; for each arg, passing the result to swallow each time
// to keep them printed in order, they're in an initializer list
// note that using std::endl won't work with this, but it's unimportant with '\n' anyway
template <typename... Args>
void dbgprint(Args &&... args)
{
   swallow({(cout << forward<Args>(args), 0)...});
}
#endif

/*
* @brief Take a bool array and display it on screen. This does
* not currently use a GL. TODO: Use a GL!
*
* @param display (array<array<bool, 32, 64>) a 64 * 32 pixel
* screen to print
*
* @note "88" corresponds to an on pixel; "  " corresponds to
* an off pixel
*/
void printToScreen(const array<array<bool, 32>, 64> &display)
{
   cout << endl;
   for (int i = 0; i < 64; i++)
      cout << "--";
   cout << endl;
   for (int i = 0; i < 32; i++)
   {
      cout << '|';
      for (int j = 0; j < 64; j++)
      {
         if (display[j][i] == true)
            cout << "88";
         else
            cout << "  ";
      }
      cout << '|' << endl;
   }
   for (int i = 0; i < 64; i++)
      cout << "--";
   cout << endl;
}
/*
* @brief Set all boolean value in a 2D array to false
*
* @param display (array<array<bool, 32>, 64>) a 64 * 32 pixel
* screen to print
*/
void clearDisplay(array<array<bool, 32>, 64> &display)
{
   for (int j = 0; j < 32; j++)
   {
      for (int i = 0; i < 64; i++)
      {
         display[i][j] = false;
      }
   }
   dbgprint("CLEARED DISPLAY\n");
}

/*
* @brief Load a byte of a sprite into the display, at location (x, y)
*
* @param sprite (unsigned char) a byte in the sprite to display
*
* @param display (array<array<bool, 32>, 64> a 64 * 32 pixel screen
* which stores the values
*
* @param x (int) the x-coordinate of the position to place the sprite
*
* @param x (int) the y-coordinate of the position to place the sprite
*
* @note Mask is used to get the value of the bits in the byte. Sprites
* are stored as bytes, where '0' corresponds to an off pixel, and '1'
* corresponds to an on pixel. We AND the two values (sprite and mask)
* and right-shift mask, so if the position of the '1' in mask is at
* the same position of a '1' in sprite, then the corresponding pixel
* is set to TRUE. If the position of the '1' in mask is at the same
* position of a '0' in sprite, then the corresponding pixel is set to
* FALSE. Recall: 1 & 1 = 1; 1 & 0 = 0
*/
bool loadSprite(unsigned char sprite, array<array<bool, 32>, 64> &display, int x, int y)
{
   short mask = 128; // 1000 0000
   bool erased = false;
   for (int i = 0; i < 8; i++)
   {
      display[x + i][y] ^= (bool)(sprite & mask);
      if (!display[x + i][y])
      {
         erased = true;
      }
      dbgprint("sprite = ", dec, (int)sprite, '\n');
      dbgprint("mask = ", dec, (int)mask, '\n');
      dbgprint("sprite & mask = ", (int)(sprite & mask), " = ", (bool)(sprite & mask), hex, '\n');
      mask = mask >> 1; // shift right
   }
   return erased;
}

/*
* @param (short) opcode: the current opcode being processed
*
* @param (int *) RAM: a pointer to the RAM
*
* @param (short) PC: a reference to the current value of PC
*
* @param (stack<short>) theStack: a reference to a stack of the previous
* values of PC; this is necessary for returning from a subroutine
*/
void determineInstruction(CHIP8state &currState)
{
   uint16_t opcode = (currState.RAM[currState.PC] << 8) | currState.RAM[currState.PC + 1];
   dbgprint("\nPROCESSING ", showbase, uppercase, hex, opcode, '\n');
   int x = getSecondNibble(opcode);
   int y = getThirdNibble(opcode);
   int n = getLastNibble(opcode);
   int kk = getLastByte(opcode);
   uint16_t nnn = getLastThreeNibbles(opcode);
   short sprite = 0;

   static mt19937 randGen{random_device{}()};

   switch (getFirstNibble(opcode))
   {
      case 0x0:
         if (opcode == 0x00E0)
         {
            // clear screen
            dbgprint("0x00E0: clear screen\n");
            clearDisplay(currState.display);
            printToScreen(currState.display);
         }

         else if (opcode == 0x00EE)
         {
            // return from subroutine -- we'll need to keep track of the value
            // of PC before jumping to a subroutine
            dbgprint("0x00EE: return from subroutine\n");
            dbgprint("PC was ", currState.PC, '\n');
            // We assume the stack can always be popped, because if this is being
            // called, we should assume the program knows what it's doing.
            // If we ever make a ROM, it would be handy to check.
            currState.PC = currState.theStack.top();
            currState.theStack.pop();
            dbgprint("PC is now ", currState.PC, '\n');
            dbgprint("Returned from subroutine!\n");
         }

         else
         {
            // invalid opcode
            dbgprint("INVALID code: ", hex, opcode, "!\n");
            currState.PC = 0; // stop loop
            return;
         }

         currState.PC += 2;
         break;
      case 0x1:
         dbgprint("0x1nnn: jump to nnn\n");
         dbgprint("PC was ", hex, currState.PC, '\n');
         currState.PC = nnn;
         dbgprint("PC is now ", hex, currState.PC, '\n');
         break;
      case 0x2:
         dbgprint("0x2nnn: call subroutine at nnn\n");
         currState.theStack.push(currState.PC);
         dbgprint("PC was ", currState.PC, '\n');
         currState.PC = nnn;
         dbgprint("PC is now ", hex, currState.PC, '\n');
         break;
      case 0x3:
         // skip next instruction if Vx equals a constant
         // if Vx == kk, PC += 4
         // else, PC += 2
         dbgprint("0x3xkk: skip if Vx equals kk\n");
         if (currState.V[x] != kk)
         {
            currState.PC += 4;
            dbgprint("Jump 4\n");
         }

         else
         {
            currState.PC += 2;
            dbgprint("Jump 2\n");
         }

         break;
      case 0x4:
         // skip next instruction if Vx doesn't equal a constant
         dbgprint("0x4xkk: skip if Vx doesn't equal kk\n");
         if (currState.V[x] != kk)
         {
            currState.PC += 4;
            dbgprint("Jump 4\n");
         }

         else
         {
            currState.PC += 2;
            dbgprint("Jump 2\n");
         }

         break;
      case 0x5:
         // skip next instruction if two registers are equal
         // if Vx == Vy, PC += 4
         // else, PC == 2
         dbgprint("0x5xy0: skip if Vx equals Vy\n");
         // if Vx == Vy, PC += 4
         if (currState.V[x] == currState.V[y])
         {
            currState.PC += 4;
            dbgprint("Jump 4\n");
         }

         else
         {
            currState.PC += 2;
            dbgprint("Jump 2\n");
         }

         break;
      case 0x6:
         // set Vx to a constant
         dbgprint("0x6xkk: load kk into Vx\n");

         currState.V[x] = kk;
         dbgprint("V", dec, x, " set to ", hex, kk, " = ", dec, kk, '\n');
         currState.PC += 2;
         break;
      case 0x7:
         // add a constant to Vx, and store in Vx
         dbgprint("0x7xkk: add kk to Vx\n");

         currState.V[x] += kk;
         dbgprint("V", dec, x, " set to ", kk, " so now equals ", static_cast<int>(currState.V[x]), '\n');
         currState.PC += 2;
         break;
      case 0x8:
         dbgprint("0x8xy?: binary operation between Vx and Vy\n");
         process0x8000Codes(currState, opcode);
         currState.PC += 2;
         break;
      case 0x9:
         // skip next instruction if two registers are not equal
         dbgprint("0x9xy0: skip if Vx doesn't equal Vy\n");
         // if Vx != Vy, PC += 4
         if (currState.V[x] != currState.V[y])
         {
            currState.PC += 4;
            dbgprint("Jump 4\n");
         }

         else
         {
            currState.PC += 2;
            dbgprint("Jump 2\n");
         }

         break;
      case 0xA:
         // set I register to a constant
         dbgprint("0xAnnn: load nnn into I\n");
         currState.I = nnn;
         dbgprint("I register set to ", hex, currState.I, '\n');
         currState.PC += 2;
         break;
      case 0xB:
         // jump to location nnn + V0
         dbgprint("0xBnnn: jump to nnn + V0\n");
         dbgprint("PC was ", currState.PC, '\n');
         currState.PC = nnn + currState.V[0];
         dbgprint("PC is now ", currState.PC, '\n');
         break;
      case 0xC:
      {
         // set Vx to random byte & kk
         // use math class random function
         dbgprint("0xCxkk: load random byte AND kk into Vx\n");
         uniform_int_distribution<> dist(0x00, 0xFF);
         auto newVal = dist(randGen) & kk;
         currState.V[x] = newVal;
         dbgprint("V", dec, x, " set to ", newVal, '\n');
         currState.PC += 2;
         break;
      }
      case 0xD:
         // display sprite on screen
         dbgprint("case 0xDxyn\n");
         dbgprint("load ", n, " byte sprite at I (", currState.I);
         dbgprint(") to (", (short)currState.V[x], ", ", (short)currState.V[y], ")\n");
         currState.V[15] = 0;
         for (int i = 0; i < n; i++)
         {
            sprite = currState.RAM[currState.I + i];
            // take bits of this sprite and put into display
            if (loadSprite(sprite, currState.display, currState.V[x], currState.V[y] + i))
            {
               currState.V[15] = 1;
            }
         }
         printToScreen(currState.display);
         currState.PC += 2;
         break;
      case 0xE:
         // skip next instruction if a specific key is pressed or not.
         // The key corresponds to the value of Vx.
         // Let's have another thread to check key presses, and
         // store a bool array, saying which key is being pressed
         // current. This will obviously need to be updated
         // frequently!
         dbgprint("0xEx??: skip if key with value Vx is or isn't pressed\n");
         currState.PC += 2;
         break;
      case 0xF:
         process0xF000Codes(currState, x, kk);
         dbgprint("0xFx??: miscellaneous\n");
         currState.PC += 2;
         break;
      default:
         // unknown opcode - do we assert(0) or do we keep going?
         // I'd say stop, but through an exception and a nice error message, and our program not dying.
         dbgprint("unknown opcode: ", opcode, "!\n");
         currState.PC += 2;
         break;
   }
}

int getNibbleAt(uint16_t opcode, int pos)
{
   assert(0 <= pos && pos <= 3);

   auto nibblesToShift = 3 - pos;
   auto bitsToShift = 4 * nibblesToShift;
   auto shifted = opcode >> bitsToShift;

   return shifted & 0xF;
}

int getFirstNibble(uint16_t opcode)
{
   return getNibbleAt(opcode, 0);
}

int getSecondNibble(uint16_t opcode)
{
   return getNibbleAt(opcode, 1);
}

int getThirdNibble(uint16_t opcode)
{
   return getNibbleAt(opcode, 2);
}

int getLastNibble(uint16_t opcode)
{
   return getNibbleAt(opcode, 3);
}

uint16_t getLastThreeNibbles(uint16_t opcode)
{
   return opcode & 0x0FFF;
}

unsigned char getLastByte(uint16_t opcode)
{
   return opcode & 0xFF;
}

void process0x8000Codes(CHIP8state &currState, uint16_t opcode)
{
   (void)opcode; // shut up possible unused parameter warning (until we use it)
   assert((opcode & 0xF000) == 0x8000);
   unsigned char x = getSecondNibble(opcode);
   unsigned char y = getThirdNibble(opcode);
   switch (getLastNibble(opcode))
   {
      case 0:
         // Set V[x] to V[y]
         dbgprint("Setting Vx (", (short)currState.V[x], ") to Vy (", (short)currState.V[y], ")\n");
         currState.V[x] = currState.V[y];
         dbgprint("Vx is now: ", (short)currState.V[x], "\n");
         break;
      case 1:
         dbgprint("Vx = Vx OR Vy");
         dbgprint("Vx = ", (short)currState.V[x], " OR ", (short)currState.V[y], "\n");
         // Set V[x] to V[x] OR V[y]
         currState.V[x] |= currState.V[y];
         dbgprint("Vx is now: ", (short)currState.V[x], "\n");
         break;
      case 2:
         dbgprint("Vx = Vx AND Vy\n");
         dbgprint("Vx = ", (short)currState.V[x], " AND ", (short)currState.V[y], "\n");
         // Set V[x] to V[x] AND V[y]
         currState.V[x] &= currState.V[y];
         dbgprint("Vx is now ", (short)currState.V[x], "\n");
         break;
      case 3:
         dbgprint("V[x] = V[x] XOR V[y]\n");
         dbgprint("Vx = ", (short)currState.V[x], "\nVy = ", (short)currState.V[y], "\n");
         // Set V[x] to V[x] XOR V[y]
         currState.V[x] ^= currState.V[y];
         dbgprint("Vx is now: ", (short)currState.V[x], "\n");
         break;
      case 4:
         dbgprint("V[x] = V[x] + V[y]; Vf = carry\n");
         // Set V[x] to V[x] + V[y], and set Vf to 1 if there is a carry
         dbgprint((short)currState.V[x], " = ", (short)currState.V[x], " + ", (short)currState.V[y], "\n");
         currState.V[0xF] = (currState.V[x] + currState.V[y] > 255);
         currState.V[x] += currState.V[y];
         dbgprint("V[x] is now ", (short)currState.V[x], "\nVf = ", (short)currState.V[0xF], "\n");
         break;
      case 5:
         dbgprint("V[x] = V[x] - V[y]\n");
         // Set V[x] to V[x] - V[y], and set Vf to NOT borrow
         dbgprint((short)currState.V[x], " = ", (short)currState.V[x], " - ", (short)currState.V[y], "\n");
         currState.V[0xF] = (currState.V[x] > currState.V[y]);
         currState.V[x] = currState.V[x] - currState.V[y];
         dbgprint("V[x] is now: ", (short)currState.V[x], "\nVf = ", (short)currState.V[0xF], "\n");
         break;
      case 6:
         // Shift V[x] right
         dbgprint("currState.V[x] = ", (short)currState.V[x], "\n");
         currState.V[0xF] = getLastNibble(currState.V[x]) & 00000001;
         currState.V[x] = currState.V[x] >> 1; // divide by 2
         dbgprint("After right shift: ", (short)currState.V[x], "\n");
         break;
      case 7:
         // Set V[x] = V[y] - V[x], and set Vf to NOT borrow
         dbgprint("V[x] = V[y] - V[x]; V[f] = !BORROW\n");
         dbgprint((short)currState.V[x], " = ", (short)currState.V[y], " - ", (short)currState.V[x], "\n");
         currState.V[0xF] = (currState.V[y] > currState.V[x]);
         currState.V[x] = currState.V[y] - currState.V[x];
         dbgprint("V[x] = ", (short)currState.V[x], "\n");
         break;
      case 0xE:
         dbgprint("Left shift V[x] = ", (short)currState.V[x], "\n");
         currState.V[0xF] = (currState.V[x] & 10000000);
         currState.V[x] = currState.V[x] << 1; // multiply by 2
         dbgprint("V[x] = ", (short)currState.V[x], hex, "\n");
         break;
      default:
         dbgprint("An error occurred!\nInvalid instruction: ", hex, opcode, "\n");
         break;
   }
}

void process0xF000Codes(CHIP8state &currState, int x, int kk)
{
   switch (kk)
   {
      case 0x07:
         dbgprint("0xFx07: set Vx to DT\n");
         // currState.V[x] = getDT();
         // dbgprint("Vx is now ", dec, static_cast<int>(currState.V[x]), '\n');
         break;

      case 0x0A:
         dbgprint("0xFx0A: wait for key press and store key in Vx\n");
         // currState.V[x] = waitKey();
         // dbgprint("Vx is now ", dec, static_cast<int>(currState.V[x]), '\n');
         break;

      case 0x15:
         dbgprint("0xFx15: set DT to Vx\n");
         // setDT(currState.V[x]);
         // dbgprint("DT is now ", dec, static_cast<int>(currState.V[x]), '\n');
         break;

      case 0x18:
         dbgprint("0xFx15: set ST to Vx\n");
         // setST(currState.V[x]);
         // dbgprint("ST is now ", dec, static_cast<int>(currState.V[x]), '\n');
         break;

      case 0x1E:
         dbgprint("0xFx1E: set I to I + Vx\n");
         currState.I += currState.V[x];
         dbgprint("I is now ", hex, currState.I, '\n');
         break;

      case 0x29:
         dbgprint("0xFx29: set I to location of sprite for digit Vx\n");
         currState.I = currState.V[x] * 5; // digit X is at memory location X * 5
         dbgprint("I is now ", hex, currState.I, '\n');
         break;

      case 0x33:
         dbgprint("0xFx33: Store BCD representation of Vx starting at memory locations I\n");
         currState.RAM[currState.I] = currState.V[x] / 100;            // first digit
         currState.RAM[currState.I + 1] = (currState.V[x] % 100) / 10; // second digit
         currState.RAM[currState.I + 2] = currState.V[x] % 10;         // third digit
         dbgprint("RAM at I, I + 1, I + 2: ", static_cast<int>(currState.RAM[currState.I]), ' ');
         dbgprint(static_cast<int>(currState.RAM[currState.I + 1]), static_cast<int>(currState.RAM[currState.I + 2]), '\n');
         break;

      case 0x55:
         dbgprint("0xFx55: Store registers V0 through Vx in memory starting at I\n");
         copy_n(begin(currState.V), x, next(begin(currState.RAM), currState.I));
         break;

      case 0x65:
         dbgprint("0xFx65: Read registers V0 through Vx from memory starting at I\n");
         copy_n(next(begin(currState.RAM), currState.I), x, begin(currState.V));
         break;

      default:
         dbgprint("Unrecognized opcode: 0xF", x, kk, '\n');
         break;
   }
}