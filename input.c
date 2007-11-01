#include "starruby.h"

#define ADD_KEY(currentKey, _name, _sdlKey) do {\
  KeyboardKey* key = ALLOC(KeyboardKey);\
  key->rbSymbol = ID2SYM(rb_intern(_name));\
  key->sdlKey   = _sdlKey;\
  key->state    = 0;\
  key->next     = NULL;\
  currentKey->next = key;\
  currentKey = key;\
} while (false)

typedef struct KeyboardKey {
  VALUE rbSymbol;
  SDLKey sdlKey;
  int state;
  struct KeyboardKey* next;
} KeyboardKey;
static KeyboardKey* keyboardKeys;

static int gamePadCount;

typedef struct {
  SDL_Joystick* sdlJoystick;
  int downState;
  int leftState;
  int rightState;
  int upState;
  int buttonCount;
  int* buttonStates;
} GamePad;
static GamePad* gamePads;

typedef struct {
  int leftState;
  int middleState;
  int rightState;
} Mouse;
static Mouse* mouse;

static VALUE symbol_delay;
static VALUE symbol_device_number;
static VALUE symbol_down;
static VALUE symbol_duration;
static VALUE symbol_game_pad;
static VALUE symbol_interval;
static VALUE symbol_keyboard;
static VALUE symbol_left;
static VALUE symbol_middle;
static VALUE symbol_mouse;
static VALUE symbol_right;
static VALUE symbol_up;

static VALUE Input_mouse_location(VALUE self)
{
  return rb_iv_get(rb_mInput, "mouse_location");
}

static bool isPressed(int status, int duration, int delay, int interval)
{
  if (status <= 0 || duration == 0)
    return false;
  if (duration < 0)
    return true;
  if (status <= duration)
    return true;
  if (delay < 0)
    return false;
  if (status <= duration + delay)
    return false;
  if (0 <= interval)
    return (status - (duration + delay + 1)) % (interval + 1) == 0;
  return false;
}

static VALUE Input_pressed_keys(int argc, VALUE* argv, VALUE self)
{
  VALUE rbDevice, rbOptions;
  rb_scan_args(argc, argv, "11", &rbDevice, &rbOptions);
  if (NIL_P(rbOptions))
    rbOptions = rb_hash_new();

  VALUE rbResult = rb_ary_new();

  VALUE rbDeviceNumber = rb_hash_aref(rbOptions, symbol_device_number);
  VALUE rbDuration     = rb_hash_aref(rbOptions, symbol_duration);
  VALUE rbDelay        = rb_hash_aref(rbOptions, symbol_delay);
  VALUE rbInterval     = rb_hash_aref(rbOptions, symbol_interval);

  int deviceNumber = !NIL_P(rbDeviceNumber) ? NUM2INT(rbDeviceNumber) : 0;
  int duration     = !NIL_P(rbDuration)     ? NUM2INT(rbDuration)     : -1;
  int delay        = !NIL_P(rbDelay)        ? NUM2INT(rbDelay)        : -1;
  int interval     = !NIL_P(rbInterval)     ? NUM2INT(rbInterval)     : 0;

  if (rbDevice == symbol_keyboard) {
    KeyboardKey* key = keyboardKeys;
    while (key) {
      if (isPressed(key->state, duration, delay, interval))
        rb_ary_push(rbResult, key->rbSymbol);
      key = key->next;
    }
  } else if (rbDevice == symbol_game_pad) {
    if (0 <= deviceNumber && deviceNumber < gamePadCount) {
      GamePad* gamePad = &(gamePads[deviceNumber]);
      if (isPressed(gamePad->downState, duration, delay, interval))
        rb_ary_push(rbResult, symbol_down);
      if (isPressed(gamePad->leftState, duration, delay, interval))
        rb_ary_push(rbResult, symbol_left);
      if (isPressed(gamePad->rightState, duration, delay, interval))
        rb_ary_push(rbResult, symbol_right);
      if (isPressed(gamePad->upState, duration, delay, interval))
        rb_ary_push(rbResult, symbol_up);
      for (int i = 0; i < gamePad->buttonCount; i++)
        if (isPressed(gamePad->buttonStates[i], duration, delay, interval))
          rb_ary_push(rbResult, INT2NUM(i + 1));
    }
  } else if (rbDevice == symbol_mouse) {
    if (isPressed(mouse->leftState, duration, delay, interval))
      rb_ary_push(rbResult, symbol_left);
    if (isPressed(mouse->middleState, duration, delay, interval))
      rb_ary_push(rbResult, symbol_middle);
    if (isPressed(mouse->rightState, duration, delay, interval))
      rb_ary_push(rbResult, symbol_right);
  }

  OBJ_FREEZE(rbResult);
  return rbResult;
}

