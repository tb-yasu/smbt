/*
 * build.cpp
 * Copyright (c) 2013 Yasuo Tabei All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE and * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include "./smbt_trie/succinct_multibit_tree_trie.hpp"
#include "./smbt_vla/succinct_multibit_tree_vla.hpp"
#include "./mbt/multibit_tree.hpp"

using namespace std;

char    *fname  = NULL;
char    *oname  = NULL;
uint64_t minsup = 10;
uint64_t mode   = 1;

void usage();
void version();
void parse_parameters (int argc, char **argv);

int main(int argc, char **argv) {
  parse_parameters(argc, argv);

  // Validate the mode, and build the index in memory, *before* touching
  // the output file at all: this way neither an invalid -mode nor a
  // build() failure (e.g. unreadable input, which build() itself already
  // exit(1)s on) ever leaves a truncated/garbage index file behind.
  if (mode != 1 && mode != 2 && mode != 3) {
    std::cerr << "error : mode should be 1 or 2 or 3." << std::endl;
    return 1;
  }

  // Index format v2: the leading uint64 is a format tag (mode + 10), so
  // smbt-search can tell v2 indexes (11/12/13) apart from old v1 indexes
  // (1/2/3) and reject the latter with a clear error.
  const uint64_t tag = mode + 10;

  if      (mode == 1) {
    smbt::trie::SuccinctMultibitTreeTRIE mbt;
    mbt.build(fname, minsup);
    ofstream ofs(oname, ios::out | ios::binary | ios::trunc);
    if (!ofs) {
      cerr << "cannot open: " << oname << endl;
      exit(1);
    }
    ofs.write((const char*)(&tag), sizeof(tag));
    mbt.save(ofs);
    ofs.close();
    if (!ofs) {
      cerr << "error writing index file: " << oname << endl;
      exit(1);
    }
  }
  else if (mode == 2) {
    smbt::vla::SuccinctMultibitTreeVLA mbt;
    mbt.build(fname, minsup);
    ofstream ofs(oname, ios::out | ios::binary | ios::trunc);
    if (!ofs) {
      cerr << "cannot open: " << oname << endl;
      exit(1);
    }
    ofs.write((const char*)(&tag), sizeof(tag));
    mbt.save(ofs);
    ofs.close();
    if (!ofs) {
      cerr << "error writing index file: " << oname << endl;
      exit(1);
    }
  }
  else {
    smbt::mbt::MultibitTree mbt;
    mbt.build(fname, minsup);
    ofstream ofs(oname, ios::out | ios::binary | ios::trunc);
    if (!ofs) {
      cerr << "cannot open: " << oname << endl;
      exit(1);
    }
    ofs.write((const char*)(&tag), sizeof(tag));
    mbt.save(ofs);
    ofs.close();
    if (!ofs) {
      cerr << "error writing index file: " << oname << endl;
      exit(1);
    }
  }

  return 0;
}

void version() {
  std::cerr << "Succinct Multibit Tree version 0.0.1" << std::endl
            << "Written by Yasuo Tabei" << std::endl << std::endl;
}

void usage(){
  std::cerr << std::endl
       << "Usage: smbt-build [OPTION]... DATABASEFILE INDEXFILE" << std::endl << std::endl
       << "       where [OPTION]...  is a list of zero or more optional arguments" << std::endl
       << "             DATABASEFILE  is the name of a graph database" << std::endl
       << "             INDEXFILE     is the name of an indexfile" << std::endl << std::endl
       << "Additional arguments (input and output files may be specified):" << std::endl
       << "       -mode [1|2|3]" << std::endl
       << "       1:Succinct Multibit Tree + Succinct TRIE 2:Succinct Multibit Tree + Variable Length Array 3:Multibit Tree" << std::endl
       << "       (mode: " << mode << ")" << std::endl
       << "       -minsup [value]: minimum number of data at each leaf" << std::endl
       << "       (default: " << minsup << ")" << std::endl
       << std::endl;
  exit(1);
}

void parse_parameters (int argc, char **argv){
  if (argc == 1) usage();
  int argno;
  for (argno = 1; argno < argc; argno++){
    if (argv[argno][0] == '-'){
      if      (!strcmp (argv[argno], "-version")){
	version();
      }
      else if (!strcmp (argv[argno], "-mode")) {
	if (argno == argc - 1) { std::cerr << "Must specify mode after -mode" << std::endl; exit(1); }
	mode = atoi(argv[++argno]);
      }
      else if (!strcmp (argv[argno], "-minsup")) {
	if (argno == argc - 1) { std::cerr << "Must specify minsup after -minsup" << std::endl; exit(1); }
	minsup = atoi(argv[++argno]);
      }
      else {
	usage();
      }
    } else {
      break;
    }
  }
  if (argno + 1 >= argc)
    usage();

  fname     = argv[argno];
  oname     = argv[argno + 1];
}
