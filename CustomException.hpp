#pragma once

#include <iostream>

class CustomException : public std::exception {

private:
  char *message{};

public:
  CustomException(char *msg) : message(msg){};

  char *what() { return message; }
};