void UpdateInput()
{
  SDL_JoystickUpdate();

  Uint8* sdlKeyState = SDL_GetKeyState(NULL);
  KeyboardKey* key = keyboardKeys;
  while (key) {
    if (sdlKeyState[key->sdlKey])
      key->state++;
    else
      key->state = 0;
    key = key->next;
  }

  for (int i = 0; i < gamePadCount; i++) {
    GamePad* gamePad = &(gamePads[i]);
    if (SDL_JoystickGetAxis(gamePad->sdlJoystick, 1) > 3200)
      gamePad->downState++;
    else
      gamePad->downState = 0;
    if (SDL_JoystickGetAxis(gamePad->sdlJoystick, 0) < -3200)
      gamePad->leftState++;
    else
      gamePad->leftState = 0;
    if (SDL_JoystickGetAxis(gamePad->sdlJoystick, 0) > 3200)
      gamePad->rightState++;
    else
      gamePad->rightState = 0;
    if (SDL_JoystickGetAxis(gamePad->sdlJoystick, 1) < -3200)
      gamePad->upState++;
    else
      gamePad->upState = 0;
    for (int j = 0; j < gamePad->buttonCount; j++)
      if (SDL_JoystickGetButton(gamePad->sdlJoystick, j))
        gamePad->buttonStates[j]++;
      else
        gamePad->buttonStates[j] = 0;
  }

  int mouseLocationX, mouseLocationY;
  Uint8 sdlMouseButtons = SDL_GetMouseState(&mouseLocationX, &mouseLocationY);
  VALUE rbMouseLocation = rb_assoc_new(INT2NUM(mouseLocationX),
                                       INT2NUM(mouseLocationY));
  OBJ_FREEZE(rbMouseLocation);

  if (sdlMouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT))
    mouse->leftState++;
  else
    mouse->leftState = 0;
  if (sdlMouseButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE))
    mouse->middleState++;
  else
    mouse->middleState = 0;
  if (sdlMouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT))
    mouse->rightState++;
  else
    mouse->rightState = 0;
  
  rb_iv_set(rb_mInput, "mouse_location", rbMouseLocation);
}

