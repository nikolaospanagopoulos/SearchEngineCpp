#include "Crawler.hpp"

#include "CustomException.hpp"
#include <cstddef>
#include <curl/curl.h>
#include <regex>

Crawler::Crawler() {
  response = new std::string{};

  links = new std::set<std::string>{};
}

Crawler::~Crawler() {
  delete response;
  delete links;
  response = nullptr;
  links = nullptr;
}

bool Crawler::validLink(const std::string &href) {

  if (href.find("javascript:", 0, 11) != std::string::npos) {

    return false;
  }

  if (href.find("#", 0, 1) != std::string::npos) {

    return false;
  }
  return true;
}

void Crawler::validateUrl(const std::string website) const {

  std::regex url_regex(
      "((http|https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//"
      "=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");

  if (!std::regex_match(website.c_str(), url_regex)) {
    CustomException((char *)"not a valid url");
  }
}

size_t writeResponseToString(char *contents, size_t size, size_t nmemb,
                             void *userData) {
  size_t newLength = size * nmemb;
  try {

    ((std::string *)userData)->append((char *)contents, newLength);

  } catch (std::bad_alloc &e) {
    std::cerr << e.what() << std::endl;
  }

  return newLength;
}
size_t (*callBackToWrite)(char *, size_t, size_t,
                          void *) = writeResponseToString;

void Crawler::search_for_links(GumboNode *node) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return;
  }
  GumboAttribute *href;
  if (node->v.element.tag == GUMBO_TAG_A &&
      (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
    std::string link = (std::string)href->value;
    if (validLink(link)) {
      links->insert(link);
    }
  }

  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    search_for_links(static_cast<GumboNode *>(children->data[i]));
  }
}

std::string *Crawler::crawlWebsite(const std::string website) {

  CURL *curl;
  CURLcode res{};

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callBackToWrite);
    curl_easy_setopt(curl, CURLOPT_URL, website.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      throw CustomException((char *)(curl_easy_strerror(res)));
    }
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();

  return response;
}
std::vector<std::string> *Crawler::getAllResults(std::string website) {

  std::string *res = crawlWebsite(website);

  GumboOutput *output = gumbo_parse(res->c_str());
  search_for_links(output->root);

  for (auto &i : *links) {
    std::cout << i << std::endl;
  }
  gumbo_destroy_output(&kGumboDefaultOptions, output);
}
