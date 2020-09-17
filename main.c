// Copyright (c) 2020 CUJO LLC. All rights reserved.

#include <lua.h>

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#define RESULT_FAIL 0

#define LIBLUA_ENOUGH 503.0
#define LIBLUA_TOO_MUCH 504.0
/* According to documentation at:
   https://www.openssl.org/docs/man1.0.2/man3/OPENSSL_VERSION_NUMBER.html

   The version nybbles are:
   MNNFFPPS: major minor fix patch status
*/
#define LIBSSL_MIN_OPENSSL_VERSION_NUM 0x100020bfUL
#define LIBSSL_MIN_SSLEAY 0x100020bfL

static int testno = 0;
static int numfail = 0;

/* Print out a running test number, test name and result.
   Non-zero result indicates success. */
static void report_test_result(const char *test_name, int result) {
  testno++;
  printf("%sok %d %s\n", result ? "" : "not ", testno, test_name);
  if (!result) {
    numfail++;
  }
}

/* Perform checks for liblua, and its version.
   Returns zero, if liblua can not be used for further tests. */
static void agent_readiness_test_liblua_checks(void) {
  int result = RESULT_FAIL;
  void *handle_liblua = NULL;
  const lua_Number * (*agent_readiness_test_liblua_version)(lua_State *) = NULL;
  const lua_Number *version = NULL;

  /* liblua is loadable. */
  handle_liblua = dlopen("liblua.so", RTLD_LAZY);
  result = !!handle_liblua;
  report_test_result("liblua.so", result);

  /* lua_version function exists. */
  if (result) {
    agent_readiness_test_liblua_version =
      (const lua_Number * (*) (lua_State *)) dlsym(handle_liblua,
        "lua_version");
    result = !!agent_readiness_test_liblua_version;
  }
  report_test_result("lua_version", result);

  /* lua_version returns a supported version number. */
  if (result) {
    version = lua_version(NULL);
    printf("# DEBUG lua_version = %f\n", *version);
    result = (*version >= LIBLUA_ENOUGH && *version < LIBLUA_TOO_MUCH);
  }
  report_test_result("Lua interpreter version 5.3", result);
}

#define LWS_EXACT_VERSION "4.0.16 "
#define LWS_EXACT_VERSION_LENGTH 7

/* Perform checks for libwebsockets, and its version.
   Returns zero, if libwebsockets can not be used for further tests. */
static void agent_readiness_test_libwebsockets_checks(void) {
  int result = RESULT_FAIL;
  void *handle_libwebsockets = NULL;
  const char * (*agent_readiness_test_lws_get_library_version)(void) = NULL;
  const char *version = NULL;

  /* libwebsockets is loadable. */
  handle_libwebsockets = dlopen("libwebsockets.so", RTLD_LAZY);
  result = !!handle_libwebsockets;
  report_test_result("libwebsockets.so", result);

  /* lws_get_library_version exists. */
  if (result) {
    agent_readiness_test_lws_get_library_version = (const char * (*) (void)) dlsym(
      handle_libwebsockets, "lws_get_library_version");
    result = !!agent_readiness_test_lws_get_library_version;
  }
  report_test_result("lws_get_library_version", result);

  /* lws_get_library_version returns a supported version string. */
  if (result) {
    version = agent_readiness_test_lws_get_library_version();
    printf("# DEBUG lws_get_library_version = '%s'\n", version);
    if (strlen(version) >= LWS_EXACT_VERSION_LENGTH) {
      result = !strncmp(version, LWS_EXACT_VERSION, LWS_EXACT_VERSION_LENGTH);
    } else {
      result = RESULT_FAIL;
    }
  }
  report_test_result("libwebsockets version 4.0.16", result);
}

#define RESULT_OPENSSL_VERSION_NUM 1
#define RESULT_SSLEAY 2

static void agent_readiness_test_libssl_checks(void) {
  int result = RESULT_FAIL;
  int result_dlsym = RESULT_FAIL;
  int result_version = RESULT_FAIL;
  void *handle_libssl = NULL;
  unsigned long (*agent_readiness_test_OpenSSL_version_num)(void) = NULL;
  long (*agent_readiness_test_SSLeay)(void) = NULL;
  unsigned long openssl_version = 0;
  long ssleay_version = 0;

  handle_libssl = dlopen("libssl.so.1.0.0", RTLD_LAZY);
  result = !!handle_libssl;
  report_test_result("libssl.so.1.0.0", result);

  result_dlsym = RESULT_FAIL;
  result_version = RESULT_FAIL;
  if (result) {
    /* Either OpenSSL_version_num or SSLeay is available.
       If both are, prefer the former. */
    agent_readiness_test_OpenSSL_version_num = (unsigned long (*) (void)) dlsym(
      handle_libssl, "OpenSSL_version_num");
    if (agent_readiness_test_OpenSSL_version_num) {
      result_dlsym = RESULT_OPENSSL_VERSION_NUM;
      openssl_version = agent_readiness_test_OpenSSL_version_num();
      printf("# DEBUG OpenSSL_version_num() = 0x%lx\n", openssl_version);
      if (openssl_version >= LIBSSL_MIN_OPENSSL_VERSION_NUM) {
        result_version = result_dlsym;
      }
    }
    agent_readiness_test_SSLeay = (long (*) (void)) dlsym(handle_libssl,
      "SSLeay");
    if (agent_readiness_test_SSLeay) {
      result_dlsym = RESULT_SSLEAY;
      ssleay_version = agent_readiness_test_SSLeay();
      printf("# DEBUG SSLeay() = 0x%lx\n", ssleay_version);
      if (ssleay_version >= LIBSSL_MIN_SSLEAY) {
        result_version = result_dlsym;
      }
    }
  }
  report_test_result("OpenSSL_version_num || SSLeay", result_dlsym);
  report_test_result("OpenSSL version >= 1.0.2b", result_version);
}
      

int main(void) {
  agent_readiness_test_liblua_checks();
  agent_readiness_test_libwebsockets_checks();
  agent_readiness_test_libssl_checks();

  return numfail;
}
