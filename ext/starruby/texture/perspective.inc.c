static VALUE
Texture_transform_in_perspective(int argc, VALUE* argv, VALUE self)
{
  const Texture* texture;
  Data_Get_Struct(self, Texture, texture);
  strb_TextureCheckDisposed(texture);
  volatile VALUE rbX, rbY, rbHeight, rbOptions;
  rb_scan_args(argc, argv, "31", &rbX, &rbY, &rbHeight, &rbOptions);
  if (NIL_P(rbOptions)) {
    rbOptions = rb_hash_new();
  }
  const int screenWidth = texture->width;
  PerspectiveOptions options;
  AssignPerspectiveOptions(&options, rbOptions, texture);
  const double cosYaw   = cos(options.cameraYaw);
  const double sinYaw   = sin(options.cameraYaw);
  const double cosPitch = cos(options.cameraPitch);
  const double sinPitch = sin(options.cameraPitch);
  const double cosRoll  = cos(options.cameraRoll);
  const double sinRoll  = sin(options.cameraRoll);
  double x = NUM2INT(rbX) - options.cameraX;
  double y = NUM2DBL(rbHeight);
  double z = NUM2INT(rbY) - options.cameraY;
  double x2, y2, z2;
  x2 = cosYaw  * x + sinYaw * z;
  z2 = -sinYaw * x + cosYaw * z;
  x = x2;
  z = z2;
  y2 = sinPitch * z + cosPitch * (y - options.cameraHeight)
    + options.cameraHeight;
  z2 = cosPitch * z - sinPitch * (y - options.cameraHeight);
  y = y2;
  z = z2;
  volatile VALUE rbResult = rb_ary_new3(3, Qnil, Qnil, Qnil);
  OBJ_FREEZE(rbResult);
  if (z == 0) {
    return rbResult;
  }
  const double distance = screenWidth / (2 * tan(options.viewAngle / 2));
  const double scale = -distance / z;
  const double screenX = x * scale;
  const double screenY = (options.cameraHeight - y) * scale;
  const long screenXLong =
    (long)(cosRoll  * screenX + sinRoll * screenY + options.intersectionX);
  const long screenYLong =
    (long)(-sinRoll * screenX + cosRoll * screenY + options.intersectionY);
  if (FIXABLE(screenXLong) && INT_MIN <= screenXLong && screenXLong <= INT_MAX) {
    RARRAY_PTR(rbResult)[0] = LONG2FIX(screenXLong);
  } else {
    RARRAY_PTR(rbResult)[0] = Qnil;
  }
  if (FIXABLE(screenYLong) && INT_MIN <= screenYLong && screenYLong <= INT_MAX) {
    RARRAY_PTR(rbResult)[1] = LONG2FIX(screenYLong);
  } else {
    RARRAY_PTR(rbResult)[1] = Qnil;
  }
  RARRAY_PTR(rbResult)[2] = rb_float_new(scale);
  return rbResult;
}
