/*
Copyright (c) 2018 Jeremie-C

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Python.h"
#include "common.h"
#include "boards.h"

/* Repka Pi 3 */
const int pin_to_gpio_repkapi3[41] = {
  -1, -1, -1, // 0, 1, 2
  12, -1, // 3, 4
  11, -1, // 5, 6
  7, 4, // 7, 8
  -1, 5, // 9, 10
  8, 6, // 11, 12
  9, -1, // 13, 14
  10, 354, // 15, 16
  -1, 355, // 17, 18
  64, -1, // 19, 20
  65, 2, // 21, 22
  66, 67, // 23, 24
  -1, 3, // 25, 26
  19, 18, // 27, 28
  0, -1, // 29, 30
  1, 363, // 31, 32
  362, -1, // 33, 34
  16, 13, // 35, 36
  21, 15, // 37, 38
  -1, 14 // 39, 40
};

const char* FUNCTIONS[] = {
  "INPUT","OUTPUT","RESERVED","UNKNOWN","SEE_MANUAL", // 0-4
  "TWI0_SCK","TWI0_SDA",                              // 5-6
  "TWI1_SCK","TWI1_SDA",                              // 7-8
  "TWI2_SCK","TWI2_SDA",                              // 9-10
  "S_TWI_SCK", "S_TWI_SDA",                           // 11-12
  "SPI0_CS","SPI0_CLK","SPI0_MOSI","SPI0_MISO",       // 13-16
  "SPI1_CS","SPI1_CLK","SPI1_MOSI","SPI1_MISO",       // 17-20
  "UART0_TX","UART0_RX",                              // 21-22
  "UART1_TX","UART1_RX","UART1_RTS","UART1_CTS",      // 23-26
  "UART2_TX","UART2_RX","UART2_RTS","UART2_CTS",      // 27-30
  "UART3_TX","UART3_RX","UART3_RTS","UART3_CTS",      // 31-34
  "S_UART_TX","S_UART_RX",                            // 35-36
  "PWM0","PWM1","S_PWM",                              // 37-39
  "IO DISABLED"
};

const char* BOARDS[] = {
  "","Repka Pi 3"
};

/* Get Alt Function Name */
int gpio_function_name(int gpio, int func, int board)
{
  int str;

  switch (func)
  {
    case 0 : str = 0;  break;
    case 1 : str = 1; break;
    case 2 :
      switch (gpio)
      {
        case 0 : str = 27; break;
        case 1 : str = 28; break;
        case 2 : str = 29; break;
        case 3 : str = 30; break;
        case 4 : str = 21; break;
        case 5 : str = 22; break;
        case 11 : str = 5; break;
        case 12 : str = 6; break;
        case 13 : str = 17; break;
        case 14 : str = 18; break;
        case 15 : str = 19; break;
        case 16 : str = 20; break;
        case 198 : str = 25; break;
        case 199 : str = 26; break;
        case 200 : str = 27; break;
        case 201 : str = 28; break;
        case 352 : str = 11; break;
        case 353 : str = 12; break;
        case 354 : str = 35; break;
        case 355 : str = 36; break;
        case 362 : str = 37; break;
        default : str = 2; break;
      }
      break;
    case 3 :
      switch (gpio)
      {
        case 5 : str = 37; break;
        case 6 : // PWM1 only H3
          /*if (board == 1 || board == 3 || board == 4) {
            str = 38;
          } else {*/
            str = 4;
          /*}*/
          break;
        case 13 : str = 31; break;
        case 14 : str = 32; break;
        case 15 : str = 33; break;
        case 16 : str = 34; break;
        case 18 : str = 7; break;
        case 19 : str = 8; break;
        case 64 : str = 15; break;
        case 65 : str = 16; break;
        case 66 : str = 14; break;
        case 67 : str = 13; break;
        case 140 : str = 9; break;
        case 141 : str = 10; break;
        case 162 : str = 21; break;
        case 164 : str = 22; break;
        default : str = 2; break;
      }
      break;
    case 4 :
      switch (gpio)
      {
        case 68 : // H5 Only
          if (board == 1) {
            str = 16;
          } else {
            str = 2;
          }
          break;
        default : str = 2; break;
      }
      break;
    case 5 : str = 2; break;
    case 6 : str = 4; break;
    case 7 : str = 40; break;
    default : str = 3; break;
  }
  return str;
}


