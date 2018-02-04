#pragma once
#include "Factory.h"
#include <memory>

class ExampleFactory final : public Factory
{
public:
  std::shared_ptr<void> Create(const char* aTypeName) override;
};


