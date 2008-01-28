#include "starruby.h"

void
AffineMatrix_Invert(AffineMatrix* m)
{
  double det = m->a * m->d - m->b * m->c;
  if (det == 0)
    rb_bug("invalid affine matrix");
  double newA = m->d / det;
  double newB = -m->b / det;
  double newC = -m->c / det;
  double newD = m->a / det;
  *m = (AffineMatrix) {
    .a  = newA, .b  = newB, .tx = newA * -(m->tx) + newB * -(m->ty),
    .c  = newC, .d  = newD, .ty = newC * -(m->tx) + newD * -(m->ty),
  };
}

bool
AffineMatrix_IsRegular(AffineMatrix* m)
{
  return m->a * m->d - m->b * m->c != 0;
}

void
AffineMatrix_Transform(AffineMatrix* m,
                       double x, double y,
                       double* restrict xOut, double* restrict yOut)
{
  *xOut = m->a * x + m->b * y + m->tx;
  *yOut = m->c * x + m->d * y + m->ty;
}

#ifdef DEBUG
void
TestAffineMatrix(void)
{
  printf("Begin Test: AffineMatrix\n");
  
  assert(AffineMatrix_IsRegular(&(AffineMatrix) {
    .a = 1, .b = 2, .tx = 5,
    .c = 3, .d = 4, .ty = 6,
  }));
  assert(AffineMatrix_IsRegular(&(AffineMatrix) {
    .a = 7, .b = 8,  .tx = 11,
    .c = 9, .d = 10, .ty = 12,
  }));
  assert(!AffineMatrix_IsRegular(&(AffineMatrix) {
    .a = 0, .b = 0, .tx = 13,
    .c = 0, .d = 0, .ty = 14,
  }));
  assert(!AffineMatrix_IsRegular(&(AffineMatrix) {
    .a = 1, .b = 0, .tx = 15,
    .c = 1, .d = 0, .ty = 16,
  }));
  
  AffineMatrix m3 = {
    .a = 1, .b = 2, .tx = 5,
    .c = 3, .d = 4, .ty = 6,
  };
  AffineMatrix_Invert(&m3);
  assert(m3.a  == -2);
  assert(m3.b  == 1);
  assert(m3.c  == 1.5);
  assert(m3.d  == -0.5);
  assert(m3.tx == 4);
  assert(m3.ty == -4.5);
  AffineMatrix m4 = {
    .a = 7, .b = 8,  .tx = 11,
    .c = 9, .d = 10, .ty = 12,
  };
  AffineMatrix_Invert(&m4);
  double delta = 0.00001;
  assert(fabs(m4.a  - (-5))   < delta);
  assert(fabs(m4.b  - 4)      < delta);
  assert(fabs(m4.c  - 4.5)    < delta);
  assert(fabs(m4.d  - (-3.5)) < delta);
  assert(fabs(m4.tx - 7)      < delta);
  assert(fabs(m4.ty - (-7.5)) < delta);

  AffineMatrix m5 = {
    .a = 1, .b = 2, .tx = 5,
    .c = 3, .d = 4, .ty = 6,
  };
  double x1;
  double y1;
  AffineMatrix_Transform(&m5, 7, 8, &x1, &y1);
  assert(fabs(x1 - 28) < delta);
  assert(fabs(y1 - 59) < delta);
  double x2;
  double y2;
  AffineMatrix_Transform(&m5, -9, 10, &x2, &y2);
  assert(fabs(x2 - 16) < delta);
  assert(fabs(y2 - 19) < delta);

  printf("End Test: AffineMatrix\n");
}
#endif
