#ifndef GRAVITY_H
#define GRAVITY_H

typedef struct sprite2d sprite2d;
typedef struct imageFrame2d imageFrame2d;

unsigned long long systemNow(void);
void systemSetBackgroundColor(int color);
sprite2d *sprite2dCreate(void);
void sprite2dRemove(sprite2d *sprt);
void sprite2dShow(sprite2d *sprt);
void sprite2dHide(sprite2d *sprt);
void sprite2dSetX(sprite2d *sprt, int x);
void sprite2dSetY(sprite2d *sprt, int y);
void sprite2dSetWidth(sprite2d *sprt, int width);
void sprite2dSetHeight(sprite2d *sprt, int height);
void sprite2dRender(sprite2d *sprt, imageFrame2d *frame);
void sprite2dSetLayer(sprite2d *sprt, int layer);
char *assetLoadString(const char *assetName, int *len);
imageFrame2d *assetLoadImageFrame2d(const char *assetName, int left, int top,
  int width, int height);
void imageFrame2dFlip(imageFrame2d *frame);
void imageFrame2dSetOffsetX(imageFrame2d *frame, int width);
void imageFrame2dSetOffsetY(imageFrame2d *frame, int height);

#endif
