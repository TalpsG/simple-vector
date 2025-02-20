#pragma once

#include "roaring/roaring.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class FilterIndex {
public:
  enum class Operation { EQUAL, NOT_EQUAL };

  FilterIndex();
  void addIntFieldFilter(const std::string &fieldname, int64_t value,
                         uint64_t id);
  void updateIntFieldFilter(const std::string &fieldname, int64_t *old_value,
                            int64_t new_value, uint64_t id);
  void getIntFieldFilterBitmap(const std::string &fieldname, Operation op,
                               int64_t value, roaring_bitmap_t *result_bitmap);

private:
  std::map<std::string, std::map<long, roaring_bitmap_t *>> intFieldFilter;
};
