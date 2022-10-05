#include "Crawler.hpp"
#include "CustomException.hpp"

#include <iostream>

int main() {

  Crawler *crawler = new Crawler;
  try {

    crawler->getAllResults("https://www.cnn.gr/");

    crawler->showResults();

    delete crawler;

    return 0;
  } catch (CustomException &e) {

    delete crawler;
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