void InitializeSdlInput()
{
  keyboardKeys = ALLOC(KeyboardKey);
  keyboardKeys->rbSymbol = Qundef; // dummy
  keyboardKeys->sdlKey   = 0;
  keyboardKeys->state    = 0;
  keyboardKeys->next     = NULL;
  
  KeyboardKey* currentKey = keyboardKeys;
  for (int i = 0; i < SDLK_z - SDLK_a + 1; i++)
    ADD_KEY(currentKey, ((char[]){'a' + i, '\0'}), SDLK_a + i);
  for (int i = 0; i <= 9; i++)
    ADD_KEY(currentKey, ((char[]){'d', '0' + i, '\0'}), SDLK_0 + i);
  for (int i = 0; i < 15; i++) {
    char name[4];
    snprintf(name, sizeof(name), "f%d", i + 1);
    ADD_KEY(currentKey, name, SDLK_F1 + i);
  }
  for (int i = 0; i <= 9; i++)
    ADD_KEY(currentKey, ((char[]){'n','u','m','p','a','d', '0' + i, '\0'}),
            SDLK_KP0 + i);
  char* names[] = {
    "add",
    "back",
    "capslock", "clear",
    "decimal", "delete", "divide", "down",
    "end", "enter", "escape",
    "help", "home",
    "insert",
    "lcontrolkey", "left", "lmenu", "lshiftkey", "lwin",
    "multiply",
    "numlock",
    "pagedown", "pageup",
    "rcontrolkey", "right", "rmenu", "rshiftkey", "rwin",
    "scroll", "space", "subtract",
    "tab",
    "up",
    NULL,
  };
  SDLKey sdlKeys[] = {
    SDLK_KP_PLUS,
    SDLK_BACKSPACE,
    SDLK_CAPSLOCK, SDLK_CLEAR,
    SDLK_PERIOD, SDLK_DELETE, SDLK_KP_DIVIDE, SDLK_DOWN,
    SDLK_END, SDLK_RETURN, SDLK_ESCAPE,
    SDLK_HELP, SDLK_HOME,
    SDLK_INSERT,
    SDLK_LCTRL, SDLK_LEFT, SDLK_LALT, SDLK_LSHIFT, SDLK_LSUPER,
    SDLK_KP_MULTIPLY, SDLK_NUMLOCK,
    SDLK_PAGEDOWN, SDLK_PAGEUP,
    SDLK_RCTRL, SDLK_RIGHT, SDLK_RALT, SDLK_RSHIFT, SDLK_RSUPER,
    SDLK_SCROLLOCK, SDLK_SPACE, SDLK_KP_MINUS,
    SDLK_TAB,
    SDLK_UP,
    -1,
  };
  char** name = names;
  for (SDLKey* sdlKey = sdlKeys;
       *name;
       name++, sdlKey++)
    ADD_KEY(currentKey, *name, *sdlKey);

  SDL_JoystickEventState(SDL_ENABLE);
  gamePadCount = SDL_NumJoysticks();
  gamePads = ALLOC_N(GamePad, gamePadCount);
  MEMZERO(gamePads, GamePad, gamePadCount);
  for (int i = 0; i < gamePadCount; i++) {
    SDL_Joystick* ptr = gamePads[i].sdlJoystick = SDL_JoystickOpen(i);
    if (ptr == NULL)
      rb_raise_sdl_error();
    int buttonCount = gamePads[i].buttonCount = SDL_JoystickNumButtons(ptr);
    gamePads[i].buttonStates = ALLOC_N(int, buttonCount);
    MEMZERO(gamePads[i].buttonStates, int, buttonCount);
  }

  mouse = ALLOC(Mouse);
  MEMZERO(mouse, Mouse, 1);
}

void FinalizeSdlInput()
{
  free(mouse);
  mouse = NULL;
  
  for (int i = 0; i < gamePadCount; i++) {
    GamePad* gamePad = &(gamePads[i]);
    if (SDL_JoystickOpened(i))
      SDL_JoystickClose(gamePad->sdlJoystick);
    gamePad->sdlJoystick = NULL;
    free(gamePad->buttonStates);
    gamePad->buttonStates = NULL;
  }
  free(gamePads);
  gamePads = NULL;
  
  KeyboardKey* key = keyboardKeys;
  while (key) {
    KeyboardKey* nextKey = key->next;
    free(key);
    key = nextKey;
  }
  keyboardKeys = NULL;
}

