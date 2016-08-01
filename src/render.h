#ifndef RENDER_H
#define RENDER_H

typedef struct sprite2d sprite2d;
typedef struct imageFrame2d imageFrame2d;
typedef struct animation2d animation2d;

extern int update(void);

#define MAX_LAYER 3

sprite2d *sprite2dCreate(void);
void sprite2dRemove(sprite2d *sprt);
void sprite2dShow(sprite2d *sprt);
void sprite2dHide(sprite2d *sprt);
void sprite2dSetX(sprite2d *sprt, int x);
void sprite2dSetY(sprite2d *sprt, int y);
void sprite2dRender(sprite2d *sprt, imageFrame2d *frame);
void sprite2dSetLayer(sprite2d *sprt, int layer);
char *assetLoadString(const char *assetName, int *len);
imageFrame2d *assetLoadImageFrame2d(const char *assetName, int left, int top,
  int width, int height);
void imageFrame2dFlip(imageFrame2d *frame);
void imageFrame2dSetOffsetX(imageFrame2d *frame, int width);
void imageFrame2dSetOffsetY(imageFrame2d *frame, int height);
void imageFrame2dDispose(imageFrame2d *frame);
animation2d *animation2dCreate(void);
int animation2dAddFrame(animation2d *ani, imageFrame2d *frame, int duration);
imageFrame2d *animationQueryCurrentFrame(animation2d *ani);

#endif
