


#ifndef OSC_DATA_H
#define OSC_DATA_H


#ifdef __cplusplus
extern "C" {
#endif

char* oscEncodeString(char* buf, uint32_t* remaining, const char* str);
char* oscEncodeInt32(char* buf, uint32_t* remaining, int i);
char* oscEncodeFloat32(char* buf, uint32_t* remaining, float f);
char* oscEncodeBlob(char* buf, uint32_t* remaining, const char* b, uint32_t len);

char* oscDecodeInt32(char* buf, uint32_t* remaining, int* value);
char* oscDecodeFloat32(char* buf, uint32_t* remaining, float* value);
char* oscDecodeString(char* buf, uint32_t* remaining, char** str);
char* oscDecodeBlob(char* buf, uint32_t* remaining, char** blob, uint32_t* len);

int oscPaddedStrlen(const char* str);

#ifdef __cplusplus
}
#endif

#endif // OSC_DATA_H


