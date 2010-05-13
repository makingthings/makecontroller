


#ifndef OSC_DATA_H
#define OSC_DATA_H


#ifdef __cplusplus
extern "C" {
#endif

char* oscEncodeString(char* buf, int* remaining, const char* str);
char* oscEncodeInt32(char* buf, int* remaining, int i);
char* oscEncodeFloat32(char* buf, int* remaining, float f);
char* oscEncodeBlob(char* buf, int* remaining, const char* b, int len);

char* oscDecodeInt32(char* buf, int* remaining, int* value);
char* oscDecodeFloat32(char* buf, int* remaining, float* value);
char* oscDecodeString(char* buf, int* remaining, char** str);
char* oscDecodeBlob(char* buf, int* remaining, char** blob, int* len);

int oscPaddedStrlen(const char* str);

#ifdef __cplusplus
}
#endif

#endif // OSC_DATA_H


