#if !defined(HANDMADE_H)

/* NOTE(bjorn):

   :HANDMADE_INTERNAL:
   0 - Build for public release. 
   1 - Build for developer only.

   :HANDMADE_SLOW:
   0 - No slow code allowed!
   1 - Slow code welcome.
 */

#include <stdint.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;
typedef u32 b32;

// TODO(bjorn): Implement sine on my own.
//#include <math.h>
#define Pi32 3.14159265359f

// TODO(bjorn): This can be improved by dividing the exponent instead.
//              See also the Quake inverse-square hack.
// TODO(bjorn): Experiment with how many iterations that are needed for float preciscion.
  inline f32
SquareRoot(f32 Number, f32 Epsilon)
{
  // NOTE(bjorn): Tangent line (linearization of f(x)) is y = f'(x0)dx + f(x0).
  //              Let the function be f(x) = x^2 - Number, where Number is from sqrt(Number).
  //              y = 0 solved for x gives x = x0 - f(x0)/f'(x0). f' = 2x.
  //              This combined gives x = x0 - (x0^2 - Number)/2x0 which simplifies to
  //              x = 1/2 * (x0 + Number/x0). Given an estimate x0 a better root x can be 
  //              found exluding some caveats like x0 = 0 etc.
  f32 Root = Number / 2.0f;
  f32 Error = Root * Root - Number; 
  f32 LastError = 0.0f;
  while((Error > Epsilon || -Epsilon > Error) && (LastError != Error))
  {
    LastError = Error;
    Root = (Root + Number/Root)/2.0f;
    Error = Root * Root - Number; 
  }
  return Root;
}

#define internal_function static 
#define local_persist static 
#define global_variable static 

#if HANDMADE_SLOW
#define Assert(expression) if(!(expression)){ *(s32 *)0 = 0; }
#else
#define Assert(expression)
#endif

#define Kilobytes(number) (number * 1024LL)
#define Megabytes(number) (Kilobytes(number) * 1024LL)
#define Gigabytes(number) (Megabytes(number) * 1024LL)
#define Terabytes(number) (Gigabytes(number) * 1024LL)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

// TODO(bjorn): swap, min, max...   macros???

//
// NOTE(bjorn): Stuff the platform provides to the server.
//
#define PLATFORM_LOG_STRING(name) void name(char *Message)
typedef PLATFORM_LOG_STRING(platform_log_string);

struct read_file_result
{
  s32 ContentSize;
  void *Content;
};
#define PLATFORM_READ_ENTIRE_FILE(name) read_file_result name(char *FileName)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(char *FileName, \
                                                  u32 FileSize, \
                                                  void *FileMemory)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void name(void *FileMemory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

//
// NOTE(bjorn): Stuff the server provides to the platform.
//
#define HANDLE_INCOMING_MESSAGE(name) void name()
typedef HANDLE_INCOMING_MESSAGE(handle_incoming_message);
HANDLE_INCOMING_MESSAGE(HandleIncomingMessageStub)
{
  return;
}

#define HANDMADE_H
#endif
