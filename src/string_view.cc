#include "string_view.hh"

string_view peal_head(string_view& sv, const char* ds) {
  const auto i1 = sv.find_first_not_of(ds);
  if (i1==string_view::npos) return sv = { };
  auto i2 = std::min(sv.find_first_of(ds,i1+1),sv.size());
  auto head = sv.substr(i1,i2-i1);
  sv.remove_prefix(i2);
  return head;
}
