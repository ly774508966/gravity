#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "duktape.h"
#include "libphone.h"

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

duk_context *js = 0;

static int jsCallGlobalVoidReturnInt(const char *funcName) {
  int result = 0;
  duk_push_global_object(js);
  duk_get_prop_string(js, -1, funcName);
  if (duk_pcall(js, 0) != 0) {
    phoneLog(PHONE_LOG_ERROR, __FUNCTION__,
      "duk_pcall failed: %s\n", duk_safe_to_string(js, -1));
  } else {
    result = duk_to_int(js, -1);
  }
  duk_pop(js);
  return result;
}

static void layout(void);

static void appShowing(void) {
  jsCallGlobalVoidReturnInt("appShowing");
}

static void appHiding(void) {
  jsCallGlobalVoidReturnInt("appHiding");
}

static void appTerminating(void) {
  jsCallGlobalVoidReturnInt("appTerminating");
}

static int appBackClick(void) {
  return jsCallGlobalVoidReturnInt("appBackClick");
}

static void appLayoutChanging(void) {
  layout();
  jsCallGlobalVoidReturnInt("appLayoutChanging");
}

static char *loadAsset(const char *resourceName, int *size) {
  char *fileContent;
  int fileSize;
  FILE *fp = phoneOpenAsset(resourceName);
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
  if (fileSize != fread(fileContent, 1, fileSize, fp)) {
    free(fileContent);
    fclose(fp);
    return 0;
  }
  fclose(fp);
  fileContent[fileSize] = '\0';
  if (size) {
    *size = fileSize;
  }
  return fileContent;
}

static void initJavascript(void) {
  int scriptLen;
  js = duk_create_heap_default();
  char *script = loadAsset("MMORPG.js", &scriptLen);
  if (0 != duk_peval_lstring(js, script, scriptLen)) {
    free(script);
    phoneLog(PHONE_LOG_DEBUG, __FUNCTION__,
      "duk_peval_lstring lunch js failed: %s", duk_safe_to_string(js, -1));
    return;
  }
  duk_pop(js);
  free(script);
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
  initJavascript();
  layout();
  return 0;
}

static void layout(void) {
  // no-op
}
