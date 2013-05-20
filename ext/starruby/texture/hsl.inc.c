typedef struct {
  float hue;
  float saturation;
  float lightness;
} HSL;

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} RGB;

/* http://jswidget.com/blog/2011/04/21/rgb-color-model-and-hsl-color-model/ */
static uint8_t hslValue(float n1, float n2, float hue)
{
   float value;

   if (hue > 255) { hue -= 255; }
   else if (hue < 0) { hue += 255; }

   if (hue < 42.5) {
      value = n1 + (n2 - n1) * (hue / 42.5);
   } else if (hue < 127.5) {
      value = n2;
   } else if (hue < 170) {
      value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
   } else {
      value = n1;
   }
   return (uint8_t)(value * 255);
}

static HSL rgb2hsl(uint8_t red, uint8_t green, uint8_t blue)
{
   uint8_t r = red, g = green, b = blue;
   float h, s, l;
   uint8_t min, max;
   uint8_t delta;

   if (r > g){
      max = max(r, b);
      min = min(g, b);
   } else {
      max = max(g, b);
      min = min(r, b);
   }

   l = (max + min) / 2.0;

   if (max == min){
      s = 0.0;
      h = 0.0;
   } else {
      delta = (max - min);

      if (l < 128){
         s = 255 * delta / (max + min);
      }else{
         s = 255 * delta / (511 - max - min);
      }
      if (r == max){
         h = (g - b) / delta;
      }else if (g == max){
         h = 2 + (b - r) / delta;
      }else{
         h = 4 + (r - g) / delta;
      }

      h = h * 42.5;

      if (h < 0){ h += 255; }
      else if (h > 255){ h -= 255; }
   }
   return (HSL){h, s, l};
}

static RGB hsl2rgb(HSL hsl)
{
   float h = hsl.hue, s = hsl.saturation, l = hsl.lightness;
   if (s == 0) {
      /*  achromatic case  */
      return (RGB){l, l, l};
   } else {
      float m1, m2;
      if (l < 128) {
        m2 = (l * (255 + s)) / 65025.0;
      } else {
        m2 = (l + s - (l * s) / 255.0) / 255.0;
      }
      m1 = (l / 127.5) - m2;
      /* chromatic case */
      return (RGB){hslValue(m1, m2, h + 85),
                   hslValue(m1, m2, h),
                   hslValue(m1, m2, h - 85)};
   }
};

static blend_hue(RGB v1, RGB v2)
{
  var hsl1 = COLOR_SPACE.rgb2hsl(v1.r,v1.g,v1.b);
  var hsl2 = COLOR_SPACE.rgb2hsl(v2.r,v2.g,v2.b);
  return COLOR_SPACE.hsl2rgb(hsl1.h,hsl2.s,hsl2.l);
}
