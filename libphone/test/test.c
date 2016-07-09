#define testListMap(XX)                                                                             \
  XX("timer will trigger in 1 secs", testTimerWillTriggerInOneSeconds)                              \
  XX("container view change parent", testContainerViewChangeParent)                                 \
  XX("remove view", testRemoveView)

///////////////////////////////////////////////////////////////////////////////
#include "test.h"
#include <stdarg.h>

#if __ANDROID__
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
   phoneInitJava(vm);
   return JNI_VERSION_1_6;
}
#endif

#define XX(name, func) extern int func(testItem *item);
testListMap(XX)
#undef XX

#define XX(name, func) {name, func},
static struct {
  const char *testName;
  testItemHandler testHandler;
} testList[] = {
  testListMap(XX)
};
#undef XX

struct testContext {
  testItem *firstTestItem;
  testItem *lastTestItem;
  int postedExit:1;
};

typedef struct testItem {
  int finished:1;
  int succeed:1;
  const char *testName;
  testItemHandler testHandler;
  struct testItem *next;
  struct testItem *prev;
  void *tag;
  phoneAppNotificationHandler *appNotificationHandler;
} testItem;

static testContext testStruct;
static testContext *test = &testStruct;

static void appShowing(void) {
  testItem *loop = test->firstTestItem;
  while (loop) {
    if (loop->appNotificationHandler &&
        loop->appNotificationHandler->showing) {
      loop->appNotificationHandler->showing();
    }
    loop = loop->next;
  }
}

static void appHiding(void) {
  testItem *loop = test->firstTestItem;
  while (loop) {
    if (loop->appNotificationHandler &&
        loop->appNotificationHandler->hiding) {
      loop->appNotificationHandler->hiding();
    }
    loop = loop->next;
  }
}

static void appTerminating(void) {
  testItem *loop = test->firstTestItem;
  while (loop) {
    if (loop->appNotificationHandler &&
        loop->appNotificationHandler->terminating) {
      loop->appNotificationHandler->terminating();
    }
    loop = loop->next;
  }
}

static int appBackClick(void) {
  testItem *loop = test->firstTestItem;
  while (loop) {
    if (loop->appNotificationHandler &&
        loop->appNotificationHandler->backClick) {
      loop->appNotificationHandler->backClick();
    }
    loop = loop->next;
  }
  return PHONE_DONTCARE;
}

static void appLayoutChanging(void) {
  testItem *loop = test->firstTestItem;
  while (loop) {
    if (loop->appNotificationHandler &&
        loop->appNotificationHandler->layoutChanging) {
      loop->appNotificationHandler->layoutChanging();
    }
    loop = loop->next;
  }
}

void testSetItemTag(testItem *item, void *tag) {
  item->tag = tag;
}

void *testGetItemTag(testItem *item) {
  return item->tag;
}

void testSetItemAppNotificationHandler(testItem *item,
    phoneAppNotificationHandler *handler) {
  item->appNotificationHandler = handler;
}

void testAddItem(const char *testName, testItemHandler testHandler) {
  testItem *item = (testItem *)calloc(1, sizeof(testItem));
  testAssert(item && "Insufficient Memory!");
  item->testName = testName;
  item->testHandler = testHandler;
  if (test->lastTestItem) {
    test->lastTestItem->next = item;
    item->prev = test->lastTestItem;
  } else {
    test->firstTestItem = item;
  }
  test->lastTestItem = item;
  testHandler(item);
}

static void realExit(int handle) {
  int code = (int)phoneGetHandleTag(handle);
  exit(code);
}

static void testExit(int code) {
  int delayHandle;
  if (test->postedExit) {
    return;
  }
  test->postedExit = 1;
  delayHandle = phoneCreateTimer(500, realExit);
  phoneSetHandleTag(delayHandle, (void *)0 + code);
}

static void testCheck(void) {
  testItem *loop = test->firstTestItem;
  int total = 0;
  int finished = 0;
  int succeed = 0;
  while (loop) {
    if (loop->finished) {
      ++finished;
    }
    if (loop->succeed) {
      ++succeed;
    }
    ++total;
    loop = loop->next;
  }
  if (total != finished) {
    return;
  }
  if (total != succeed) {
    phoneLog(PHONE_LOG_ERROR, TEST_TAG, TEST_TAG "Test Failed(%d/%d)", total - succeed,
      total);
    testExit(total - succeed);
    return;
  }
  phoneLog(PHONE_LOG_INFO, TEST_TAG, TEST_TAG "All Test Succeed(%d)", total);
  testExit(0);
}

void testSucceed(testItem *item) {
  item->finished = 1;
  item->succeed = 1;
  phoneLog(PHONE_LOG_INFO, TEST_TAG, TEST_TAG "[-] \"%s\" succeeded", item->testName);
  testCheck();
}

void testFail(testItem *item, const char *fmt, ...) {
  char errMsg[2048];
  va_list args;
  va_start(args, fmt);
  item->finished = 1;
  snprintf(errMsg, sizeof(errMsg), fmt, args);
  phoneLog(PHONE_LOG_ERROR, TEST_TAG, TEST_TAG "[x] \"%s\" failed", item->testName);
  va_end(args);
  testCheck();
}

static void testAll(void) {
  int i;
  memset(test, 0, sizeof(testContext));
  for (i = 0; i < sizeof(testList) / sizeof(testList[0]); ++i) {
    testAddItem(testList[i].testName, testList[i].testHandler);
  }
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
  testAll();
  return 0;
}
