#include <cstdio>
#include <cstdlib>

#include "JSONwriter.h"


struct ExampleStruct
{
  void Serialize(Serializer* aSerializer)
  {
    aSerializer->Serialize(&myIntVal, "myIntVal");
    aSerializer->Serialize(&myStrVal, "myStrVal");
  }

  int myIntVal;
  std::string myStrVal;
};

int main(int argc, char **argv, char **envp)
{
  printf("Hello world\n");
  
  int test_int = 1;
  float test_float = 0.245f;
  ExampleStruct test_struct{ 2, "SomeString" };

  {
    JSONwriter writer("example_builtin_types.json");
    writer.Serialize(&test_int, "test_int");
    writer.Serialize(&test_float, "test_float");
    writer.Serialize(&test_struct, "test_struct");
  }
    
  system("PAUSE");

  return 1;
}