void InitializeInput(void)
{
  rb_mInput = rb_define_module_under(rb_mStarRuby, "Input");
  rb_define_singleton_method(rb_mInput, "mouse_location",
                             Input_mouse_location, 0);
  rb_define_singleton_method(rb_mInput, "pressed_keys",
                             Input_pressed_keys,   -1);

  symbol_delay         = ID2SYM(rb_intern("delay"));
  symbol_device_number = ID2SYM(rb_intern("device_number"));
  symbol_down          = ID2SYM(rb_intern("down"));
  symbol_duration      = ID2SYM(rb_intern("duration"));
  symbol_game_pad      = ID2SYM(rb_intern("game_pad"));
  symbol_interval      = ID2SYM(rb_intern("interval"));
  symbol_keyboard      = ID2SYM(rb_intern("keyboard"));
  symbol_left          = ID2SYM(rb_intern("left"));
  symbol_middle        = ID2SYM(rb_intern("middle"));
  symbol_mouse         = ID2SYM(rb_intern("mouse"));
  symbol_right         = ID2SYM(rb_intern("right"));
  symbol_up            = ID2SYM(rb_intern("up"));
}

#ifdef DEBUG
static KeyboardKey* searchKey(const char* name)
{
  VALUE rbNameSymbol = ID2SYM(rb_intern(name));
  KeyboardKey* key = keyboardKeys;
  while (key) {
    if (key->rbSymbol == rbNameSymbol)
      return key;
    key = key->next;
  }
  return NULL;
}

