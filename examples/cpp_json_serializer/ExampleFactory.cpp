#include "ExampleFactory.h"
#include "ExampleTypes.h"

std::shared_ptr<void> ExampleFactory::Create(const char* aTypeName)
{
  if (strcmp(aTypeName, "SerializableClass") == 0)
  {
    return std::make_shared<SerializableClass>();
  }

  return std::shared_ptr<void>();
}
