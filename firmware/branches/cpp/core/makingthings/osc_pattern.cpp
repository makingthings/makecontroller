

#include "osc_pattern.h"


/*
  Adapted from OSC-pattern-match.c, by Matt Wright
  Adapted from oscpattern.c, by Matt Wright and Amar Chaudhury
*/
bool OscPattern::match(const char *  pattern, const char * test)
{
  // theWholePattern = pattern;
  
  if (pattern == 0 || pattern[0] == 0) 
  {
    return test[0] == 0;
  } 
  
  if (test[0] == 0)
  {
    if (pattern[0] == '*')
      return match(pattern+1,test);
    else
      return false;
  }

  switch (pattern[0]) 
  {
    case 0: 
      return test[0] == 0;
    case '?': 
      return match(pattern + 1, test + 1);
    case '*': 
      if (match(pattern+1, test)) 
        return true;
      else 
	      return match(pattern, test+1);
    case ']':
    case '}':
      // OSCWarning("Spurious %c in pattern \".../%s/...\"",pattern[0], theWholePattern);
      return false;
    case '[':
      return matchBrackets (pattern,test);
    case '{':
      return matchList (pattern,test);
    case '\\':  
      if (pattern[1] == 0) 
      	return test[0] == 0;
      else 
      {
        if (pattern[1] == test[0]) 
          return match(pattern+2,test+1);
        else 
          return false;
      }
    default:
      if (pattern[0] == test[0]) 
      	return match(pattern+1,test+1);
      else 
      	return false;
  }
}


// we know that pattern[0] == '[' and test[0] != 0
bool OscPattern::matchBrackets (const char *pattern, const char *test) 
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

  return match(p+1,test+1);
}

bool OscPattern::matchList (const char *pattern, const char *test) 
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
     if (match(restOfPattern, tp)) 
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
       return match(restOfPattern, tp);
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


