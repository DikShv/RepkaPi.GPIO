/*
Copyright (c) 2012-2016 Ben Croston

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
#include "c_gpio.h"
#include "event_gpio.h"
#include "py_pwm.h"
#include "boards.h"
#include "common.h"
#include "constants.h"

static int gpio_warnings = 1;
static PyObject *board_info;

struct py_callback
{
  unsigned int gpio;
  PyObject *py_cb;
  struct py_callback *next;
};
static struct py_callback *py_callbacks = NULL;

static int mmap_gpio_mem(int gpio)
{
  int result;
  int gc;

  if(gpio>=352 && gpio<=363)
    gc=1;
  else
    gc=0;

  result = setup(gpio);
  if (result == SETUP_DEVMEM_FAIL)
  {
    PyErr_SetString(PyExc_RuntimeError, "Нет доступа к /dev/mem. Попробуйте запустить под root! / No access to /dev/mem.  Try running as root!");
    return 1;
  } else if (result == SETUP_MALLOC_FAIL) {
    PyErr_NoMemory();
    return 2;
  } else if (result == SETUP_MMAP_FAIL) {
    PyErr_SetString(PyExc_RuntimeError, "Ошибка Mmap регистров GPIO / Mmap of GPIO registers failed");
    return 3;
  } else { // result == SETUP_OK
    module_setup = 1;
    if(gpio>=352 && gpio<=363)
      gpio_chip_1=1;
    else
      gpio_chip_0=1;
		//printf("SETUP_OK!\n");
    return 0;
  }
}

// python function setboard(board)
static PyObject *py_setrpi(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, "i", &board_type))
    return NULL;

  if (setup_error)
  {
    PyErr_SetString(PyExc_RuntimeError, "Модуль импортирован неправильно! / Module not imported correctly!");
    return NULL;
  }

  if (board_type < REPKAPI3)
  {
    PyErr_SetString(PyExc_ValueError, "Передана не верная модель платы в setboard() / An invalid board was passed to setboard()");
    return NULL;
  }

  //here is the 'pin_to_gpio' initialization
  switch (board_type) {
    case 1 :pin_to_gpio = &pin_to_gpio_repkapi3; break;
  }

  Py_RETURN_NONE;
}

static PyObject *py_getrpi(PyObject *self, PyObject *args)
{
   PyObject *value;

   if (setup_error)
   {
      PyErr_SetString(PyExc_RuntimeError, "Модуль импортирован неправильно! / Module not imported correctly!");
      return NULL;
   }

   if (board_type == BOARD_UNKNOWN)
      Py_RETURN_NONE;

   value = Py_BuildValue("s", BOARDS[board_type]);
   return value;
}

// python function setmode(mode)
static PyObject *py_setmode(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, "i", &gpio_mode))
    return NULL;

  if (setup_error)
  {
      PyErr_SetString(PyExc_RuntimeError, "Модуль импортирован неправильно! / Module not imported correctly!");
      return NULL;
  }

  if (gpio_mode != BOARD && gpio_mode != BCM && gpio_mode != MODE_SOC)
  {
    PyErr_SetString(PyExc_ValueError, "Недопустимый режим был передан в setmode() / An invalid mode was passed to setmode()");
    return NULL;
  }

  if ((gpio_mode != MODE_SOC) && board_type == BOARD_UNKNOWN)
  {
    PyErr_SetString(PyExc_ValueError, "Пожалуйста, используйте setboard(board) перед setmode() / Please use setboard(board) before setmode()");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *py_getmode(PyObject *self, PyObject *args)
{
   PyObject *value;

   if (setup_error)
   {
      PyErr_SetString(PyExc_RuntimeError, "Модуль импортирован неправильно! / Module not imported correctly!");
      return NULL;
   }

   if (gpio_mode == MODE_UNKNOWN)
      Py_RETURN_NONE;

   value = Py_BuildValue("i", gpio_mode);
   return value;
}

// python function setwarnings(state)
static PyObject *py_setwarnings(PyObject *self, PyObject *args)
{
   if (!PyArg_ParseTuple(args, "i", &gpio_warnings))
      return NULL;

   if (setup_error)
   {
      PyErr_SetString(PyExc_RuntimeError, "Модуль импортирован неправильно! / Module not imported correctly!");
      return NULL;
   }

   Py_RETURN_NONE;
}

// python function setup(channel, direction, pull_up_down=PUD_OFF, initial=None)
static PyObject *py_setup_channel(PyObject *self, PyObject *args, PyObject *kwargs)
{
  unsigned int gpio;
  int channel = -1;
  int direction;
  int i, chancount;
  PyObject *chanlist = NULL;
  PyObject *chantuple = NULL;
  PyObject *tempobj;
  int pud = PUD_OFF + PY_PUD_CONST_OFFSET;
  int initial = -1;
  static char *kwlist[] = {"channel", "direction", "pull_up_down", "initial", NULL};
  int func;

  int setup_one(void) {
      if (get_gpio_number(channel, &gpio))
         return 0;

      if (mmap_gpio_mem(gpio))
        return 0;

      func = gpio_function(gpio);
      if (gpio_warnings &&                             // warnings enabled and
          ((func != 0 && func != 1) ||                 // (already one of the alt functions or
          (gpio_direction[gpio] == -1 && func == 1)))  // already an output not set from this program)
      {
         PyErr_Warn(NULL, "Этот канал уже используется, но выполнение продолжится. Используйте GPIO.setwarnings(False), чтобы отключить предупреждения. / This channel is already in use, continuing anyway. Use GPIO.setwarnings(False) to disable warnings.");
      }

      // warn about pull/up down on i2c channels
      if (gpio_warnings) {
         if (gpio == 12 || gpio == 11) {
            if (pud == PUD_UP || pud == PUD_DOWN)
               PyErr_WarnEx(NULL, "На этом канале установлен физический подтягивающий резистор!", 1);
         }
      }

      if (direction == OUTPUT && (initial == LOW || initial == HIGH)) {
         output_gpio(gpio, initial);
      }
      setup_gpio(gpio, direction, pud);
      gpio_direction[gpio] = direction;
      return 1;
   }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|ii", kwlist, &chanlist, &direction, &pud, &initial))
      return NULL;
#if PY_MAJOR_VERSION >= 3
   if (PyLong_Check(chanlist)) {
      channel = (int)PyLong_AsLong(chanlist);
#else
   if (PyInt_Check(chanlist)) {
      channel = (int)PyInt_AsLong(chanlist);
#endif
    if (PyErr_Occurred())
         return NULL;
      chanlist = NULL;
   } else if (PyList_Check(chanlist)) {
      // do nothing
   } else if (PyTuple_Check(chanlist)) {
      chantuple = chanlist;
      chanlist = NULL;
   } else {
      // raise exception
      PyErr_SetString(PyExc_ValueError, "Канал должен быть целым числом или списком/кортежом целых чисел / Channel must be an integer or list/tuple of integers");
      return NULL;
   }

   // check module has been imported cleanly
   if (setup_error){
      PyErr_SetString(PyExc_RuntimeError, "Модуль импортирован неправильно! / Module not imported correctly!");
      return NULL;
   }

   if (direction != INPUT && direction != OUTPUT) {
      PyErr_SetString(PyExc_ValueError, "В функцию setup() было передано неверное направление / An invalid direction was passed to setup()");
      return 0;
   }

   if (direction == OUTPUT && pud != PUD_OFF + PY_PUD_CONST_OFFSET) {
      PyErr_SetString(PyExc_ValueError, "Параметр pull_up_down не действителен для выходов / pull_up_down parameter is not valid for outputs");
      return 0;
   }

   if (direction == INPUT && initial != -1) {
      PyErr_SetString(PyExc_ValueError, "initial параметр не действителен для входов / initial parameter is not valid for inputs");
      return 0;
   }

   if (direction == OUTPUT)
      pud = PUD_OFF + PY_PUD_CONST_OFFSET;

   pud -= PY_PUD_CONST_OFFSET;
   if (pud != PUD_OFF && pud != PUD_DOWN && pud != PUD_UP) {
      PyErr_SetString(PyExc_ValueError, "Недопустимое значение для pull_up_down — должно быть PUD_OFF, PUD_UP или PUD_DOWN. / Invalid value for pull_up_down - should be either PUD_OFF, PUD_UP or PUD_DOWN");
      return NULL;
   }

   if (chanlist) {
       chancount = PyList_Size(chanlist);
   } else if (chantuple) {
       chancount = PyTuple_Size(chantuple);
   } else {
       if (!setup_one())
          return NULL;
       Py_RETURN_NONE;
   }

   for (i=0; i<chancount; i++) {
      if (chanlist) {
         if ((tempobj = PyList_GetItem(chanlist, i)) == NULL) {
            return NULL;
         }
      } else { // assume chantuple
         if ((tempobj = PyTuple_GetItem(chantuple, i)) == NULL) {
            return NULL;
         }
      }

#if PY_MAJOR_VERSION >= 3
      if (PyLong_Check(tempobj)) {
         channel = (int)PyLong_AsLong(tempobj);
#else
      if (PyInt_Check(tempobj)) {
         channel = (int)PyInt_AsLong(tempobj);
#endif
         if (PyErr_Occurred())
             return NULL;
      } else {
          PyErr_SetString(PyExc_ValueError, "Канал должен быть целым числом / Channel must be an integer");
          return NULL;
      }

      if (!setup_one())
         return NULL;
   }

   Py_RETURN_NONE;
}

// python function output(channel, value)
static PyObject *py_output_gpio(PyObject *self, PyObject *args)
{
   unsigned int gpio;
   int channel = -1;
   int value = -1;
   int i;
   PyObject *chanlist = NULL;
   PyObject *valuelist = NULL;
   PyObject *chantuple = NULL;
   PyObject *valuetuple = NULL;
   PyObject *tempobj = NULL;
   int chancount = -1;
   int valuecount = -1;

   int output(void) {
      if (get_gpio_number(channel, &gpio))
          return 0;

      if (gpio_direction[gpio] != OUTPUT)
      {
         PyErr_SetString(PyExc_RuntimeError, "Канал GPIO не был настроен как OUTPUT / The GPIO channel has not been set up as an OUTPUT");
         return 0;
      }

      if (check_gpio_priv())
         return 0;

      output_gpio(gpio, value);
      return 1;
   }

   if (!PyArg_ParseTuple(args, "OO", &chanlist, &valuelist))
       return NULL;

#if PY_MAJOR_VERSION >= 3
   if (PyLong_Check(chanlist)) {
      channel = (int)PyLong_AsLong(chanlist);
#else
   if (PyInt_Check(chanlist)) {
      channel = (int)PyInt_AsLong(chanlist);
#endif
      if (PyErr_Occurred())
         return NULL;
      chanlist = NULL;
   } else if (PyList_Check(chanlist)) {
      // do nothing
   } else if (PyTuple_Check(chanlist)) {
      chantuple = chanlist;
      chanlist = NULL;
   } else {
       PyErr_SetString(PyExc_ValueError, "Канал должен быть целым числом или списком/кортежом целых чисел / Channel must be an integer or list/tuple of integers");
       return NULL;
   }

#if PY_MAJOR_VERSION >= 3
   if (PyLong_Check(valuelist)) {
       value = (int)PyLong_AsLong(valuelist);
#else
   if (PyInt_Check(valuelist)) {
       value = (int)PyInt_AsLong(valuelist);
#endif
      if (PyErr_Occurred())
         return NULL;
       valuelist = NULL;
   } else if (PyList_Check(valuelist)) {
      // do nothing
   } else if (PyTuple_Check(valuelist)) {
      valuetuple = valuelist;
      valuelist = NULL;
   } else {
       PyErr_SetString(PyExc_ValueError, "Значение должно быть целым числом/логином или списком/кортежом целых чисел/логических / Value must be an integer/boolean or a list/tuple of integers/booleans");
       return NULL;
   }

   if (chanlist)
       chancount = PyList_Size(chanlist);
   if (chantuple)
       chancount = PyTuple_Size(chantuple);
   if (valuelist)
       valuecount = PyList_Size(valuelist);
   if (valuetuple)
       valuecount = PyTuple_Size(valuetuple);
   if ((chancount != -1 && chancount != valuecount && valuecount != -1) || (chancount == -1 && valuecount != -1)) {
       PyErr_SetString(PyExc_RuntimeError, "Количество каналов! = Количество значений / Number of channels != number of values");
       return NULL;
   }

   if (chancount == -1) {
      if (!output())
         return NULL;
      Py_RETURN_NONE;
   }

   for (i=0; i<chancount; i++) {
      // get channel number
      if (chanlist) {
         if ((tempobj = PyList_GetItem(chanlist, i)) == NULL) {
            return NULL;
         }
      } else { // assume chantuple
         if ((tempobj = PyTuple_GetItem(chantuple, i)) == NULL) {
            return NULL;
         }
      }

#if PY_MAJOR_VERSION >= 3
      if (PyLong_Check(tempobj)) {
         channel = (int)PyLong_AsLong(tempobj);
#else
      if (PyInt_Check(tempobj)) {
         channel = (int)PyInt_AsLong(tempobj);
#endif
         if (PyErr_Occurred())
             return NULL;
      } else {
          PyErr_SetString(PyExc_ValueError, "Канал должен быть целым числом / Channel must be an integer");
          return NULL;
      }

      // get value
      if (valuecount > 0) {
          if (valuelist) {
             if ((tempobj = PyList_GetItem(valuelist, i)) == NULL) {
                return NULL;
             }
          } else { // assume valuetuple
             if ((tempobj = PyTuple_GetItem(valuetuple, i)) == NULL) {
                return NULL;
             }
          }
#if PY_MAJOR_VERSION >= 3
          if (PyLong_Check(tempobj)) {
             value = (int)PyLong_AsLong(tempobj);
#else
          if (PyInt_Check(tempobj)) {
             value = (int)PyInt_AsLong(tempobj);
#endif
             if (PyErr_Occurred())
                 return NULL;
          } else {
              PyErr_SetString(PyExc_ValueError, "Значение должно быть целым и логическим / Value must be an integer or boolean");
              return NULL;
          }
      }
      if (!output())
         return NULL;
   }

   Py_RETURN_NONE;
}

// python function value = input(channel)
static PyObject *py_input_gpio(PyObject *self, PyObject *args)
{
   unsigned int gpio;
   int channel;
   PyObject *value;

   if (!PyArg_ParseTuple(args, "i", &channel))
      return NULL;

   if (get_gpio_number(channel, &gpio))
       return NULL;

   // check channel is set up as an input or output
   if (gpio_direction[gpio] != INPUT && gpio_direction[gpio] != OUTPUT)
   {
      PyErr_SetString(PyExc_RuntimeError, "Сначала вы должны настроить канал GPIO через setup() / You must setup() the GPIO channel first");
      return NULL;
   }

   if (check_gpio_priv())
      return NULL;

   if (input_gpio(gpio)) {
      value = Py_BuildValue("i", HIGH);
   } else {
      value = Py_BuildValue("i", LOW);
   }
   return value;
}

// python function value = gpio_function(channel)
static PyObject *py_gpio_function(PyObject *self, PyObject *args)
{
  unsigned int gpio;
  int channel;
  int f;
  int fn;
  PyObject *func;

  if (!PyArg_ParseTuple(args, "i", &channel))
    return NULL;

  if (get_gpio_number(channel, &gpio))
    return NULL;

  if (mmap_gpio_mem(gpio))
    return NULL;

  f = gpio_function(gpio);

  switch (f)
  {
    case 0 : fn = INPUT;  break;
    case 1 : fn = OUTPUT; break;
    case 2 :
    case 3 :
    case 4 :
    case 5 :
    case 6 :
    case 7 : fn = f; break;
    default : fn = ALT_UNKNOWN; break;
  }

  func = Py_BuildValue("i", fn);
  return func;
}

// python function value = gpio_function_name(channel)
static PyObject *py_gpio_function_string(PyObject *self, PyObject *args)
{
  unsigned int gpio;
  int channel;
  int f;
  int fn;
  char *str;
  PyObject *func;

  if (!PyArg_ParseTuple(args, "i", &channel))
    return NULL;

  if (get_gpio_number(channel, &gpio))
    return NULL;

  if (mmap_gpio_mem(gpio))
    return NULL;

  f = gpio_function(gpio);

  fn = gpio_function_name(gpio,f,board_type);

  str = FUNCTIONS[fn];

  func = Py_BuildValue("s", str);
  return func;
}

// python function cleanup(channel=None)
static PyObject *py_cleanup(PyObject *self, PyObject *args, PyObject *kwargs)
{
   int i;
   int chancount = -666;
   int found = 0;
   int channel = -666;
   unsigned int gpio;
   PyObject *chanlist = NULL;
   PyObject *chantuple = NULL;
   PyObject *tempobj;
   static char *kwlist[] = {"channel", NULL};

   void cleanup_one(void)
   {
      // clean up any /sys/class exports
      event_cleanup(gpio);

      // set everything back to input
      if (gpio_direction[gpio] != -1) {
         setup_gpio(gpio, INPUT, PUD_OFF);
         gpio_direction[gpio] = -1;
         found = 1;
      }
   }

   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &chanlist))
      return NULL;

   if (chanlist == NULL) {  // channel kwarg not set
      // do nothing
#if PY_MAJOR_VERSION > 2
   } else if (PyLong_Check(chanlist)) {
      channel = (int)PyLong_AsLong(chanlist);
#else
   } else if (PyInt_Check(chanlist)) {
      channel = (int)PyInt_AsLong(chanlist);
#endif
      if (PyErr_Occurred())
         return NULL;
      chanlist = NULL;
   } else if (PyList_Check(chanlist)) {
      chancount = PyList_Size(chanlist);
   } else if (PyTuple_Check(chanlist)) {
      chantuple = chanlist;
      chanlist = NULL;
      chancount = PyTuple_Size(chantuple);
   } else {
      // raise exception
      PyErr_SetString(PyExc_ValueError, "Канал должен быть целым числом или списком/кортежом целых чисел / Channel must be an integer or list/tuple of integers");
      return NULL;
   }

   if (module_setup && !setup_error) {
      if (channel == -666 && chancount == -666) {   // channel not set - cleanup everything
         // clean up any /sys/class exports
         event_cleanup_all();

         // set everything back to input
         for (i=0; i<383; i++) {
            if (gpio_direction[i] != -1) {
               setup_gpio(i, INPUT, PUD_OFF);
               gpio_direction[i] = -1;
               found = 1;
            }
         }
         gpio_mode = MODE_UNKNOWN;
      } else if (channel != -666) {    // channel was an int indicating single channel
         if (get_gpio_number(channel, &gpio))
            return NULL;
         cleanup_one();
      } else {  // channel was a list/tuple
         for (i=0; i<chancount; i++) {
            if (chanlist) {
               if ((tempobj = PyList_GetItem(chanlist, i)) == NULL) {
                  return NULL;
               }
            } else { // assume chantuple
               if ((tempobj = PyTuple_GetItem(chantuple, i)) == NULL) {
                  return NULL;
               }
            }

#if PY_MAJOR_VERSION > 2
            if (PyLong_Check(tempobj)) {
               channel = (int)PyLong_AsLong(tempobj);
#else
            if (PyInt_Check(tempobj)) {
               channel = (int)PyInt_AsLong(tempobj);
#endif
               if (PyErr_Occurred())
                  return NULL;
            } else {
               PyErr_SetString(PyExc_ValueError, "Канал должен быть целым числом / Channel must be an integer");
               return NULL;
            }

            if (get_gpio_number(channel, &gpio))
               return NULL;
            cleanup_one();
         }
      }
   }

   // check if any channels set up - if not warn about misuse of GPIO.cleanup()
   if (!found && gpio_warnings) {
      PyErr_Warn(NULL, "Каналы пока не настроены - нечего очищать! Попробуйте выполнить очистку в конце программы! / No channels have been set up yet - nothing to clean up!  Try cleaning up at the end of your program instead!");
   }

   Py_RETURN_NONE;
}

/** EVENT **/
static void run_py_callbacks(unsigned int gpio)
{
  PyObject *result;
  PyGILState_STATE gstate;
  struct py_callback *cb = py_callbacks;

  while (cb != NULL)
  {
    if (cb->gpio == gpio) {
      // run callback
      gstate = PyGILState_Ensure();
      result = PyObject_CallFunction(cb->py_cb, "i", gpio);
      if (result == NULL && PyErr_Occurred()){
        PyErr_Print();
        PyErr_Clear();
      }
      Py_XDECREF(result);
      PyGILState_Release(gstate);
    }
    cb = cb->next;
  }
}

