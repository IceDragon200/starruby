int blend(unsigned char result[4], unsigned char fg[4], unsigned char bg[4])
{
    unsigned int alpha = fg[3] + 1;
    unsigned int inv_alpha = 256 - fg[3];
    result[0] = (unsigned char)((alpha * fg[0] + inv_alpha * bg[0]) >> 8);
    result[1] = (unsigned char)((alpha * fg[1] + inv_alpha * bg[1]) >> 8);
    result[2] = (unsigned char)((alpha * fg[2] + inv_alpha * bg[2]) >> 8);
    result[3] = 0xff;
}
/*
I don't know how fast it is, but it's all integer. It works by turning alpha (and inv_alpha) into 8.8 fixed-point representations. Don't worry about the fact that alpha's min value is 1. In that case, fg[3] was 0, meaning the foreground is transparent. The blends will be 1*fg + 256*bg, which means that all the bits of fg will be shifted out of the result.

You could do it very fast, indeed, if you packed your RGBAs in 64 bit integers. You could then compute all three result colors in parallel with a single expression.
*/