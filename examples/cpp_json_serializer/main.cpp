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

struct Description1 : Description
{
  const char* GetTypeName() const override { return "Described1"; }
  unsigned GetHash() const override { return std::hash<int>()(myIntVal) ^ std::hash<float>()(myFloatVal); }
  void Serialize(Serializer* aSerializer) override
  {
    aSerializer->Serialize(&myIntVal, "myIntVal");
    aSerializer->Serialize(&myFloatVal, "myFloatVal");
  }

  int myIntVal;
  float myFloatVal;
};

struct Described1
{
  Description1 GetDescription() const
  {
    Description1 desc;
    desc.myIntVal = myIntVal;
    desc.myFloatVal = myFloatVal;
    return desc;
  }

  int myIntVal;
  float myFloatVal;
};

struct Description2 : Description
{
  const char* GetTypeName() const override { return "Described2"; }
  unsigned GetHash() const override { return myDescribed1.GetHash(); }
  void Serialize(Serializer* aSerializer) override
  {
    aSerializer->Serialize(&myDescribed1, "myDescribed1");
  }

  Description1 myDescribed1;
};

struct Described2
{
  Description2 GetDescription() const
  {
    Description2 desc;
    desc.myDescribed1 = myDescribed1->GetDescription();
    return desc;
  }

  std::shared_ptr<Described1> myDescribed1;
};

int main(int argc, char **argv, char **envp)
{
  printf("Hello world\n");
  
  int test_int = 1;
  float test_float = 0.245f;
  ExampleStruct test_struct{ 2, "SomeString" };

  std::shared_ptr<Described1> described1 = std::make_shared<Described1>();
  described1->myIntVal = 4;
  described1->myFloatVal = 0.7821f;

  std::shared_ptr<Described2> described2 = std::make_shared<Described2>();
  described2->myDescribed1 = described1;

  {
    JSONwriter writer("example_builtin_types.json");
    writer.Serialize(&test_int, "test_int");
    writer.Serialize(&test_float, "test_float");

    writer.Serialize(&test_struct, "test_struct");

    writer.Serialize(&described2, "described2");
  }
    
  system("PAUSE");

  return 1;
}