static int add_py_callback(unsigned int gpio, PyObject *cb_func)
{
  struct py_callback *new_py_cb;
  struct py_callback *cb = py_callbacks;

  // add callback to py_callbacks list
  new_py_cb = malloc(sizeof(struct py_callback));
  if (new_py_cb == 0)
  {
    PyErr_NoMemory();
    return -1;
  }
  new_py_cb->py_cb = cb_func;
  Py_XINCREF(cb_func);         // Add a reference to new callback
  new_py_cb->gpio = gpio;
  new_py_cb->next = NULL;
  if (py_callbacks == NULL) {
    py_callbacks = new_py_cb;
  } else {
    // add to end of list
    while (cb->next != NULL)
      cb = cb->next;

    cb->next = new_py_cb;
  }
  add_edge_callback(gpio, run_py_callbacks);
  return 0;
}

// python function add_event_callback(gpio, callback)
static PyObject *py_add_event_callback(PyObject *self, PyObject *args, PyObject *kwargs)
{
  unsigned int gpio;
  int channel;
  PyObject *cb_func;
  char *kwlist[] = {"gpio", "callback", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO|i", kwlist, &channel, &cb_func))
    return NULL;

  if (!PyCallable_Check(cb_func))
  {
    PyErr_SetString(PyExc_TypeError, "Параметр должен быть вызываемым / Parameter must be callable");
    return NULL;
  }

  if (get_gpio_number(channel, &gpio))
    return NULL;

  // check channel is set up as an input
  if (gpio_direction[gpio] != INPUT)
  {
    PyErr_SetString(PyExc_RuntimeError, "Сначала вы должны настроить канал GPIO как input используя setup() / You must setup() the GPIO channel as an input first");
    return NULL;
  }

  if (!gpio_event_added(gpio))
  {
    PyErr_SetString(PyExc_RuntimeError, "Добавьте обнаружение событий, используя add_event_detect, прежде чем добавлять обратный вызов / Add event detection using add_event_detect first before adding a callback");
    return NULL;
  }

  if (add_py_callback(gpio, cb_func) != 0)
    return NULL;

  Py_RETURN_NONE;
}

// python function add_event_detect(gpio, edge, callback=None, bouncetime=0
static PyObject *py_add_event_detect(PyObject *self, PyObject *args, PyObject *kwargs)
{
  unsigned int gpio;
  int channel, edge, result;
  unsigned int bouncetime = 0;
  PyObject *cb_func = NULL;
  char *kwlist[] = {"gpio", "edge", "callback", "bouncetime", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|Oi", kwlist, &channel, &edge, &cb_func, &bouncetime))
    return NULL;

  if (cb_func != NULL && !PyCallable_Check(cb_func))
  {
    PyErr_SetString(PyExc_TypeError, "Параметр должен быть вызываемым / Parameter must be callable");
    return NULL;
  }

	if (get_gpio_number(channel, &gpio))
    return NULL;

  // check channel is set up as an input
  if (gpio_direction[gpio] != INPUT)
  {
    PyErr_SetString(PyExc_RuntimeError, "Сначала вы должны настроить канал GPIO как input используя setup() / You must setup() the GPIO channel as an input first");
    return NULL;
  }
  // is edge valid value
  edge -= PY_EVENT_CONST_OFFSET;
  if (edge != RISING_EDGE && edge != FALLING_EDGE && edge != BOTH_EDGE)
  {
    PyErr_SetString(PyExc_ValueError, "edge должен быть установлен как RISING, FALLING или BOTH / The edge must be set to RISING, FALLING or BOTH");
    return NULL;
  }

  if (check_gpio_priv())
    return NULL;

  if ((result = add_edge_detect(gpio, edge, bouncetime)) != 0)   // starts a thread
  {
    if (result == 1)
    {
      PyErr_SetString(PyExc_RuntimeError, "Обнаружение границ уже включено для этого канала GPIO / Edge detection already enabled for this GPIO channel");
      return NULL;
    } else {
      PyErr_SetString(PyExc_RuntimeError, "Не удалось добавить обнаружение границ / Failed to add edge detection");
      return NULL;
    }
  }

  if (cb_func != NULL)
    if (add_py_callback(gpio, cb_func) != 0)
      return NULL;

  Py_RETURN_NONE;
}

// python function remove_event_detect(gpio)
static PyObject *py_remove_event_detect(PyObject *self, PyObject *args)
{
  unsigned int gpio;
  int channel;
  struct py_callback *cb = py_callbacks;
  struct py_callback *temp;
  struct py_callback *prev = NULL;

  if (!PyArg_ParseTuple(args, "i", &channel))
    return NULL;

	if (get_gpio_number(channel, &gpio))
    return NULL;

  // remove all python callbacks for gpio
  while (cb != NULL)
  {
    if (cb->gpio == gpio)
    {
      Py_XDECREF(cb->py_cb);
      if (prev == NULL)
        py_callbacks = cb->next;
      else
        prev->next = cb->next;

      temp = cb;
      cb = cb->next;
      free(temp);
    } else {
      prev = cb;
      cb = cb->next;
    }
  }

  if (check_gpio_priv())
    return NULL;

  remove_edge_detect(gpio);
  Py_RETURN_NONE;
}

// python function value = event_detected(channel)
static PyObject *py_event_detected(PyObject *self, PyObject *args)
{
  unsigned int gpio;
  int channel;

  if (!PyArg_ParseTuple(args, "i", &channel))
    return NULL;

	if (get_gpio_number(channel, &gpio))
    return NULL;

  if (event_detected(gpio))
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

// python function channel = wait_for_edge(channel, edge, bouncetime=None, timeout=None)
static PyObject *py_wait_for_edge(PyObject *self, PyObject *args, PyObject *kwargs)
{
   unsigned int gpio;
   int channel, edge, result;
   int bouncetime = -666; // None
   int timeout = -1; // None

   static char *kwlist[] = {"channel", "edge", "bouncetime", "timeout", NULL};

   if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|ii", kwlist, &channel, &edge, &bouncetime, &timeout))
      return NULL;

   if (get_gpio_number(channel, &gpio))
      return NULL;

   // check channel is setup as an input
   if (gpio_direction[gpio] != INPUT)
   {
      PyErr_SetString(PyExc_RuntimeError, "Сначала вы должны настроить канал GPIO как input используя setup() / You must setup() the GPIO channel as an input first");
      return NULL;
   }

   // is edge a valid value?
   edge -= PY_EVENT_CONST_OFFSET;
   if (edge != RISING_EDGE && edge != FALLING_EDGE && edge != BOTH_EDGE)
   {
      PyErr_SetString(PyExc_ValueError, "edge должен быть установлен как RISING, FALLING или BOTH / The edge must be set to RISING, FALLING or BOTH");
      return NULL;
   }

   if (bouncetime <= 0 && bouncetime != -666)
   {
      PyErr_SetString(PyExc_ValueError, "Bouncetime должно быть больше 0 / Bouncetime must be greater than 0");
      return NULL;
   }

   if (timeout <= 0 && timeout != -1)
   {
      PyErr_SetString(PyExc_ValueError, "Timeout должен быть больше 0 / Timeout must be greater than 0");
      return NULL;
   }

   if (check_gpio_priv())
      return NULL;

   Py_BEGIN_ALLOW_THREADS // disable GIL
   result = blocking_wait_for_edge(gpio, edge, bouncetime, timeout);
   Py_END_ALLOW_THREADS   // enable GIL

   if (result == 0) {
      Py_RETURN_NONE;
   } else if (result == -1) {
      PyErr_SetString(PyExc_RuntimeError, "Конфликтные события обнаружения краев уже существуют для этого канала GPIO / Conflicting edge detection events already exist for this GPIO channel");
      return NULL;
   } else if (result == -2) {
      PyErr_SetString(PyExc_RuntimeError, "Ошибка ожидания края / Error waiting for edge");
      return NULL;
   } else {
      return Py_BuildValue("i", channel);
   }

}

static const char moduledocstring[] = "Функциональность GPIO для плат Repka Pi с использованием Python / GPIO functionality for Repka Pi boards using Python";


PyMethodDef rpi_gpio_methods[] = {
  {"setboard", py_setrpi, METH_VARARGS, "Установка модели платы Repka Pi для использования\n-----------\nSet up Repka Pi board model to use."},
  {"getboardmodel", py_getrpi, METH_VARARGS, "Получить используемую модель платы Repka Pi\n-----------\nGet Repka Pi board model in use."},
  {"setmode", py_setmode, METH_VARARGS, "Настройте режим нумерации для каналов.\nBOARD — используйте номера плат Repka Pi\nBCM — используйте номера Broadcom GPIO 00..nn\nSOC — используйте номера портов SUNXI\n-----------\nSet up numbering mode to use for channels.\nBOARD - Use Repka Pi board numbers\nBCM   - Use Broadcom GPIO 00..nn numbers\nSOC   - Use SUNXI port numbers"},
  {"getmode", py_getmode, METH_VARARGS, "Получить режим нумерации, используемый для номеров каналов.\nВозвращает BOARD, BCM, SOC или None.\n-----------\nGet numbering mode used for channel numbers.\nReturns BOARD, BCM, SOC or None"},
  {"setwarnings", py_setwarnings, METH_VARARGS, "Включить или отключить предупреждающие сообщения\n-----------\nEnable or disable warning messages"},
  {"setup", (PyCFunction)py_setup_channel, METH_VARARGS | METH_KEYWORDS, "Настройка канала GPIO, направление и (опционально) управление напряжением/вверх-вниз\nканал — либо номер контакта платы, либо номер SOC в зависимости от того, какой режим установлен.\nнаправление — ВХОД или ВЫХОД\n[pull_up_down] — PUD_OFF (по умолчанию), PUD_UP или PUD_DOWN\n[initial] — начальное значение выходного канала.\n-----------\nSet up the GPIO channel, direction and (optional) pull/up down control\nchannel        - either board pin number or SOC number depending on which mode is set.\ndirection      - INPUT or OUTPUT\n[pull_up_down] - PUD_OFF (default), PUD_UP or PUD_DOWN\n[initial]      - Initial value for an output channel"},
  {"cleanup", (PyCFunction)py_cleanup, METH_VARARGS | METH_KEYWORDS, "Очистка путем сброса всех каналов GPIO, которые использовались этой программой, на вход INPUT без подтягивания/понижения и без обнаружения событий.\n[канал] — отдельный канал для очистки. По умолчанию — очистить каждый использованный канал.\n-----------\nClean up by resetting all GPIO channels that have been used by this program to INPUT with no pullup/pulldown and no event detection\n[channel] - individual channel to clean up.  Default - clean every channel that has been used."},
  {"output", py_output_gpio, METH_VARARGS, "Вывод на канал GPIO\nканал — либо номер контакта платы, либо номер SOC в зависимости от того, какой режим установлен.\nзначение — 0/1 или False/True или LOW/HIGH.\n-----------\nOutput to a GPIO channel\nchannel - either board pin number or SOC number depending on which mode is set.\nvalue   - 0/1 or False/True or LOW/HIGH"},
  {"input", py_input_gpio, METH_VARARGS, "Ввод из канала GPIO. Возвращает HIGH=1=True или LOW=0=False\nchannel — либо номер контакта платы, либо номер SOC, в зависимости от того, какой режим установлен.\n-----------\nInput from a GPIO channel.  Returns HIGH=1=True or LOW=0=False\nchannel - either board pin number or SOC number depending on which mode is set."},
  {"gpio_function", py_gpio_function, METH_VARARGS, "Возвращает текущую функцию GPIO (IN, OUT, идентификатор функции мультиплексирования)\nchannel — либо номер контакта платы, либо номер GPIO в зависимости от того, какой режим установлен.\n-----------\nReturn the current GPIO function (IN, OUT, Multiplexing function ID)\nchannel - either board pin number or GPIO number depending on which mode is set."},
  {"gpio_function_name", py_gpio_function_string, METH_VARARGS, "Возвращает текущую функцию GPIO (IN, OUT, функция мультиплексирования) в виде строки\nchannel — либо номера контакта платы, либо номера GPIO, в зависимости от того, какой режим установлен.\n-----------\nReturn the current GPIO function (IN, OUT, Multiplexing function) as string\nchannel - either board pin number or GPIO number depending on which mode is set."},
  {"add_event_detect", (PyCFunction)py_add_event_detect, METH_VARARGS | METH_KEYWORDS, "Включите события обнаружения краев для конкретного канала GPIO. \nchannel - либо номер PIN -кода, либо номер BCM, в зависимости от того, какой режим установлен. \nedge - RISING, PALLE или оба \n [обратный вызов] - функция обратного вызова для события (необязательно) \n [bouncetime] - переключение тайм -аут отскока в MS для обратного вызова\n-----------\nEnable edge detection events for a particular GPIO channel.\nchannel      - either board pin number or BCM number depending on which mode is set.\nedge         - RISING, FALLING or BOTH\n[callback]   - A callback function for the event (optional)\n[bouncetime] - Switch bounce timeout in ms for callback"},
  {"remove_event_detect", py_remove_event_detect, METH_VARARGS, "Удалить обнаружение фронта для определенного канала GPIO\nканала — либо номера контакта платы, либо номера SOC, в зависимости от того, какой режим установлен.\n-----------\nRemove edge detection for a particular GPIO channel\nchannel - either board pin number or SOC number depending on which mode is set."},
  {"event_detected", py_event_detected, METH_VARARGS, "Возвращает значение True, если на данном GPIO произошел перепад. Сначала вам необходимо включить обнаружение границ с помощью add_event_detect().\nchannel — либо номер контакта платы, либо номер SOC, в зависимости от того, какой режим установлен.\n-----------\nReturns True if an edge has occured on a given GPIO.  You need to enable edge detection using add_event_detect() first.\nchannel - either board pin number or SOC number depending on which mode is set."},
  {"add_event_callback", (PyCFunction)py_add_event_callback, METH_VARARGS | METH_KEYWORDS, "Добавьте обратный вызов для события, уже определенного с помощью add_event_detect()\nchannel — либо номер контакта платы, либо номер SOC, в зависимости от того, какой режим установлен.\ncallback — функция обратного вызова\n-----------\nAdd a callback for an event already defined using add_event_detect()\nchannel      - either board pin number or SOC number depending on which mode is set.\ncallback     - a callback function"},
  {"wait_for_edge", py_wait_for_edge, METH_VARARGS, "Дождитесь фронта.\nchannel — либо номер контакта платы, либо номер SOC в зависимости от того, какой режим установлен.\nedge — RISING, FALLING или BOTH\n-----------\nWait for an edge.\nchannel - either board pin number or SOC number depending on which mode is set.\nedge    - RISING, FALLING or BOTH"},
  {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION > 2
static struct PyModuleDef rpigpiomodule = {
   PyModuleDef_HEAD_INIT,
   "RepkaPi.GPIO",       // name of module
   moduledocstring,  // module documentation, may be NULL
   -1,               // size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
   rpi_gpio_methods
};
#endif

#if PY_MAJOR_VERSION > 2
PyMODINIT_FUNC PyInit_GPIO(void)
#else
PyMODINIT_FUNC initGPIO(void)
#endif
{
   int i;
   PyObject *module = NULL;

#if PY_MAJOR_VERSION > 2
   if ((module = PyModule_Create(&rpigpiomodule)) == NULL)
      return NULL;
#else
   if ((module = Py_InitModule3("RepkaPi.GPIO", rpi_gpio_methods, moduledocstring)) == NULL)
      return;
#endif

  define_constants(module);

  for (i=0; i<383; i++)
    gpio_direction[i] = -1;

  board_info = Py_BuildValue("{sissssssssss}",
                              "P1_REVISION",3,
                              "REVISION","",
                              "TYPE","Repka Pi 3",
                              "MANUFACTURER","ИНТЕЛЛЕКТ",
                              "PROCESSOR","Allwinner H5",
                              "RAM","1GB");
   PyModule_AddObject(module, "RPI_INFO", board_info);

   pin_to_gpio = &pin_to_gpio_repkapi3;

  // Add PWM class
  if (PWM_init_PWMType() == NULL)
#if PY_MAJOR_VERSION > 2
    return NULL;
#else
    return;
#endif

  Py_INCREF(&PWMType);
  PyModule_AddObject(module, "PWM", (PyObject*)&PWMType);

  if (!PyEval_ThreadsInitialized())
    PyEval_InitThreads();

  // register exit functions - last declared is called first
  if (Py_AtExit(cleanup) != 0)
  {
    setup_error = 1;
    cleanup();
#if PY_MAJOR_VERSION > 2
    return NULL;
#else
    return;
#endif
  }

  if (Py_AtExit(event_cleanup_all) != 0)
  {
    setup_error = 1;
    cleanup();
#if PY_MAJOR_VERSION > 2
    return NULL;
#else
    return;
#endif
  }

#if PY_MAJOR_VERSION > 2
 return module;
#else
 return;
#endif
}
