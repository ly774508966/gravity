#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include "gravity.h"
#include "duktape.h"
#include "libphone.h"
#include "upng.h"
#include "matrix.h"

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
  "uniform mat4 rotationMatrixIn;"                                \
  "void main(void) {"                                             \
  "    gl_Position = rotationMatrixIn * Position;"                \
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
  int roundId;
} sprite2d;

typedef struct gameContext {
  duk_context *js;
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
  GLuint rotationMatrixSlot;
  GLuint indexBuffer;
  matrix unrotatedMatrix;
  //matrix rotatedMatrix;
  int disableOpenGLErrorOutput:1;
  int inited:1;
  texture2d *firstTex;
  texture2d *lastTex;
  sprite2d *firstSprite;
  sprite2d *lastSprite;
  float clearColorR;
  float clearColorG;
  float clearColorB;
  int roundId;
} gameContext;

static gameContext gameStruct = {0};
static gameContext *game = &gameStruct;

#define addToLink(item, first, last) do {         \
  (item)->prev = (last);                          \
  if (last) {                                     \
    (last)->next = (item);                        \
  } else {                                        \
    (first) = (item);                             \
  }                                               \
  (last) = (item);                                \
} while (0)

#define removeFromLink(item, first, last) do {    \
  if ((item)->prev) {                             \
    (item)->prev->next = (item)->next;            \
  }                                               \
  if ((item)->next) {                             \
    (item)->next->prev = (item)->prev;            \
  }                                               \
  if ((item) == (first)) {                        \
    (first) = (item)->next;                       \
  }                                               \
  if ((item) == (last)) {                         \
    (last) = (item)->prev;                        \
  }                                               \
} while (0)

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

unsigned long long systemNow(void) {
  struct timeval time;
  unsigned long long millisecs;
  gettimeofday(&time, 0);
  millisecs = ((unsigned long long)time.tv_sec * 1000) + (time.tv_usec / 1000);
  return millisecs;
}

static void setStatusBarBackgroundColor(void *tag) {
  int color = (char *)tag - 0;
  phoneSetStatusBarBackgroundColor(color);
}

void systemSetBackgroundColor(int color) {
  game->clearColorR = (float)((color & 0xff0000) >> 16) / 255;
  game->clearColorG = (float)((color & 0x00ff00) >> 8) / 255;
  game->clearColorB = (float)((color & 0x0000ff)) / 255;
  phoneRunOnMainWorkQueue(setStatusBarBackgroundColor,
    (void *)((char *)0 + color));
}

void sprite2dSetLayer(sprite2d *sprt, int layer) {
  sprt->layer = layer;
}

sprite2d *sprite2dCreate(void) {
  sprite2d *sprt = (sprite2d *)calloc(1, sizeof(sprite2d));
  if (!sprt) {
    return 0;
  }
  addToLink(sprt, game->firstSprite, game->lastSprite);
  return sprt;
}

