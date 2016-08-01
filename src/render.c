#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include "render.h"
#include "libphone.h"
#include "upng.h"
#include "link.h"

#if __ANDROID__
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
   phoneInitJava(vm);
   return JNI_VERSION_1_6;
}
#elif __APPLE__
int main(int argc, char *argv[]) {
  return phoneDefaultAppEntry(argc, argv);
}
#endif

#define FRAGMENT_SHADER                                           \
  "varying lowp vec2 TexCoordOut;"                                \
  "uniform sampler2D Texture;"                                    \
  "void main(void) {"                                             \
  "    gl_FragColor = texture2D(Texture, TexCoordOut);"           \
  "}"

#define VERTEX_SHADER                                             \
  "attribute vec4 Position;"                                      \
  "attribute vec2 TexCoordIn;"                                    \
  "varying vec2 TexCoordOut;"                                     \
  "void main(void) {"                                             \
  "    gl_Position = Position;"                                   \
  "    TexCoordOut = TexCoordIn;"                                 \
  "}"

const GLubyte fixedIndices[] = {
   0, 1, 2,
   2, 3, 0
};

typedef struct {
  float position[3];
  float texCoord[2];
} vertex;

typedef struct texture2d {
  char assetName[780];
  int refs;
  GLuint id;
  int width;
  int height;
  struct texture2d *next;
  struct texture2d *prev;
} texture2d;

typedef struct imageFrame2d {
  texture2d *tex;
  float left;
  float top;
  float width;
  float height;
  float offsetX;
  float offsetY;
  int fliped:1;
} imageFrame2d;

typedef struct sprite2d {
  GLuint idx;
  GLuint texId;
  int hidden;
  int x;
  int y;
  GLuint vertexBuffer;
  vertex vertices[4];
  struct sprite2d *prev;
  struct sprite2d *next;
  int layer;
} sprite2d;

typedef struct imageFrame2dNode {
  struct imageFrame2dNode *next;
  struct imageFrame2dNode *prev;
  imageFrame2d *frame;
  unsigned int stopDuration;
} imageFrame2dNode;

typedef struct animation2d {
  imageFrame2dNode *firstFrame;
  imageFrame2dNode *lastFrame;
  imageFrame2dNode *currentFrame;
  unsigned int lastStopDuration;
  unsigned long long startTime;
} animation2d;

typedef struct gameContext {
  int glView;
  volatile int width;
  volatile int height;
  GLuint colorRenderBuffer;
  GLuint framebuffer;
  GLuint positionSlot;
  GLuint vertexShader;
  GLuint fragmentShader;
  GLuint program;
  GLuint texCoordSlot;
  GLuint textureUniform;
  GLuint indexBuffer;
  int disableOpenGLErrorOutput:1;
  int inited:1;
  texture2d *firstTex;
  texture2d *lastTex;
  sprite2d *firstSprite[MAX_LAYER + 1];
  sprite2d *lastSprite[MAX_LAYER + 1];
  float clearColorR;
  float clearColorG;
  float clearColorB;
} gameContext;

static gameContext gameStruct = {0};
static gameContext *game = &gameStruct;

#define checkOpenGLError() do {                                            \
  if (!game->disableOpenGLErrorOutput) {                                   \
    GLenum err;                                                            \
    err = glGetError();                                                    \
    if (GL_NO_ERROR != err) {                                              \
      phoneLog(PHONE_LOG_ERROR, __FUNCTION__, "glGetError: 0x%x %s:%u",    \
        err, __FILE__, __LINE__);                                          \
    }                                                                      \
  }                                                                        \
} while (0)

/////////////////////////////// C API ////////////////////////////////////

static texture2d *findTexture(const char *assetName) {
  texture2d *loop = game->firstTex;
  while (loop) {
    if (0 == strcmp(assetName, loop->assetName)) {
      return loop;
    }
    loop = loop->next;
  }
  return 0;
}

