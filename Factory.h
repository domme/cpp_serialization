#pragma once

#include <memory>

class Factory
{
public:
  virtual std::shared_ptr<void> Create(const char* aTypeName) = 0;
};