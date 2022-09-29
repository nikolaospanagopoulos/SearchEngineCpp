#pragma once
#include "gumbo.h"
#include <set>
#include <string>
#include <vector>
class Crawler {

public:
  // use static in case we want to use it somewhere else without instantiating
  static bool validLink(const std::string &href);
  std::set<std::string> *links{};
  std::string *crawlWebsite(const std::string website);
  std::vector<std::string> results{};
  std::vector<std::string> *getAllResults(std::string website);
  std::string createLink(const std::string &website, std::string href);

  Crawler();
  ~Crawler();

private:
  void search_for_links(GumboNode *node);
  std::string *response;
  void validateUrl(const std::string website) const;
};
