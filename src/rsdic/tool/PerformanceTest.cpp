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
#include <cmath>
#include "../lib/RSDic.hpp"
#include "../lib/RSDicBuilder.hpp"

#include <time.h>
#include <sys/time.h>
#include <stdio.h>

double gettimeofday_sec()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + (double)tv.tv_usec*1e-6;
}


using namespace std;
using namespace rsdic;

void Test(uint64_t num, float ratio){
  RSDicBuilder bvb;
  for (uint64_t i = 0; i < num; ++i){
    float r = (float)rand() / RAND_MAX;
    if (r < ratio) bvb.PushBack(1);
    else bvb.PushBack(0);
  }

  RSDic bv;
  bvb.Build(bv);

  uint64_t query_num = 1000000;
  uint64_t dummy = 0;

  uint64_t m = bv.one_num();
  cout << ratio << "\t" << m * (1 / log(2) + log((float)num / m) / log(2.0)) / num << "\t";
  cout << (float)bv.GetUsageBytes() * 8 / num << "\t";

  double start = gettimeofday_sec();
  for (uint64_t i = 0; i < query_num; ++i){
    uint64_t pos = rand() % num;
    dummy += bv.GetBit(pos);
  }
  cout << query_num / (gettimeofday_sec() - start) << "\t";

  start = gettimeofday_sec();
  for (uint64_t i = 0; i < query_num; ++i){
    uint64_t pos = rand() % num;
    dummy += bv.Rank(pos, 1);
  }
  cout << query_num / (gettimeofday_sec() - start) << "\t";

  uint64_t one_num = bv.one_num();
  start = gettimeofday_sec();
  for (uint64_t i = 0; i < query_num; ++i){
    uint64_t pos = rand() % one_num;
    dummy += bv.Select(pos, 1);
  }
  cout << query_num / (gettimeofday_sec() - start) << endl;

  if (dummy == 777){
    // your very lucky
    cout << "";
  }
}

int main(int argc, char* argv[]){
  if (argc != 2){
    cerr << argv[0] << " num" << endl;
    return -1;
  }
  uint64_t num = atoll(argv[1]);
  cout << "ratio\tentropy\tbits per orig bit\tGetBit qps\tRank qps\tSelect qps" << endl;
  Test(num, 0.5);
  Test(num, 0.1);
  Test(num, 0.05);
  Test(num, 0.01);
  Test(num, 0.005);
  Test(num, 0.001);
  Test(num, 0.9);
  Test(num, 0.99);
  return 0;
}
