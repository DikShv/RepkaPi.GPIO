/*
Copyright (c) 2013-2015 Ben Croston

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

#define BOARD_UNKNOWN  -1
#define REPKAPI3        1


#define MODE_UNKNOWN   -1
#define BOARD          10
#define BCM            11
#define MODE_SOC       13
#define SUNXI          12

#define ALT_UNKNOWN    -1

int board_type;
int gpio_mode;
const int (*pin_to_gpio)[41];

int gpio_direction[383];
int setup_error;
int module_setup;
int gpio_chip_0;
int gpio_chip_1;

int get_gpio_number(int channel, unsigned int *gpio);
int check_gpio_priv(void);
