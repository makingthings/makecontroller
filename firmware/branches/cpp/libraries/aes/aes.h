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
