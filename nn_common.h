#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

typedef enum {
  Start,
  Stop, 
  Flush,
  Delay,
  Play,
  Volume,
  Parameters,
  Mute
} MsgType;

typedef enum {
  CharLen = 1,
  IntLen = 4,
  FloatLen = 4
} Sizes;

typedef struct {
  char *buf;
  size_t len;
  size_t writeIdx;
} Buffer;

Buffer *NewBuffer(size_t length);
void FreeBuffer(Buffer *buffer);
void WriteMsgType(Buffer *buffer, MsgType type);
void WriteInt(Buffer *buffer, int32_t value);
void WriteFloat(Buffer *buffer, float value);
void WriteShortArray(Buffer *buffer, short buf[], size_t length);
static const size_t StartBufLen = CharLen + IntLen;
void EncodeStart(Buffer *buffer, int sample_rate);
size_t PlayBufLen(int samples);
void EncodePlay(Buffer *buffer, short buf[], int samples);
static const size_t StopBufLen = CharLen;
void EncodeStop(Buffer *buffer);
static const size_t FlushBufLen = CharLen;
void EncodeFlush(Buffer *buffer);
static const size_t VolumeBufLen = CharLen + FloatLen;
void EncodeVolume(Buffer *buffer, double vol);
static const size_t MuteBufLen = CharLen + IntLen;
void EncodeMute(Buffer *buffer, int do_mute);
