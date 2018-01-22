#pragma once

class Description;

class Factory
{
public:
  virtual void* Create(const char* aTypeName, void* aUserData) = 0;
  virtual void* Create(const char* aTypeName, const Description& aDesc, void* aUserData) = 0;
};