static texture2d *openTexture(const char *assetName) {
  texture2d *tex = findTexture(assetName);
  if (!tex) {
    unsigned char *imageData;
    upng_t *upng;
    int imageDataSize = 0;
    imageData = (unsigned char *)assetLoadString(assetName, &imageDataSize);
    upng = upng_new_from_bytes(imageData, imageDataSize);
    if (!upng) {
      phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "upng decode %s failed",
        assetName);
      free(imageData);
      return 0;
    }
    if (0 != upng_decode(upng)) {
      upng_free(upng);
      free(imageData);
      return 0;
    }
    tex = (texture2d *)calloc(1, sizeof(texture2d));
    if (!tex) {
      upng_free(upng);
      free(imageData);
      return 0;
    }
    addToLink(tex, game->firstTex, game->lastTex);
    phoneCopyString(tex->assetName, sizeof(tex->assetName), assetName);

    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    tex->width = upng_get_width(upng);
    tex->height = upng_get_height(upng);
    assert(upng_get_size(upng) == tex->width * tex->height * 4);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height,
      0, GL_RGBA, GL_UNSIGNED_BYTE, upng_get_buffer(upng));

    glBindTexture(GL_TEXTURE_2D, 0);

    upng_free(upng);
    free(imageData);
  }
  return tex;
}

static void removeTexture(texture2d *tex) {
  if (tex->id) {
    glDeleteTextures(1, &tex->id);
    tex->id = 0;
  }
  removeFromLink(tex, game->firstTex, game->lastTex);
  free(tex);
}

static void addTextureRef(texture2d *tex) {
  ++tex->refs;
}

static void delTextureRef(texture2d *tex) {
  --tex->refs;
  if (0 == tex->refs) {
    removeTexture(tex);
  }
}

unsigned long long now(void) {
  struct timeval time;
  unsigned long long millisecs;
  gettimeofday(&time, 0);
  millisecs = ((unsigned long long)time.tv_sec * 1000) + (time.tv_usec / 1000);
  return millisecs;
}

void sprite2dSetLayer(sprite2d *sprt, int layer) {
  assert(layer >= 0 && layer <= MAX_LAYER);
  if (sprt->layer != layer) {
    removeFromLink(sprt, game->firstSprite[sprt->layer],
      game->lastSprite[sprt->layer]);
    sprt->layer = layer;
    addToLink(sprt, game->firstSprite[sprt->layer],
      game->lastSprite[sprt->layer]);
  }
}

sprite2d *sprite2dCreate(void) {
  sprite2d *sprt = (sprite2d *)calloc(1, sizeof(sprite2d));
  if (!sprt) {
    return 0;
  }
  addToLink(sprt, game->firstSprite[sprt->layer],
    game->lastSprite[sprt->layer]);
  return sprt;
}

void sprite2dRemove(sprite2d *sprt) {
  if (sprt->vertexBuffer) {
    glDeleteBuffers(1, &sprt->vertexBuffer);
    sprt->vertexBuffer = 0;
  }
  removeFromLink(sprt, game->firstSprite[sprt->layer],
    game->lastSprite[sprt->layer]);
  free(sprt);
}

void sprite2dShow(sprite2d *sprt) {
  sprt->hidden = 0;
}

void sprite2dHide(sprite2d *sprt) {
  sprt->hidden = 1;
}

void sprite2dSetX(sprite2d *sprt, int x) {
  sprt->x = x;
}

void sprite2dSetY(sprite2d *sprt, int y) {
  sprt->y = y;
}

