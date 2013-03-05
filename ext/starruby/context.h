#ifndef STARRUBY_CONTEXT_H
  #define STARRUBY_CONTEXT_H

  typedef struct
  {
    VALUE texture;
    VALUE clip_rect;
    uint8_t blend_type, alpha;
  } Context;

#endif
