#pragma once

#include "Serializer.h"
#include "Serializable.h"

struct SimpleStruct
{
  SimpleStruct()
    : myIntVal(2)
    , myStrVal("Test") {}

  void Serialize(Serializer* aSerializer)
  {
    aSerializer->Serialize(&myIntVal, "myIntVal");
    aSerializer->Serialize(&myStrVal, "myStrVal");
  }

  int myIntVal;
  std::string myStrVal;
};

struct SerializableClass
{
  SerializableClass()
    : myIntVal(3)
    , myFloatVal(4.721f) {}

  void Serialize(Serializer* aSerializer)
  {
    aSerializer->Serialize(&myIntVal, "myIntVal");
    aSerializer->Serialize(&myFloatVal, "myFloatVal");
  }

  const char* GetTypeName() const { return "SerializableClass"; }

  int myIntVal;
  float myFloatVal;
};

SERIALIZABLE(SerializableClass);
