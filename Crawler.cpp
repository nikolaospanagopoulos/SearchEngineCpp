#include "Crawler.hpp"
#include "exception"
#include <boost/regex.hpp>

#include "CustomException.hpp"
#include <cstddef>
#include <curl/curl.h>
#include <regex>

Crawler::Crawler() {
  response = new std::string{};

  alreadyCrawled = new std::unordered_set<std::string>{};
  links = new std::queue<std::string>{};
}

Crawler::~Crawler() {
  delete response;
  delete links;
  delete alreadyCrawled;
  response = nullptr;
  alreadyCrawled = nullptr;
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

void Crawler::saveRawLinks(std::string &website, GumboOutput *output) {
  search_for_links(output->root, website);
}
void Crawler::search_for_links(GumboNode *node, const std::string &url) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return;
  }
  GumboAttribute *href;
  if (node->v.element.tag == GUMBO_TAG_A &&
      (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
    std::string link = (std::string)href->value;
    makeLinkProper(link, url);
    if (!existsInSet(link)) {
      links->push(link);
      alreadyCrawled->insert(link);
    }
  }

  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    search_for_links(static_cast<GumboNode *>(children->data[i]), url);
  }
}

void Crawler::showResults() const {

  for (const auto &res : *alreadyCrawled) {
    std::cout << res << std::endl;
  }

  std::cout << "the size is: " << alreadyCrawled->size() << std::endl;
}

std::string *Crawler::crawlWebsite(const std::string website) {

  if (!validLink(website)) {
    throw CustomException((char *)"not a valid url");
  }
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
void Crawler::getAllResults(std::string website) {

  std::string *res = crawlWebsite(website);

  GumboOutput *output = gumbo_parse(res->c_str());
  try {
    saveRawLinks(website, output);
    while (!links->empty()) {
      std::string linkToParse = links->front();
      alreadyCrawled->insert(linkToParse);

      std::string title = (std::string)find_title(output->root);
      std::string description = search_for_metatag(output->root, "description");
      std::string keywords = search_for_metatag(output->root, "keywords");
      links->pop();

      if (alreadyCrawled->size() > 300) {
        break;
      }
      getAllResults(linkToParse);
    }
    gumbo_destroy_output(&kGumboDefaultOptions, output);

  } catch (const std::runtime_error &re) {
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    throw CustomException((char *)re.what());
  } catch (CustomException &e) {
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    throw CustomException((char *)e.what());
  } catch (const std::exception &ex) {
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    throw CustomException((char *)ex.what());
  } catch (...) {
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    throw CustomException((char *)"something went wrong");
  }
}

bool Crawler::existsInSet(const std::string &word) const {
  for (auto &wordInSet : *alreadyCrawled) {
    if (wordInSet == word) {
      return true;
    }
  }
  return false;
}
void Crawler::makeLinkProper(std::string &link, const std::string &url) {
  boost::regex ex(
      "(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
  boost::cmatch what;
  if (!regex_match(url.c_str(), what, ex)) {
    return;
  }
  std::string protocol{what[1].first, what[1].second};
  std::string host{what[2].first, what[2].second};
  std::string path{what[4].first, what[4].second};

  if (link.substr(0, 2) == "//") {
    link = protocol + ":" + link;
  } else if (link.substr(0, 1) == "/") {
    link = protocol + "://" + host + link;
  } else if (link.substr(0, 2) == "./") {
    link = protocol + "://" + host + path + link.substr(1);
  } else if (link.substr(0, 2) == "../") {
    link = protocol + "://" + host + "/" + link;
  } else if (link.substr(0, 4) != "http") {
    link = protocol + "://" + host + "/" + link;
  }
}

GumboNode *Crawler::findHead(const GumboNode *root) {

  if (root->type != GUMBO_NODE_ELEMENT) {
    throw CustomException((char *)"not a valid element \n");
  }
  const GumboVector *root_children = &root->v.element.children;
  GumboNode *head = NULL;
  for (int i = 0; i < root_children->length; ++i) {
    GumboNode *child = (GumboNode *)root_children->data[i];
    if (child->type == GUMBO_NODE_ELEMENT &&
        child->v.element.tag == GUMBO_TAG_HEAD) {
      head = child;
      break;
    }
  }

  return head;
}

std::string Crawler::search_for_metatag(GumboNode *root, std::string tagType) {

  GumboNode *head = findHead(root);

  if (head == nullptr) {
    throw CustomException((char *)"something went wrong");
  }

  std::string metaInfo{};
  GumboAttribute *metaTagInfo;
  GumboVector *head_children = &head->v.element.children;
  for (int i = 0; i < head_children->length; ++i) {
    GumboNode *child = (GumboNode *)head_children->data[i];
    if (child->type == GUMBO_NODE_ELEMENT &&
        child->v.element.tag == GUMBO_TAG_META) {
      if ((metaTagInfo =
               gumbo_get_attribute(&child->v.element.attributes, "name"))) {

        if ((std::string)metaTagInfo->value == tagType) {

          metaInfo = gumbo_get_attribute(

                         &child->v.element.attributes, "content")
                         ->value;
        }
      }
    }
  }
  return metaInfo;
}
const char *Crawler::find_title(const GumboNode *root) {

  GumboNode *head = findHead(root);

  if (head == nullptr) {
    throw CustomException((char *)"something went wrong");
  }

  GumboVector *head_children = &head->v.element.children;
  for (int i = 0; i < head_children->length; ++i) {
    GumboNode *child = (GumboNode *)head_children->data[i];
    if (child->type == GUMBO_NODE_ELEMENT &&
        child->v.element.tag == GUMBO_TAG_TITLE) {
      if (child->v.element.children.length != 1) {
        return "<empty title>";
      }
      GumboNode *title_text = (GumboNode *)child->v.element.children.data[0];

      assert(title_text->type == GUMBO_NODE_TEXT ||
             title_text->type == GUMBO_NODE_WHITESPACE);
      return title_text->v.text.text;
    }
  }
  return "<no title found>";
}