void TestInput(void)
{
  printf("Begin Test: Input\n");

  assert(SDLK_a == searchKey("a")->sdlKey);
  assert(SDLK_b == searchKey("b")->sdlKey);
  assert(SDLK_c == searchKey("c")->sdlKey);
  assert(SDLK_e == searchKey("e")->sdlKey);
  assert(SDLK_i == searchKey("i")->sdlKey);
  assert(SDLK_q == searchKey("q")->sdlKey);
  assert(SDLK_z == searchKey("z")->sdlKey);

  assert(SDLK_0 == searchKey("d0")->sdlKey);
  assert(SDLK_1 == searchKey("d1")->sdlKey);
  assert(SDLK_2 == searchKey("d2")->sdlKey);
  assert(SDLK_4 == searchKey("d4")->sdlKey);
  assert(SDLK_8 == searchKey("d8")->sdlKey);
  assert(SDLK_9 == searchKey("d9")->sdlKey);

  assert(SDLK_F1  == searchKey("f1")->sdlKey);
  assert(SDLK_F2  == searchKey("f2")->sdlKey);
  assert(SDLK_F4  == searchKey("f4")->sdlKey);
  assert(SDLK_F8  == searchKey("f8")->sdlKey);
  assert(SDLK_F15 == searchKey("f15")->sdlKey);

  assert(SDLK_KP0 == searchKey("numpad0")->sdlKey);
  assert(SDLK_KP1 == searchKey("numpad1")->sdlKey);
  assert(SDLK_KP2 == searchKey("numpad2")->sdlKey);
  assert(SDLK_KP4 == searchKey("numpad4")->sdlKey);
  assert(SDLK_KP8 == searchKey("numpad8")->sdlKey);
  assert(SDLK_KP9 == searchKey("numpad9")->sdlKey);

  assert(SDLK_DOWN  == searchKey("down")->sdlKey);
  assert(SDLK_LEFT  == searchKey("left")->sdlKey);
  assert(SDLK_RIGHT == searchKey("right")->sdlKey);
  assert(SDLK_UP    == searchKey("up")->sdlKey);

  for (KeyboardKey* key = keyboardKeys; key; key = key->next)
    assert(0 == key->state);

  assert(false == isPressed(0, -1, -1, 0));
  assert(true  == isPressed(1, -1, -1, 0));
  assert(true  == isPressed(2, -1, -1, 0));
  assert(true  == isPressed(3, -1, -1, 0));
  assert(true  == isPressed(4, -1, -1, 0));
  assert(true  == isPressed(5, -1, -1, 0));
  assert(true  == isPressed(100, -1, -1, 0));

  assert(false == isPressed(0, 0, -1, 0));
  assert(false == isPressed(1, 0, -1, 0));
  assert(false == isPressed(2, 0, -1, 0));
  assert(false == isPressed(3, 0, -1, 0));
  assert(false == isPressed(4, 0, -1, 0));
  assert(false == isPressed(5, 0, -1, 0));
  assert(false == isPressed(100, 0, -1, 0));

  assert(false == isPressed(0, 1, -1, 0));
  assert(true  == isPressed(1, 1, -1, 0));
  assert(false == isPressed(2, 1, -1, 0));
  assert(false == isPressed(3, 1, -1, 0));
  assert(false == isPressed(4, 1, -1, 0));
  assert(false == isPressed(5, 1, -1, 0));
  assert(false == isPressed(100, 1, -1, 0));

  assert(false == isPressed(0, 3, -1, 0));
  assert(true  == isPressed(1, 3, -1, 0));
  assert(true  == isPressed(2, 3, -1, 0));
  assert(true  == isPressed(3, 3, -1, 0));
  assert(false == isPressed(4, 3, -1, 0));
  assert(false == isPressed(5, 3, -1, 0));
  assert(false == isPressed(100, 3, -1, 0));

  // with delay
  assert(false == isPressed(0, -1, 3, 0));
  assert(true  == isPressed(1, -1, 3, 0));
  assert(true  == isPressed(2, -1, 3, 0));
  assert(true  == isPressed(3, -1, 3, 0));
  assert(true  == isPressed(4, -1, 3, 0));
  assert(true  == isPressed(5, -1, 3, 0));
  assert(true  == isPressed(100, -1, 3, 0));

  assert(false == isPressed(0, 0, 3, 0));
  assert(false == isPressed(1, 0, 3, 0));
  assert(false == isPressed(2, 0, 3, 0));
  assert(false == isPressed(3, 0, 3, 0));
  assert(false == isPressed(4, 0, 3, 0));
  assert(false == isPressed(5, 0, 3, 0));
  assert(false == isPressed(100, 0, 3, 0));

  assert(false == isPressed(0, 1, 3, 0));
  assert(true  == isPressed(1, 1, 3, 0));
  assert(false == isPressed(2, 1, 3, 0));
  assert(false == isPressed(3, 1, 3, 0));
  assert(false == isPressed(4, 1, 3, 0));
  assert(true  == isPressed(5, 1, 3, 0));
  assert(true  == isPressed(6, 1, 3, 0));
  assert(true  == isPressed(7, 1, 3, 0));
  assert(true  == isPressed(8, 1, 3, 0));
  assert(true  == isPressed(100, 1, 3, 0));

  assert(false == isPressed(0, 3, 3, 0));
  assert(true  == isPressed(1, 3, 3, 0));
  assert(true  == isPressed(2, 3, 3, 0));
  assert(true  == isPressed(3, 3, 3, 0));
  assert(false == isPressed(4, 3, 3, 0));
  assert(false == isPressed(5, 3, 3, 0));
  assert(false == isPressed(6, 3, 3, 0));
  assert(true  == isPressed(7, 3, 3, 0));
  assert(true  == isPressed(8, 3, 3, 0));
  assert(true  == isPressed(100, 3, 3, 0));

  // with delay and interval
  assert(false == isPressed(0, -1, 3, 1));
  assert(true  == isPressed(1, -1, 3, 1));
  assert(true  == isPressed(2, -1, 3, 1));
  assert(true  == isPressed(3, -1, 3, 1));
  assert(true  == isPressed(4, -1, 3, 1));
  assert(true  == isPressed(5, -1, 3, 1));
  assert(true  == isPressed(100, -1, 3, 1));

  assert(false == isPressed(0, 0, 3, 1));
  assert(false == isPressed(1, 0, 3, 1));
  assert(false == isPressed(2, 0, 3, 1));
  assert(false == isPressed(3, 0, 3, 1));
  assert(false == isPressed(4, 0, 3, 1));
  assert(false == isPressed(5, 0, 3, 1));
  assert(false == isPressed(100, 0, 3, 1));

  assert(false == isPressed(0, 1, 3, 1));
  assert(true  == isPressed(1, 1, 3, 1));
  assert(false == isPressed(2, 1, 3, 1));
  assert(false == isPressed(3, 1, 3, 1));
  assert(false == isPressed(4, 1, 3, 1));
  assert(true  == isPressed(5, 1, 3, 1));
  assert(false == isPressed(6, 1, 3, 1));
  assert(true  == isPressed(7, 1, 3, 1));
  assert(false == isPressed(8, 1, 3, 1));
  assert(true  == isPressed(9, 1, 3, 1));
  assert(false == isPressed(10, 1, 3, 1));
  assert(true  == isPressed(11, 1, 3, 1));
  assert(false == isPressed(12, 1, 3, 1));
  assert(false == isPressed(100, 1, 3, 1));

  assert(false == isPressed(0, 3, 3, 1));
  assert(true  == isPressed(1, 3, 3, 1));
  assert(true  == isPressed(2, 3, 3, 1));
  assert(true  == isPressed(3, 3, 3, 1));
  assert(false == isPressed(4, 3, 3, 1));
  assert(false == isPressed(5, 3, 3, 1));
  assert(false == isPressed(6, 3, 3, 1));
  assert(true  == isPressed(7, 3, 3, 1));
  assert(false == isPressed(8, 3, 3, 1));
  assert(true  == isPressed(9, 3, 3, 1));
  assert(false == isPressed(10, 3, 3, 1));
  assert(true  == isPressed(11, 3, 3, 1));
  assert(false == isPressed(12, 3, 3, 1));
  assert(false == isPressed(100, 3, 3, 1));

  // with delay and interval 2
  assert(false == isPressed(0, -1, 3, 2));
  assert(true  == isPressed(1, -1, 3, 2));
  assert(true  == isPressed(2, -1, 3, 2));
  assert(true  == isPressed(3, -1, 3, 2));
  assert(true  == isPressed(4, -1, 3, 2));
  assert(true  == isPressed(5, -1, 3, 2));
  assert(true  == isPressed(100, -1, 3, 2));

  assert(false == isPressed(0, 0, 3, 2));
  assert(false == isPressed(1, 0, 3, 2));
  assert(false == isPressed(2, 0, 3, 2));
  assert(false == isPressed(3, 0, 3, 2));
  assert(false == isPressed(4, 0, 3, 2));
  assert(false == isPressed(5, 0, 3, 2));
  assert(false == isPressed(100, 0, 3, 2));

  assert(false == isPressed(0, 1, 3, 2));
  assert(true  == isPressed(1, 1, 3, 2));
  assert(false == isPressed(2, 1, 3, 2));
  assert(false == isPressed(3, 1, 3, 2));
  assert(false == isPressed(4, 1, 3, 2));
  assert(true  == isPressed(5, 1, 3, 2));
  assert(false == isPressed(6, 1, 3, 2));
  assert(false == isPressed(7, 1, 3, 2));
  assert(true  == isPressed(8, 1, 3, 2));
  assert(false == isPressed(9, 1, 3, 2));
  assert(false == isPressed(10, 1, 3, 2));
  assert(true  == isPressed(11, 1, 3, 2));
  assert(false == isPressed(12, 1, 3, 2));
  assert(false == isPressed(100, 1, 3, 2));

  assert(false == isPressed(0, 3, 3, 2));
  assert(true  == isPressed(1, 3, 3, 2));
  assert(true  == isPressed(2, 3, 3, 2));
  assert(true  == isPressed(3, 3, 3, 2));
  assert(false == isPressed(4, 3, 3, 2));
  assert(false == isPressed(5, 3, 3, 2));
  assert(false == isPressed(6, 3, 3, 2));
  assert(true  == isPressed(7, 3, 3, 2));
  assert(false == isPressed(8, 3, 3, 2));
  assert(false == isPressed(9, 3, 3, 2));
  assert(true  == isPressed(10, 3, 3, 2));
  assert(false == isPressed(11, 3, 3, 2));
  assert(false == isPressed(12, 3, 3, 2));
  assert(true  == isPressed(100, 3, 3, 2));

  printf("End Test: Input\n");
}
#endif