#include <cstdio>
#include <cstdlib>

#include "JSONwriter.h"
#include "JSONreader.h"
#include "ExampleTypes.h"
#include "ExampleFactory.h"

int main(int argc, char **argv, char **envp)
{
  printf("Hello world\n");
  
  int test_int = 1;
  float test_float = 0.245f;

  SimpleStruct simpleStruct;
  std::shared_ptr<SerializableClass> complexInstance = std::make_shared<SerializableClass>();

  std::vector<int> exampleVec;
  exampleVec.push_back(1);
  exampleVec.push_back(2);
  exampleVec.push_back(3);

  float exampleArray[] = { 4.0f, 5.0f, 6.0f, 7.0f };

  {
    JSONwriter writer("example_builtin_types.json");
    writer.Serialize(&test_int, "test_int");
    writer.Serialize(&test_float, "test_float");
    writer.Serialize(&simpleStruct, "simpleStruct");
    writer.Serialize(&complexInstance, "complexInstance");
    writer.Serialize(&exampleVec, "exampleVec");
    writer.SerializeArray(exampleArray, "exampleArray");
  }

  {
    ExampleFactory factory;
    JSONreader reader("example_builtin_types.json", &factory);
    reader.Serialize(&test_int, "test_int");
    reader.Serialize(&test_float, "test_float");
    reader.Serialize(&simpleStruct, "simpleStruct");
    reader.Serialize(&complexInstance, "complexInstance");
    reader.Serialize(&exampleVec, "exampleVec");
    reader.SerializeArray(exampleArray, "exampleArray");
  }


    
  system("PAUSE");

  return 1;
}
