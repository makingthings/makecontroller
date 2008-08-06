/*********************************************************************************

 Copyright 2006-2008 MakingThings

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

#include "types.h"
#include "osc.h"

static const char *theWholePattern;	/* Just for warning messages */

static bool MatchBrackets (const char *pattern, const char *test);
static bool MatchList (const char *pattern, const char *test);

bool Osc_PatternMatch(const char *  pattern, const char * test) 
{
  theWholePattern = pattern;
  
  if (pattern == 0 || pattern[0] == 0) 
  {
    return test[0] == 0;
  } 
  
  if (test[0] == 0)
  {
    if (pattern[0] == '*')
      return Osc_PatternMatch(pattern+1,test);
    else
      return false;
  }

  switch (pattern[0]) 
  {
    case 0: 
      return test[0] == 0;
    case '?': 
      return Osc_PatternMatch(pattern + 1, test + 1);
    case '*': 
      if (Osc_PatternMatch(pattern+1, test)) 
        return true;
      else 
	      return Osc_PatternMatch(pattern, test+1);
    case ']':
    case '}':
      // OSCWarning("Spurious %c in pattern \".../%s/...\"",pattern[0], theWholePattern);
      return false;
    case '[':
      return MatchBrackets (pattern,test);
    case '{':
      return MatchList (pattern,test);
    case '\\':  
      if (pattern[1] == 0) 
      	return test[0] == 0;
      else 
      {
        if (pattern[1] == test[0]) 
          return Osc_PatternMatch(pattern+2,test+1);
        else 
          return false;
      }
    default:
      if (pattern[0] == test[0]) 
      	return Osc_PatternMatch(pattern+1,test+1);
      else 
      	return false;
  }
}


/* we know that pattern[0] == '[' and test[0] != 0 */

static bool MatchBrackets (const char *pattern, const char *test) 
{
  bool result;
  bool negated = false;
  const char *p = pattern;

  if (pattern[1] == 0) 
  {
    // OSCWarning("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
    return false;
  }

  if (pattern[1] == '!') 
  {
    negated = true;
    p++;
  }

  while (*p != ']') 
  {
    if (*p == 0) 
    {
      //OSCWarning("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
      return false;
    }
    if (p[1] == '-' && p[2] != 0) 
    {
      if (test[0] >= p[0] && test[0] <= p[2]) 
      {
	      result = !negated;
	      goto advance;
      }
    }
    if (p[0] == test[0]) 
    {
      result = !negated;
      goto advance;
    }
    p++;
  }

  result = negated;

advance:

  if (!result)
    return false;

  while (*p != ']') 
  {
    if (*p == 0) 
    {
      //OSCWarning("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
      return false;
    }
    p++;
  }

  return Osc_PatternMatch(p+1,test+1);
}

static bool MatchList (const char *pattern, const char *test) 
{

 const char *restOfPattern, *tp = test;

 for(restOfPattern = pattern; *restOfPattern != '}'; restOfPattern++) 
 {
   if (*restOfPattern == 0) 
   {
     //OSCWarning("Unterminated { in pattern \".../%s/...\"", theWholePattern);
     return false;
   }
 }

 restOfPattern++; /* skip close curly brace */

 pattern++; /* skip open curly brace */

 while (1) 
 {  
   if (*pattern == ',') 
   {
     if (Osc_PatternMatch(restOfPattern, tp)) 
       return true;
     else 
     {
       tp = test;
       ++pattern;
     }
   } 
   else 
   {
     if (*pattern == '}')
       return Osc_PatternMatch(restOfPattern, tp);
     else 
     {
       if (*pattern == *tp) 
       {
         ++pattern;
         ++tp;
       } 
       else 
       {
         tp = test;
         while (*pattern != ',' && *pattern != '}') {
           pattern++;
       }
       if (*pattern == ',') 
         pattern++;
      }
     }
   }
 }
}

#endif // OSC



