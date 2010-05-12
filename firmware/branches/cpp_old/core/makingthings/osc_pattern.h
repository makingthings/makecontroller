

#ifndef OSC_PATTERN_H
#define OSC_PATTERN_H

class OscPattern
{
public:
  static bool match(const char *  pattern, const char * test);
  static bool matchList(const char *pattern, const char *test);
  static bool matchBrackets(const char *pattern, const char *test);
};


#endif // OSC_PATTERN_H
