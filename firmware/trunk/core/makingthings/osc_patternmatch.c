/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/*
    osc_patternmatch.c

    Adapted from OSC-pattern-match.c, by Matt Wright 
    Adapted from oscpattern.c, by Matt Wright and Amar Chaudhury
 */

#include "config.h"
#ifdef OSC

#include "osc_patternmatch.h"
#include <stdio.h>
#include <ctype.h>

static bool oscMatchBrackets (const char *pattern, const char *test);
static bool oscMatchList (const char *pattern, const char *test);
/*
static bool oscIsSpecialChar(char c)
{
  switch(c) {
    case 0:
    case '?':
    case '*':
    case '[':
    case '{':
    case ']':
    case '}':
    case '\\':
      return true;
    default:
      return false;
  }
}
*/

bool oscPatternMatch(const char *  pattern, const char * test)
{
  if (pattern == 0 || pattern[0] == 0)
    return test[0] == 0;
  
  if (test[0] == 0) {
    if (pattern[0] == '*')
      return oscPatternMatch(pattern+1,test);
    else
      return false;
  }

  switch (pattern[0]) {
    case 0: 
      return test[0] == 0;
    case '?': 
      return oscPatternMatch(pattern + 1, test + 1);
    case '*': 
      if (oscPatternMatch(pattern + 1, test))
        return true;
      else 
	      return oscPatternMatch(pattern, test+1);
    case ']':
    case '}':
      return false; // spurious closing bracket
    case '[':
      return oscMatchBrackets(pattern,test);
    case '{':
      return oscMatchList(pattern,test);
    case '\\':  
      if (pattern[1] == 0) 
      	return test[0] == 0;
      else {
        if (pattern[1] == test[0]) 
          return oscPatternMatch(pattern+2,test+1);
        else 
          return false;
      }
    default:
      // TODO - this recurses for *each* character...should
      // iterate unless it's a special osc character
      if (pattern[0] == test[0])
      	return oscPatternMatch(pattern+1,test+1);
      else
      	return false;
  }
}


/* we know that pattern[0] == '[' and test[0] != 0 */
static bool oscMatchBrackets (const char *pattern, const char *test)
{
  bool result;
  bool negated = false;
  const char *p = pattern;

  if (pattern[1] == 0)
    return false; // unterminated [ in pattern

  if (pattern[1] == '!')  {
    negated = true;
    p++;
  }

  while (*p != ']') {
    if (*p == 0)
      return false; // unterminated [ in pattern
    if (p[1] == '-' && p[2] != 0)  {
      if (test[0] >= p[0] && test[0] <= p[2])  {
	      result = !negated;
	      goto advance;
      }
    }
    if (p[0] == test[0]) {
      result = !negated;
      goto advance;
    }
    p++;
  }

  result = negated;

advance:

  if (!result)
    return false;

  while (*p != ']') {
    if (*p == 0)
      return false; // unterminated [ in pattern
    p++;
  }

  return oscPatternMatch(p + 1, test + 1);
}

static bool oscMatchList (const char *pattern, const char *test)
{
  const char *restOfPattern, *tp = test;

  for (restOfPattern = pattern; *restOfPattern != '}'; restOfPattern++) {
    if (*restOfPattern == 0)
      return false; // unterminated { in pattern
  }

  restOfPattern++; /* skip close curly brace */
  pattern++; /* skip open curly brace */

  while (1) {
    if (*pattern == ',')  {
      if (oscPatternMatch(restOfPattern, tp))
        return true;
      else  {
        tp = test;
        ++pattern;
      }
    }
    else  {
      if (*pattern == '}')
       return oscPatternMatch(restOfPattern, tp);
      else  {
        if (*pattern == *tp)  {
          ++pattern;
          ++tp;
        }
        else {
          tp = test;
          while (*pattern != ',' && *pattern != '}')
           pattern++;
          if (*pattern == ',')
            pattern++;
        }
      }
    }
  }
}

/*
 * Match a range element in an address pattern, and populate an
 * OscRange object accordingly - the range object can either represent
 * a single value in the simplest case, or in more complex scenarios
 * a bit mask of values.
 */
bool oscNumberMatch(const char* pattern, int offset, int count, OscRange* r)
{
  r->state = EXHAUSTED;
  int n = 0;
  int digits = 0;
  while (isdigit((int)*pattern)) {
    digits++;
    n = n * 10 + (*pattern++ - '0');
  }

  if (n >= count)
    return false;

  switch (*pattern) {
    case '*':
    case '?':
    case '[':
    case '{': {
      int i;
      r->value = 0;
      char s[5];
      for (i = count - 1; i >= 0; i--) {
        r->value <<= 1;
        siprintf(s, "%d", i);
        if (oscPatternMatch(pattern, s))
          r->value |= 1;
      }
      r->index = offset;
      r->state = BITS;
      return true;
    }
    default:
      if (digits == 0) {
        r->state = EXHAUSTED;
        return false;
      }
      else {
        r->state = SINGLENUM;
        r->value = n;
        return true;
      }
  }
}

bool oscRangeHasNext(OscRange* r) {
  switch (r->state) {
    case SINGLENUM: return true;
    case BITS:      return (r->value > 0);
    default:        return false;
  }
}

int oscRangeNext(OscRange* r) {
  switch (r->state) {
    case SINGLENUM:
      r->state = EXHAUSTED;
      return r->value;
    case BITS:
      while (r->value > 0 && !(r->value & 0x01)) {
        r->index++;
        r->value >>= 1;
      }
      r->value >>= 1;
      return r->index++;
    default:
      return -1;
  }
}

#endif // OSC
