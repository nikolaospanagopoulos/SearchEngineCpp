#pragma once
#include "gumbo.h"
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>
class Crawler {

public:
  // use static in case we want to use it somewhere else without instantiating
  static bool validLink(const std::string &href);
  std::string *crawlWebsite(const std::string website);
  std::queue<std::string> *links;
  std::vector<std::string> results{};
  void getAllResults(std::string website, int depth = 1);
  std::string createLink(const std::string &website, std::string href);

  Crawler();
  ~Crawler();

private:
  bool existsInSet(const std::string &word) const;
  void saveRawLinks(std::string &website, GumboOutput *output);
  std::unordered_set<std::string> *alreadyCrawled;
  void makeLinkProper(std::string &link, const std::string &url);
  void search_for_links(GumboNode *node, const std::string &url);
  std::string *response;
  void validateUrl(const std::string website) const;
};