void sprite2dRender(sprite2d *sprt, imageFrame2d *frame) {
  float frameWidthOnTex = frame->fliped ? frame->height : frame->width;
  float frameHeightOnTex = frame->fliped ? frame->width : frame->height;
  float spriteLeft = sprt->x + frame->offsetX;
  float spriteTop = game->height + game->height - (sprt->y + frame->offsetY) -
    frame->height;
  float onCanvasLeft = spriteLeft / (game->width) - 1;
  float onCanvasTop = spriteTop / (game->height) - 1;
  float onCanvasRight = (spriteLeft + frame->width) / (game->width) - 1;
  float onCanvasBottom = (spriteTop + frame->height) / (game->height) - 1;
  float texLeft = frame->left / frame->tex->width;
  float texTop = frame->top / frame->tex->height;
  float texRight = (frame->left + frameWidthOnTex) / frame->tex->width;
  float texBottom = (frame->top + frameHeightOnTex) / frame->tex->height;
  vertex vertices[4];
  if (frame->fliped) {
    vertex rotatedVertices[4] = {
      {{onCanvasRight, onCanvasBottom, 0}, {texLeft, texBottom}},
      {{onCanvasRight, onCanvasTop, 0}, {texRight, texBottom}},
      {{onCanvasLeft, onCanvasTop, 0}, {texRight, texTop}},
      {{onCanvasLeft, onCanvasBottom, 0}, {texLeft, texTop}},
    };
    memcpy(vertices, rotatedVertices, sizeof(vertices));
  } else {
    vertex unrotatedVertices[4] = {
      {{onCanvasRight, onCanvasBottom, 0}, {texRight, texTop}},
      {{onCanvasRight, onCanvasTop, 0}, {texRight, texBottom}},
      {{onCanvasLeft, onCanvasTop, 0}, {texLeft, texBottom}},
      {{onCanvasLeft, onCanvasBottom, 0}, {texLeft, texTop}},
    };
    memcpy(vertices, unrotatedVertices, sizeof(vertices));
  }
  if (!sprt->vertexBuffer ||
      0 != memcmp(sprt->vertices, vertices, sizeof(vertices))) {
    sprt->texId = frame->tex->id;
    memcpy(sprt->vertices, vertices, sizeof(vertices));
    if (0 == sprt->vertexBuffer) {
      glGenBuffers(1, &sprt->vertexBuffer);
      assert(sprt->vertexBuffer);
      checkOpenGLError();
    }
    glBindBuffer(GL_ARRAY_BUFFER, sprt->vertexBuffer);
    checkOpenGLError();
    glBufferData(GL_ARRAY_BUFFER, sizeof(sprt->vertices), sprt->vertices,
      GL_STATIC_DRAW);
    checkOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkOpenGLError();
  }
}

animation2d *animation2dCreate(void) {
  animation2d *ani = (animation2d *)calloc(1, sizeof(animation2d));
  if (!ani) {
    return 0;
  }
  ani->currentFrame = 0;
  return ani;
}

int animation2dAddFrame(animation2d *ani, imageFrame2d *frame, int duration) {
  imageFrame2dNode *n = (imageFrame2dNode *)calloc(1, sizeof(imageFrame2dNode));
  if (!n) {
    return -1;
  }
  ani->lastStopDuration += duration;
  n->stopDuration = ani->lastStopDuration;
  n->frame = frame;
  addToLink(n, ani->firstFrame, ani->lastFrame);
  return 0;
}

imageFrame2d *animationQueryCurrentFrame(animation2d *ani) {
  unsigned long long currentTime = now();
  if (0 == ani->startTime) {
    ani->startTime = currentTime;
    ani->currentFrame = ani->firstFrame;
  }
  while (ani->currentFrame) {
    if (currentTime < ani->startTime + ani->currentFrame->stopDuration) {
      return ani->currentFrame->frame;
    }
    ani->currentFrame = ani->currentFrame->next;
  }
  ani->startTime = currentTime;
  ani->currentFrame = ani->firstFrame;
  return ani->currentFrame->frame;
}

static void drawSprite(sprite2d *sprt) {
  glActiveTexture(GL_TEXTURE0);
  checkOpenGLError();
  assert(sprt->texId);
  glBindTexture(GL_TEXTURE_2D, sprt->texId);
  checkOpenGLError();
  glUniform1i(game->textureUniform, 0);
  checkOpenGLError();

  glBindBuffer(GL_ARRAY_BUFFER, sprt->vertexBuffer);
  checkOpenGLError();

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, game->indexBuffer);
  checkOpenGLError();

  glVertexAttribPointer(game->positionSlot, 3, GL_FLOAT, GL_FALSE,
      sizeof(vertex), 0);
  checkOpenGLError();
  glVertexAttribPointer(game->texCoordSlot, 2, GL_FLOAT, GL_FALSE,
      sizeof(vertex), (GLvoid*)(sizeof(float) * 3));
  checkOpenGLError();

  glDrawElements(GL_TRIANGLES, sizeof(fixedIndices) / sizeof(fixedIndices[0]),
      GL_UNSIGNED_BYTE, 0);
  checkOpenGLError();

  glBindTexture(GL_TEXTURE_2D, 0);
  checkOpenGLError();

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  checkOpenGLError();

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  checkOpenGLError();
}

