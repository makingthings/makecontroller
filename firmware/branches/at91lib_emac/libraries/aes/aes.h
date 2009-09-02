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

#ifndef AES_H
#define AES_H

/**
  Encrypt and decrypt data using AES (Advanced Encryption Standard).

  AES is a block cipher adopted for use by the NSA (US government agency).  According to Wikipedia,
  "This marks the first time the public has had access to a cipher approved by the NSA for
  top secret information."  Pretty cool, right?  This is a good way to send info privately
  from your Make Controller over the network, to a web server for example.

  AES requires that both sides of communication have access to the same key.  So both your Make 
  Controller, and whatever other device it's communicating with need to know about the same 
  password so they can decrypt data that has been encrypted by the other.

  There are several different flavors of AES.  There are two main ways in which AES libraries
  differ:
  - the way they chain blocks
  - the way they pad data

  This library uses ECB (Electronic Code Book) chaining, and pads data with a character 
  that corresponds to the number of bytes needed to pad to 16.  Check the Wikipedia article
  for an explanation - http://en.wikipedia.org/wiki/Advanced_Encryption_Standard

  The lookup tables used in this library will use somewhere between 8 and 13 kB of program space, 
  depending on the compiler optimization you use.  Memory usage is pretty minimal.

  Code is used and adapted from Philip J. Erdelsky - see http://www.efgh.com/software/rijndael.htm
  for the original.
*/
class Aes
{
public:
  static int encrypt(unsigned char* output, int outlen, unsigned char* input, int inlen, unsigned char* key);
  static int decrypt(unsigned char* output, int outlen, unsigned char* input, int inlen, unsigned char* password);

private:
  static int setupEncrypt( unsigned long* rk, const unsigned char* key, int keybits);
  static int setupDecrypt( unsigned long *rk, const unsigned char *key, int keybits);
  static void doEncrypt( const unsigned long *rk, int nrounds, 
    const unsigned char plaintext[16], unsigned char ciphertext[16]);
  static void doDecrypt( const unsigned long *rk, int nrounds,
    const unsigned char ciphertext[16], unsigned char plaintext[16]);
};

#endif // AES_H
