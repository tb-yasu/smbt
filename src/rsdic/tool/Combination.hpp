#ifndef COMBINATION_HPP_
#define COMBINATION_HPP_

#include <vector>
#include <stdint.h>

class Combination{
public:
  Combination(uint64_t size) : size_(size){
    nCk_.resize(size_+1);
    for (size_t i = 0; i < nCk_.size(); ++i){
      nCk_[i].resize(i+1);
    }
    for (uint64_t i = 0; i <= size; ++i){
      nCk_[i][0] = 1;
      nCk_[i][i] = 1;
    }
    for (uint64_t i = 1; i <= size; ++i){
      for (uint64_t j = 1; j < i; ++j){
        nCk_[i][j] = nCk_[i-1][j-1] + nCk_[i-1][j];
      }
    }
  }

  uint64_t Choose(uint64_t n, uint64_t k) const{
    return nCk_[n][k];
  }

private:
  std::vector<std::vector<uint64_t> > nCk_;
  uint64_t size_;

};

#endif // COMBINATION_HPP_