static void drawAllSprites(void) {
  sprite2d *loop;
  int layer = 0;
  for (layer = 0; layer <= MAX_LAYER; ++layer) {
    loop = game->firstSprite[layer];
    while (loop) {
      if (!loop->hidden && loop->texId) {
        drawSprite(loop);
      }
      loop = loop->next;
    }
  }
}

char *assetLoadString(const char *assetName, int *len) {
  int fileSize;
  char *fileContent;
  FILE *fp = phoneOpenAsset(assetName);
  if (!fp) {
    return 0;
  }
  fseek(fp, 0, SEEK_END);
  fileSize = ftell(fp);
  rewind(fp);
  fileContent = malloc(fileSize + 1);
  if (!fileContent) {
    fclose(fp);
    return 0;
  }
  fread(fileContent, 1, fileSize, fp);
  fileContent[fileSize] = '\0';
  if (len) {
    *len = fileSize;
  }
  fclose(fp);
  return fileContent;
}

imageFrame2d *assetLoadImageFrame2d(const char *assetName, int left, int top,
    int width, int height) {
  texture2d *tex;
  imageFrame2d *frame;
  tex = openTexture(assetName);
  if (!tex) {
    phoneLog(PHONE_LOG_ERROR, __FUNCTION__, "openTexture %s failed",
      assetName);
    return 0;
  }
  frame = (imageFrame2d *)calloc(1, sizeof(imageFrame2d));
  if (!frame) {
    phoneLog(PHONE_LOG_ERROR, __FUNCTION__, "calloc failed");
    return 0;
  }
  addTextureRef(tex);
  frame->tex = tex;
  frame->left = left;
  frame->top = top;
  frame->width = width;
  frame->height = height;
  return frame;
}

void imageFrame2dFlip(imageFrame2d *frame) {
  frame->fliped = !frame->fliped;
}

void imageFrame2dSetOffsetX(imageFrame2d *frame, int offsetX) {
  frame->offsetX = (float)offsetX;
}

void imageFrame2dSetOffsetY(imageFrame2d *frame, int offsetY) {
  frame->offsetY = (float)offsetY;
}

void imageFrame2dDispose(imageFrame2d *frame) {
  if (frame->tex) {
    delTextureRef(frame->tex);
    frame->tex = 0;
  }
}

//////////////////////////////////////////////////////////////////////////

static void layout(void);

static void appShowing(void) {
}

static void appHiding(void) {
}

static void appTerminating(void) {
}

static int appBackClick(void) {
  return PHONE_YES;
}

static void appLayoutChanging(void) {
  layout();
}

static GLuint compileShader(const char *shaderContent, int shaderLen,
    GLenum shaderType) {
  GLint compileResult;
  GLuint shaderHandle;
  shaderHandle = glCreateShader(shaderType);
  glShaderSource(shaderHandle, 1, &shaderContent, &shaderLen);
  glCompileShader(shaderHandle);
  glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileResult);
  if (GL_FALSE == compileResult) {
    GLchar messages[1024];
    glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "compile error: %s", messages);
    return 0;
  }
  return shaderHandle;
}

static GLint linkProgram(GLuint *shaders, int shaderCount) {
  GLint linkResult;
  GLuint program = glCreateProgram();
  int i;
  for (i = 0; i < shaderCount; ++i) {
    glAttachShader(program, shaders[i]);
  }
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &linkResult);
  if (GL_FALSE == linkResult) {
    GLchar messages[1024];
    glGetShaderInfoLog(program, sizeof(messages), 0, &messages[0]);
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__, "link error: %s", messages);
    return 0;
  }
  return program;
}

