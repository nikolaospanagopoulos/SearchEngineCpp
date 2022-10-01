#include "Crawler.hpp"
#include "CustomException.hpp"

#include <iostream>

int main() {

  try {

    Crawler *crawler = new Crawler;

    crawler->getAllResults("https://www.cnn.gr/");

    delete crawler;

    return 0;
  } catch (CustomException &e) {

    std::cerr << e.what() << std::endl;
    return 1;
  }
}
