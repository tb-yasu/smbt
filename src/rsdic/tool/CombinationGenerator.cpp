/* 
 *  Copyright (c) 2012 Daisuke Okanohara
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *   1. Redistributions of source code must retain the above Copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above Copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 */

#include <iostream>
#include "Combination.hpp"

using namespace std;

int main(int argc, char* argv[]){
  if (argc != 2){
    cerr << argv[0] << " n" << endl;
    return -1;
  }
  uint64_t n = atoi(argv[1]);
  Combination cb(n);
  cout << "{ "<< endl;
  for (uint64_t i = 0; i < n; ++i){
    if (i > 0) cout << "," << endl;
    cout << "{ ";
    for (uint64_t j = 0; j <= i; ++j){
      if (j > 0) cout << ", ";
      cout << cb.Choose(j, i) << "LLU";
    }
    for (uint64_t j = i+1; j < n; ++j){
      cout << ", 0LLU";
    }
    cout << "}";
  }
  cout << "};" << endl;
  return 0;
}
