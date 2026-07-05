/*
 * search.cpp
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

char  *qfname    = NULL;
char  *infile    = NULL;
float similarity = 0.9;
uint64_t tag     = 0;

void usage();
void version();
void parse_parameters (int argc, char **argv);

int main(int argc, char **argv) {
  parse_parameters(argc, argv);

  try {
    ifstream ifs(infile, ios::in | ios::binary);
    if (!ifs) {
      cerr << "cannot open : " << infile << endl;
      return 1;
    }
    cerr << "Load:" << infile << endl;
    ifs.read((char*)(&tag), sizeof(tag));
    if (!ifs) {
      cerr << "error: corrupt or truncated index file: " << infile << endl;
      return 1;
    }
    // The leading uint64 is the index format tag: 11/12/13 = format v2
    // (mode 1/2/3). Old v1 indexes started with the raw mode 1/2/3.
    if      (tag == 11) {
      smbt::trie::SuccinctMultibitTreeTRIE mbt;
      mbt.load(ifs);
      ifs.close();
      mbt.search(qfname, similarity);
    }
    else if (tag == 12) {
      smbt::vla::SuccinctMultibitTreeVLA mbt;
      mbt.load(ifs);
      ifs.close();
      mbt.search(qfname, similarity);
    }
    else if (tag == 13) {
      smbt::mbt::MultibitTree mbt;
      mbt.load(ifs);
      ifs.close();
      mbt.search(qfname, similarity);
    }
    else if (tag == 1 || tag == 2 || tag == 3) {
      cerr << "error: " << infile
           << " is an old index format (v1) — rebuild the index with smbt-build." << endl;
      return 1;
    }
    else {
      cerr << "error: " << infile
           << " is not a smbt index file (unknown format tag " << tag << ")." << endl;
      return 1;
    }
  }
  catch (const std::exception &e) {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}

void version() {
  std::cerr << "Succinct Multibit Tree version 0.1.0" << std::endl
            << "Written by Yasuo Tabei" << std::endl << std::endl;
}

void usage(){
  std::cerr << std::endl
       << "Usage: smbt-search [OPTION]... INDEXFILE DATABASEFILE" << std::endl << std::endl
       << "       where [OPTION]...  is a list of zero or more optional arguments" << std::endl
       << "             INDEXFILE     is the name of an indexfile" << std::endl 
       << "             DATABASEFILE  is the name of a graph database" << std::endl << std::endl
       << "Additional arguments (input and output files may be specified):" << std::endl
       << "       -similarity [value]: threshold of Jaccard-Tanimoto similarity" << std::endl
       << "       (default: " << similarity << ")" << std::endl
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
      else if (!strcmp (argv[argno], "-similarity")) {
	if (argno == argc - 1) { std::cerr << "Must specify similarity after -similarity" << std::endl; exit(1); }
	similarity = atof(argv[++argno]);
	if (similarity < 0.f || 1.f < similarity) {
	  cerr << "The threshold value must be 0 <= similarity <= 1." << endl;
	  exit(1);
	}
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

  infile = argv[argno];
  qfname = argv[argno + 1];

}
