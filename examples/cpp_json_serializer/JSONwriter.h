#pragma once

#include "Serializer.h"
#include "json/json.h"

//---------------------------------------------------------------------------//
  class JSONwriter : public Serializer
  {
  public:
    JSONwriter(const std::string& anArchivePath);
    virtual ~JSONwriter() override;

  protected:
    
    struct RootHeader
    {
      unsigned int myVersion;
      
      Json::Value myInstances;
      std::vector<unsigned int> myInstanceHashes;
    };

    bool serializeImpl(DataType aDataType, void* anObject, const char* aName) override;

    void beginName(const char* aName, bool anIsArray) override;
    void endName() override;

    void AddStoredInstance(const char* aTypeName, const Json::Value& aDescriptionVal, unsigned int aHash);
    bool HasInstanceStored(unsigned int aHash);
    
    void storeHeader(Json::Value& aValue) const;

    RootHeader myHeader;
    Json::Value myCurrentEndType;

    std::stack<const char*> myNameStack;
    std::stack<Json::Value> myTypeStack;
    Json::StyledStreamWriter myJsonWriter;
  };
//---------------------------------------------------------------------------//  
