#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "nn_common.h"
#include <nanomsg/pubsub.h>
#include <nanomsg/nn.h>

Buffer *NewBuffer(size_t length) {
  void *buf = nn_allocmsg(length, 0);
  if (buf != NULL) {
    Buffer *buffer = malloc(sizeof(Buffer));
    buffer->buf = (char *)buf;
    buffer->len = length;
    buffer->writeIdx = 0;
    return buffer;
  }
  return NULL;
}

void FreeBuffer(Buffer *buffer) {
  assert(nn_freemsg((void *)buffer->buf) == 0);
  free(buffer);
}

void WriteMsgType(Buffer *buffer, MsgType type) {
  char typeChar = (char)type;
  buffer->buf[buffer->writeIdx] = typeChar;
  buffer->writeIdx += CharLen;
}

void WriteInt(Buffer *buffer, int32_t value) {
  uint32_t networkInt = htonl(value);
  memcpy(buffer->buf[buffer->writeIdx], &networkInt, IntLen);
  buffer->writeIdx += IntLen;
}

void WriteFloat(Buffer *buffer, float value) {
  union v {
    float f;
    uint32_t i;
  };
  union v val;
  val.f = value;
  val.i = htonl(val.i);
  memcpy(buffer->buf[buffer->writeIdx], &val.i, FloatLen);
  buffer->writeIdx += FloatLen;
}

void WriteShortArray(Buffer *buffer, short buf[], size_t length) {
  size_t memLength = sizeof(short) * length;
  memcpy(buffer->buf[buffer->writeIdx], buf, memLength);
  buffer->writeIdx += memLength;
}

void EncodeStart(Buffer *buffer, int sample_rate) {
  WriteMsgType(buffer, Start);
  WriteInt(buffer, sample_rate);
}

size_t PlayBufLen(int samples) {
  return CharLen + sizeof(short) * samples;
}

void EncodePlay(Buffer *buffer, short buf[], int samples) {
  WriteMsgType(buffer, Play);
  WriteShortArray(buffer, buf, samples);
}

void EncodeStop(Buffer *buffer) {
  WriteMsgType(buffer, Stop);
}

void EncodeFlush(Buffer *buffer) {
  WriteMsgType(buffer, Flush);
}

void EncodeVolume(Buffer *buffer, double vol) {
  WriteMsgType(buffer, Volume);
  WriteFloat(buffer, vol);
}

void EncodeMute(Buffer *buffer, int do_mute) {
  WriteMsgType(buffer, Mute);
  WriteInt(buffer, do_mute);
}