void sprite2dRemove(sprite2d *sprt) {
  if (sprt->vertexBuffer) {
    glDeleteBuffers(1, &sprt->vertexBuffer);
    sprt->vertexBuffer = 0;
  }
  removeFromLink(sprt, game->firstSprite, game->lastSprite);
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
  float spriteLeft = sprt->x + frame->offsetX;
  float spriteTop = sprt->y + frame->offsetY;
  float frameWidthOnTex = frame->fliped ? frame->height : frame->width;
  float frameHeightOnTex = frame->fliped ? frame->width : frame->height;
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

static void drawSprite(sprite2d *sprt) {
  glActiveTexture(GL_TEXTURE0);
  checkOpenGLError();
  assert(sprt->texId);
  glBindTexture(GL_TEXTURE_2D, sprt->texId);
  checkOpenGLError();
  glUniform1i(game->textureUniform, 0);
  checkOpenGLError();

  /*
  glUniformMatrix4fv(game->rotationMatrixSlot, 1, GL_FALSE,
    sprt->rotated ? &game->rotatedMatrix.data[0] :
      &game->unrotatedMatrix.data[0]);*/
  glUniformMatrix4fv(game->rotationMatrixSlot, 1, GL_FALSE,
    &game->unrotatedMatrix.data[0]);

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
  game->roundId += 1;
  for (layer = 0; layer <= 3; ++layer) {
    loop = game->firstSprite;
    while (loop) {
      if (!loop->hidden && loop->texId && layer == loop->layer) {
        loop->roundId = game->roundId;
        drawSprite(loop);
      }
      loop = loop->next;
    }
  }
  loop = game->firstSprite;
  while (loop) {
    if (!loop->hidden && loop->texId && loop->roundId != game->roundId) {
      drawSprite(loop);
    }
    loop = loop->next;
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

//////////////////////////////////////////////////////////////////////////

/////////////////////////////// JS UTIL //////////////////////////////////

static void *getRawPointerFromThis(duk_context *js) {
  void *rawPointer;
  duk_push_this(js);
  duk_get_prop_string(js, -1, "_rawpointer");
  rawPointer = duk_to_pointer(js, -1);
  duk_pop_n(js, 2);
  return rawPointer;
}

static void *getRawPointerFromIndex(duk_context *js, duk_idx_t index) {
  void *rawPointer;
  duk_get_prop_string(js, index, "_rawpointer");
  rawPointer = duk_to_pointer(js, -1);
  duk_pop_n(js, 1);
  return rawPointer;
}

//////////////////////////////////////////////////////////////////////////

/////////////////////////////// JS API ///////////////////////////////////

static duk_ret_t System_now(duk_context *js) {
  duk_push_number(js, systemNow());
  return 1;
}

static duk_ret_t System_log(duk_context *js) {
  duk_idx_t num = duk_get_top(js);
  duk_idx_t i;
  for (i = 0; i < num; ++i) {
    phoneLog(PHONE_LOG_INFO, "gravity", "%s",
      duk_to_string(game->js, i));
  }
  return 0;
}

static duk_ret_t System_setBackgroundColor(duk_context *js) {
  int color = duk_get_int(js, 0);
  systemSetBackgroundColor(color);
  return 0;
}

static duk_ret_t System_register(duk_context *js) {
  const char *moduleName = duk_to_string(js, 0);
  duk_push_global_object(js);
  duk_push_object(js);
  duk_put_prop_string(js, -2, moduleName);
  duk_pop(js);
  return 0;
}

static duk_ret_t Sprite2D_remove(duk_context *js) {
  sprite2d *sprt = getRawPointerFromThis(js);
  sprite2dRemove(sprt);
  duk_push_this(js);
  return 1;
}

static duk_ret_t Sprite2D_setLayer(duk_context *js) {
  sprite2d *sprt = getRawPointerFromThis(js);
  int layer = duk_to_int(js, 0);
  sprite2dSetLayer(sprt, layer);
  duk_push_this(js);
  return 1;
}

static duk_ret_t Sprite2D_show(duk_context *js) {
  sprite2d *sprt = getRawPointerFromThis(js);
  sprite2dShow(sprt);
  duk_push_this(js);
  return 1;
}

static duk_ret_t Sprite2D_hide(duk_context *js) {
  sprite2d *sprt = getRawPointerFromThis(js);
  sprite2dHide(sprt);
  duk_push_this(js);
  return 1;
}

static duk_ret_t Sprite2D_setX(duk_context *js) {
  sprite2d *sprt = getRawPointerFromThis(js);
  int x = duk_to_int(js, 0);
  sprite2dSetX(sprt, x);
  duk_push_this(js);
  return 1;
}

static duk_ret_t Sprite2D_setY(duk_context *js) {
  sprite2d *sprt = getRawPointerFromThis(js);
  int y = duk_to_int(js, 0);
  sprite2dSetY(sprt, y);
  duk_push_this(js);
  return 1;
}

static duk_ret_t Sprite2D_render(duk_context *js) {
  sprite2d *sprt = getRawPointerFromThis(js);
  imageFrame2d *frame = getRawPointerFromIndex(js, 0);
  sprite2dRender(sprt, frame);
  duk_push_this(js);
  return 1;
}

static void pushSprite2dFromRawPointer(duk_context *js,
    sprite2d *sprt) {
  duk_push_object(js);
  duk_push_c_function(js, Sprite2D_remove, 0);
  duk_put_prop_string(js, -2, "remove");
  duk_push_c_function(js, Sprite2D_show, 0);
  duk_put_prop_string(js, -2, "show");
  duk_push_c_function(js, Sprite2D_hide, 0);
  duk_put_prop_string(js, -2, "hide");
  duk_push_c_function(js, Sprite2D_setX, 1);
  duk_put_prop_string(js, -2, "setX");
  duk_push_c_function(js, Sprite2D_setY, 1);
  duk_put_prop_string(js, -2, "setY");
  duk_push_c_function(js, Sprite2D_render, 1);
  duk_put_prop_string(js, -2, "render");
  duk_push_c_function(js, Sprite2D_setLayer, 1);
  duk_put_prop_string(js, -2, "setLayer");
  duk_push_pointer(js, sprt);
  duk_put_prop_string(js, -2, "_rawpointer");
}

static duk_ret_t Sprite2D_create(duk_context *js) {
  sprite2d *sprt = sprite2dCreate();
  if (!sprt) {
    phoneLog(PHONE_LOG_ERROR, __FUNCTION__, "sprite2dCreate failed");
    return 0;
  }
  sprt->layer = 1;
  pushSprite2dFromRawPointer(js, sprt);
  return 1;
}

static duk_ret_t ImageFrame2D_flip(duk_context *js) {
  imageFrame2d *frame = getRawPointerFromThis(js);
  imageFrame2dFlip(frame);
  duk_push_this(js);
  return 1;
}

static duk_ret_t ImageFrame2D_setOffsetX(duk_context *js) {
  imageFrame2d *frame = getRawPointerFromThis(js);
  int offsetX = duk_to_int(js, 0);
  imageFrame2dSetOffsetX(frame, offsetX);
  duk_push_this(js);
  return 1;
}

static duk_ret_t ImageFrame2D_setOffsetY(duk_context *js) {
  imageFrame2d *frame = getRawPointerFromThis(js);
  int offsetY = duk_to_int(js, 0);
  imageFrame2dSetOffsetY(frame, offsetY);
  duk_push_this(js);
  return 1;
}

static void pushImageFrame2dFromRawPointer(duk_context *js,
    imageFrame2d *frame) {
  duk_push_object(js);
  duk_push_c_function(js, ImageFrame2D_flip, 0);
  duk_put_prop_string(js, -2, "flip");
  duk_push_c_function(js, ImageFrame2D_setOffsetX, 1);
  duk_put_prop_string(js, -2, "setOffsetX");
  duk_push_c_function(js, ImageFrame2D_setOffsetY, 1);
  duk_put_prop_string(js, -2, "setOffsetY");
  duk_push_pointer(js, frame);
  duk_put_prop_string(js, -2, "_rawpointer");
}

static duk_ret_t Asset_loadString(duk_context *js) {
  const char *assetName = duk_to_string(js, 0);
  int len = 0;
  char *content = assetLoadString(assetName, &len);
  if (!content) {
    phoneLog(PHONE_LOG_ERROR, __FUNCTION__, "assetLoadString %s failed",
      assetName);
    return 0;
  }
  duk_push_lstring(js, content, len);
  free(content);
  return 1;
}

static duk_ret_t Asset_loadImageFrame2D(duk_context *js) {
  const char *assetName = duk_to_string(js, 0);
  int left = duk_to_int(game->js, 1);
  int top = duk_to_int(game->js, 2);
  int width = duk_to_int(game->js, 3);
  int height = duk_to_int(game->js, 4);
  imageFrame2d *frame = assetLoadImageFrame2d(assetName, left, top, width,
    height);
  if (!frame) {
    phoneLog(PHONE_LOG_ERROR, __FUNCTION__, "assetLoadImageFrame2d failed");
    return 0;
  }
  pushImageFrame2dFromRawPointer(js, frame);
  return 1;
}

//////////////////////////////////////////////////////////////////////////

////////////////////////// REGISTER JS FUNCTIONS /////////////////////////

const duk_function_list_entry systemFuncs[] = {
  {"now", System_now, 0},
  {"log", System_log, DUK_VARARGS},
  {"register", System_register, 1},
  {"setBackgroundColor", System_setBackgroundColor, 1},
  {0, 0, 0 }
};

const duk_function_list_entry sprite2dFuncs[] = {
  {"create", Sprite2D_create, 0},
  {0, 0, 0 }
};

const duk_function_list_entry assetFuncs[] = {
  {"loadString", Asset_loadString, 1},
  {"loadImageFrame2D", Asset_loadImageFrame2D, 5},
  {0, 0, 0 }
};

const duk_function_list_entry imageFrame2dFuncs[] = {
  {0, 0, 0 }
};

const struct {
  const char *name;
  const duk_function_list_entry *entry;
} objectFuncsList[] = {
  {"System", systemFuncs},
  {"Sprite2D", sprite2dFuncs},
  {"Asset", assetFuncs},
  {"ImageFrame2D", imageFrame2dFuncs},
};

static void registerJsFunctions(duk_context *js) {
  int i;
  duk_push_global_object(js);
  for (i = 0; i < sizeof(objectFuncsList) / sizeof(objectFuncsList[0]); ++i) {
    duk_push_object(js);
    duk_put_function_list(js, -1, objectFuncsList[i].entry);
    duk_put_prop_string(js, -2, objectFuncsList[i].name);
  }
  duk_pop(js);
}

///////////////////////////////////////////////////////////////////////////

const char *scriptNameList[] = {
  "animation2d.js",
  "gravity.js"
};

static void initJavascript(void) {
  duk_context *js = duk_create_heap_default();
  int i;

  game->js = js;
  registerJsFunctions(js);

  for (i = 0; i < sizeof(scriptNameList) / sizeof(scriptNameList[0]); ++i) {
    int scriptLen;
    const char *scriptName = scriptNameList[i];
    char *script = assetLoadString(scriptName, &scriptLen);
    if (0 != duk_peval_lstring(game->js, script, scriptLen)) {
      free(script);
      phoneLog(PHONE_LOG_DEBUG, __FUNCTION__,
        "duk_peval_lstring lunch %s failed: %s",
        scriptName, duk_safe_to_string(js, -1));
      return;
    }
    free(script);
  }
  duk_pop(game->js);
}

static int update(void) {
  int result = 0;
  duk_context *js = game->js;
  duk_push_global_object(js);
  duk_get_prop_string(js, -1, "update");
  if (duk_pcall(js, 0) != 0) {
    phoneLog(PHONE_LOG_ERROR, __FUNCTION__,
      "duk_pcall failed: %s\n", duk_safe_to_string(js, -1));
  } else {
    result = duk_to_boolean(js, -1);
  }
  duk_pop(js);
  return result;
}

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
    matrixLoadIdentity(&game->unrotatedMatrix);
    //matrixRotateZ(&game->rotatedMatrix, 90);

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
    game->rotationMatrixSlot = glGetUniformLocation(game->program,
      "rotationMatrixIn");
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

    initJavascript();

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
  initOpenGLView();
  layout();
  return 0;
}

static void layout(void) {
  /*
  float openGLViewWidth;
  float openGLViewHeight;
  float margin;
  float openGLViewTop;
  float padding = dp(10);

  if (phoneGetViewWidth(0) < phoneGetViewHeight(0)) {
    margin = dp(20);
    openGLViewWidth = phoneGetViewWidth(0) - margin * 2;
    openGLViewHeight = openGLViewWidth * 320 / 480;
  } else {
    margin = dp(10);
    openGLViewHeight = phoneGetViewHeight(0) - margin * 2;
    openGLViewWidth = openGLViewHeight * 480 / 320;
  }
  openGLViewTop = (phoneGetViewHeight(0) -
    openGLViewHeight) / 2 - dp(40);
  phoneSetViewFrame(game->glView, margin, openGLViewTop,
    openGLViewWidth, openGLViewHeight);*/

  game->width = phoneGetViewWidth(0);
  game->height = phoneGetViewHeight(0);
  phoneSetViewFrame(game->glView, 0, 0,
    game->width, game->height);
}
