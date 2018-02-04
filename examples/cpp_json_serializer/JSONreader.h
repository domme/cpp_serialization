#pragma once

#if 1

#include "Serializer.h"
#include "json/json.h"

//---------------------------------------------------------------------------//
  class JSONreader : public Serializer
  {
  public:
    JSONreader(const std::string& anArchivePath, Factory* aFactory);
    ~JSONreader() override;

    const unsigned int myVersion = 0;

  protected:

    struct RootHeader
    {
      unsigned int myVersion;
      Json::Value* myInstances;
      std::map<unsigned int, std::shared_ptr<void>> myCreatedInstances;
    };

    Json::Value* GetInstanceVal(unsigned int aHash) const;

    bool serializeImpl(DataType aDataType, void* anObject, const char* aName) override;

    void beginName(const char* aName, bool anIsArray) override;
    void endName() override;

    void loadHeader();

    Factory* myFactory;

    RootHeader myHeader;
    Json::Value myDocumentVal;

    std::stack<Json::ArrayIndex> myArrayIndexStack;
    std::stack<Json::Value*> myTypeStack;
  };
//---------------------------------------------------------------------------//  

#endif