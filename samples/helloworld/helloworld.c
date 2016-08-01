#include "render.h"

sprite2d *wyvernSprite = 0;
animation2d *wyvernAnimation = 0;

int update(void) {
  if (!wyvernSprite) {
    wyvernSprite = sprite2dCreate();
  }
  if (!wyvernAnimation) {
    wyvernAnimation = animation2dCreate();
    {
      imageFrame2d *frame = assetLoadImageFrame2d("texture.png",
        1291, 461, 130, 147);
      imageFrame2dFlip(frame);
      imageFrame2dSetOffsetX(frame, 84);
      imageFrame2dSetOffsetY(frame, 70);
      animation2dAddFrame(wyvernAnimation, frame, 200);
    }
    {
      imageFrame2d *frame = assetLoadImageFrame2d("texture.png",
        1558, 180, 194, 180);
      imageFrame2dSetOffsetX(frame, 44);
      imageFrame2dSetOffsetY(frame, 43);
      animation2dAddFrame(wyvernAnimation, frame, 200);
    }
    {
      imageFrame2d *frame = assetLoadImageFrame2d("texture.png",
        1123, 106, 177, 217);
      imageFrame2dFlip(frame);
      imageFrame2dSetOffsetX(frame, 63);
      imageFrame2dSetOffsetY(frame, 0);
      animation2dAddFrame(wyvernAnimation, frame, 200);
    }
    {
      imageFrame2d *frame = assetLoadImageFrame2d("texture.png",
        454, 167, 210, 212);
      imageFrame2dFlip(frame);
      imageFrame2dSetOffsetX(frame, 39);
      imageFrame2dSetOffsetY(frame, 11);
      animation2dAddFrame(wyvernAnimation, frame, 200);
    }
    {
      imageFrame2d *frame = assetLoadImageFrame2d("texture.png",
        1752, 181, 194, 161);
      imageFrame2dSetOffsetX(frame, 30);
      imageFrame2dSetOffsetY(frame, 63);
      animation2dAddFrame(wyvernAnimation, frame, 200);
    }
  }
  sprite2dRender(wyvernSprite, animationQueryCurrentFrame(wyvernAnimation));
  return 0;
}
