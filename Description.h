#pragma once

class Serializer;

class Description
{
public:
  virtual ~Description() = default;
  virtual const char* GetTypeName() const = 0;
  virtual unsigned int GetHash() const = 0;
  virtual void Serialize(Serializer* aSerializer) = 0;
  virtual bool IsEmpty() const { return false; }
};