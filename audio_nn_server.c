/*
 * alsa proxy server via nn. This file is part of Shairport.
 * Copyright (c) Aaron Zinman 2016
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <nanomsg/pubsub.h>
#include <nanomsg/nn.h>
#include "common.h"
#include "audio.h"
#include "nn_common.h"

static int nnSocket = -1;

static int init(int argc, char **argv) {
  debug(1, "nn server init");
  const char *str;
  int value;
  
  config.audio_backend_buffer_desired_length = 44100; // one second.
  config.audio_backend_latency_offset = 0;
  config.port = 6557;

  if (config.cfg != NULL) {
    const char *str;

    /* Get the desired buffer size setting. */
    if (config_lookup_int(config.cfg, "nn_server.audio_backend_buffer_desired_length", &value)) {
      if ((value < 0) || (value > 132300))
        die("Invalid nn_server audio backend buffer desired length \"%d\". It should be between 0 and 132300, default is 44100",
            value);
      else
        config.audio_backend_buffer_desired_length = value;
    }

    /* Get the latency offset. */
    if (config_lookup_int(config.cfg, "nn_server.audio_backend_latency_offset", &value)) {
      if ((value < -66150) || (value > 66150))
        die("Invalid nn_server audio backend buffer latency offset \"%d\". It should be between -66150 and +66150, default is 0",
            value);
      else
        config.audio_backend_latency_offset = value;
    }

    /* Get the port. */
    if (config_lookup_int(config.cfg, "nn_server.port", &value)) {
      if ((value < 1) || (value > 66150))
        die("Invalid nn_server port \"%d\". Default is 6557", value);
      else
        config.port = value;
    }
  }
  
  // here, create the server
  nnSocket = nn_socket(AF_SP, NN_PUB);
  if (nnSocket < 0) die("Could not create NN server");
  char *url;
  if (asprintf(&url, "tcp://*:%d", config.port) < 0) {
    die("Couldn't allocate basic string");
  }
  debug(1, "NN socket binding %s", url);
  if (nn_bind(nnSocket, url) < 0) die("Could not bind socket for NN server");
  debug(1, "NN Socket is bound on port \"%d\"", config.port);
  free(url);

  return 0;
}

static void deinit(void) {
  if (nnSocket < 0) {
    return;
  }
  nn_shutdown(nnSocket, 0);
}

static void sendBuffer(Buffer *buffer) {
  int bytes = nn_send(nnSocket, buffer->buf, buffer->len, 0);
  assert(bytes == buffer->len);
}

static void start(int sample_rate) {
  // // this will leave fd as -1 if a reader hasn't been attached
  // fd = open(pipename, O_WRONLY | O_NONBLOCK);
  if (sample_rate != 44100) {
    die("Wtf sample rate is this");
  }
  Buffer *buffer = NewBuffer(StartBufLen);
  EncodeStart(buffer, sample_rate);
  sendBuffer(buffer);
  FreeBuffer(buffer);
}

static void play(short buf[], int samples) {
  // if the file is not open, try to open it.
  // if (fd == -1) {
  //    fd = open(pipename, O_WRONLY | O_NONBLOCK); 
  // }
  // // if it's got a reader, write to it.
  // if (fd != -1) {
  //   int ignore = non_blocking_write(fd, buf, samples * 4);
  // }

  Buffer *buffer = NewBuffer(PlayBufLen(samples));
  EncodePlay(buffer, buf, samples);
  sendBuffer(buffer);
  FreeBuffer(buffer);
}

static void stop(void) {
// Don't close the pipe just because a play session has stopped.
//  if (fd > 0)
//    close(fd);
  Buffer *buffer = NewBuffer(StopBufLen);
  EncodeStop(buffer);
  sendBuffer(buffer);
  FreeBuffer(buffer);
}

static void flush() {
  Buffer *buffer = NewBuffer(FlushBufLen);
  EncodeFlush(buffer);
  sendBuffer(buffer);
  FreeBuffer(buffer);
}

static void volume(double vol) {
  Buffer *buffer = NewBuffer(VolumeBufLen);
  EncodeVolume(buffer, vol);
  sendBuffer(buffer);
  FreeBuffer(buffer);
}

static void mute(int do_mute) {
  Buffer *buffer = NewBuffer(MuteBufLen);
  EncodeMute(buffer, do_mute);
  sendBuffer(buffer);
  FreeBuffer(buffer);
}
  
static void help(void) {
  printf("nn_server takes no arguments.\n");
}

audio_output audio_nn_server = {.name = "nn_server",
                               .help = &help,
                               .init = &init,
                               .deinit = &deinit,
                               .start = &start,
                               .stop = &stop,
                               .flush = &flush,
                               .delay = NULL,
                               .play = &play,
                               .volume = &volume,
                               .parameters = NULL,
                               .mute = &mute};
