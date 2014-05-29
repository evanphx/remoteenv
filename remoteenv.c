#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "cdecode.h"
#include "jsmn.h"

typedef char* (*getenv_type)(const char* name);

char* getenv(const char* name) {
  static getenv_type orig_getenv = 0;

  if(!orig_getenv) {
    orig_getenv = (getenv_type)dlsym(RTLD_NEXT,"getenv");
  }

  char* val = orig_getenv(name);

  // printf("getenv(\"%s\") = \"%s\"\n", name, val);

  if(val) return val;

  int fd = socket(AF_INET, SOCK_STREAM,0);

  struct sockaddr_in consul;
  struct in_addr ip;

  inet_pton(AF_INET, "127.0.0.1", &ip);
  bcopy(&ip, &consul.sin_addr.s_addr, sizeof(ip));

  consul.sin_family = AF_INET;
  consul.sin_port = htons(8500);

  connect(fd, (const struct sockaddr*)&consul, sizeof(consul));

  char buf[1024];

  FILE* out = fdopen(fd, "r+");

  fprintf(out, "GET /v1/kv/env.%s HTTP/1.0\r\n\r\n", name);

  do {
    fgets(buf, 1023, out);
  } while(strlen(buf) != 2);

  fgets(buf, 1024, out);

  // printf("fin: %s\n", buf);

  jsmn_parser parser;
  jsmntok_t tokens[20];

  jsmn_init(&parser);
  jsmn_parse(&parser, buf, strlen(buf), tokens, 20);

  if(tokens[0].type != JSMN_ARRAY) return val;
  if(tokens[1].type != JSMN_OBJECT) return val;

  for(int i = 2; i < 17; i += 2) {
    if(tokens[i].type != JSMN_STRING) continue;

    int start = tokens[i].start;
    int len = tokens[i].end - start;

    if(len != 5) continue;

    if(strncmp(buf+start, "Value", len) != 0) continue;

    start = tokens[i+1].start;
    len = tokens[i+1].end - start;

    base64_decodestate bstate;
    base64_init_decodestate(&bstate);

    char out[1024];

    int sz = base64_decode_block(buf+start, len, out, &bstate);

    return strndup(out, sz);
  }

  return val;
}