static void render(int handle) {
  if (game->width <= 0 || game->height <= 0) {
    return;
  }

  glEnable(GL_BLEND);
  checkOpenGLError();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  checkOpenGLError();

  glClearColor(game->clearColorR, game->clearColorG, game->clearColorB, 1.0);
  checkOpenGLError();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  checkOpenGLError();
  glEnable(GL_DEPTH_TEST);
  checkOpenGLError();
  glDepthFunc(GL_LEQUAL);
  checkOpenGLError();

  if (!game->inited) {
    unsigned int color = 0xaaaaaa;
    game->clearColorR = (float)((color & 0xff0000) >> 16) / 255;
    game->clearColorG = (float)((color & 0x00ff00) >> 8) / 255;
    game->clearColorB = (float)((color & 0x0000ff)) / 255;

    glGenRenderbuffers(1, &game->colorRenderBuffer);
    checkOpenGLError();
    glBindRenderbuffer(GL_RENDERBUFFER, game->colorRenderBuffer);
    checkOpenGLError();

    glGenFramebuffers(1, &game->framebuffer);
    checkOpenGLError();
    glBindFramebuffer(GL_FRAMEBUFFER, game->framebuffer);
    checkOpenGLError();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_RENDERBUFFER, game->colorRenderBuffer);
    checkOpenGLError();

    game->vertexShader = compileShader(VERTEX_SHADER,
      sizeof(VERTEX_SHADER) - 1, GL_VERTEX_SHADER);
    if (0 == game->vertexShader) {
      phoneLog(PHONE_LOG_ERROR, __FUNCTION__,
        "compileShader VERTEX_SHADER failed");
    }

    game->fragmentShader = compileShader(FRAGMENT_SHADER,
      sizeof(FRAGMENT_SHADER) - 1, GL_FRAGMENT_SHADER);
    if (0 == game->fragmentShader) {
      phoneLog(PHONE_LOG_ERROR, __FUNCTION__,
        "compileShader FRAGMENT_SHADER failed");
    }

    {
      GLuint shaders[] = {game->vertexShader, game->fragmentShader};
      game->program = linkProgram(shaders, sizeof(shaders) / sizeof(shaders[0]));
      if (0 == game->program) {
        phoneLog(PHONE_LOG_ERROR, __FUNCTION__,
          "linkProgram failed");
      }
    }

    glUseProgram(game->program);
    checkOpenGLError();

    game->positionSlot = glGetAttribLocation(game->program, "Position");
    game->texCoordSlot = glGetAttribLocation(game->program, "TexCoordIn");
    game->textureUniform = glGetUniformLocation(game->program, "Texture");
    glEnableVertexAttribArray(game->texCoordSlot);
    checkOpenGLError();
    glEnableVertexAttribArray(game->positionSlot);
    checkOpenGLError();

    glGenBuffers(1, &game->indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, game->indexBuffer);
    checkOpenGLError();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(fixedIndices), fixedIndices,
      GL_STATIC_DRAW);
    checkOpenGLError();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    game->disableOpenGLErrorOutput = 1;
    game->inited = 1;
  }

  update();

  drawAllSprites();

#ifndef __APPLE__
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  checkOpenGLError();
#endif
}

static void initOpenGLView(void) {
  game->glView = phoneCreateOpenGLView(0, 0);
  phoneSetOpenGLViewRenderHandler(game->glView, render);
}

int phoneMain(int argc, const char *argv[]) {
  static phoneAppNotificationHandler handler = {
    appShowing,
    appHiding,
    appTerminating,
    appBackClick,
    appLayoutChanging,
  };
  phoneSetAppNotificationHandler(&handler);
  phoneShowStatusBar(0);
  phoneSetStatusBarBackgroundColor(0xaaaaaa);
  phoneForceOrientation(PHONE_ORIENTATION_SETTING_LANDSCAPE);
  initOpenGLView();
  layout();
  return 0;
}

static void layout(void) {
  game->width = phoneGetViewWidth(0);
  game->height = phoneGetViewHeight(0);
  phoneSetViewFrame(game->glView, 0, 0,
    game->width, game->height);
}
