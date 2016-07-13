#ifndef MMORPG_H
#define MMORPG_H

typedef struct sprite2d sprite2d;
typedef struct imageFrame2d imageFrame2d;

unsigned long long systemNow(void);
sprite2d *sprite2dCreate(void);
void sprite2dRemove(sprite2d *sprt);
void sprite2dShow(sprite2d *sprt);
void sprite2dHide(sprite2d *sprt);
void sprite2dSetX(sprite2d *sprt, int x);
void sprite2dSetY(sprite2d *sprt, int y);
void sprite2dSetWidth(sprite2d *sprt, int width);
void sprite2dSetHeight(sprite2d *sprt, int height);
void sprite2dRender(sprite2d *sprt, imageFrame2d *frame);
char *assetLoadString(const char *assetName, int *len);
imageFrame2d *assetLoadImageFrame2d(const char *assetName, int left, int top,
  int width, int height);
void imageFrame2dRotate(imageFrame2d *frame, int degree);
void imageFrame2dSetWidth(imageFrame2d *frame, int width);
void imageFrame2dSetHeight(imageFrame2d *frame, int height);
void imageFrame2dSetCenter(imageFrame2d *frame, int x, int y);

#endif
