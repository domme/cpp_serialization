#include "JSONwriter.h"

//---------------------------------------------------------------------------//
  JSONwriter::JSONwriter(const std::string& anArchivePath) 
    : Serializer(ESerializationMode::STORE)
  {
    unsigned int archiveFlags = 0u;
    archiveFlags |= std::ios::out;

    myArchive.open(anArchivePath, archiveFlags);

    myHeader = RootHeader();
    myHeader.myDescribedObjects = Json::Value(Json::arrayValue);
    JSONwriter::beginName("Root", false);
  }
//---------------------------------------------------------------------------//
  JSONwriter::~JSONwriter()
  {
    JSONwriter::endName();

    storeHeader(myCurrentEndType);
    myJsonWriter.write(myArchive, myCurrentEndType);
  }  
//---------------------------------------------------------------------------//
  bool JSONwriter::serializeImpl(DataType aDataType, void* anObject, const char* aName)
  {
    beginName(aName, aDataType.myBaseType == EBaseDataType::Array);

    Json::Value& currJsonVal = myTypeStack.top();

    bool handled = true;
    switch (aDataType.myBaseType)
    {
    case EBaseDataType::StructOrClass:
    {
      MetaTableStructOrClass* metaTable = static_cast<MetaTableStructOrClass*>(aDataType.myUserData);
      metaTable->Serialize(this, anObject);
    } break;
    case EBaseDataType::Described:
    {
      MetaTableDescribed* metaTable = static_cast<MetaTableDescribed*>(aDataType.myUserData);
      if (!metaTable->IsValid(anObject))
      {
        currJsonVal = NULL;
        break;
      }

      std::shared_ptr<Description> desc = metaTable->GetDescription(anObject);
      
      currJsonVal["Type"] = metaTable->GetTypeName(anObject);
      currJsonVal["Hash"] = desc->GetHash();
      
      Serialize(desc.get(), "Description");
    } break;
    case EBaseDataType::Description:
    {
      Description* desc = static_cast<Description*>(anObject);

      const unsigned int descHash = desc->GetHash();

      currJsonVal["Type"] = desc->GetTypeName();
      currJsonVal["Hash"] = descHash;
      
      if (!HasDescribedObject(descHash))
      {
        myTypeStack.push(Json::Value(Json::objectValue));
        Json::Value& descVal = myTypeStack.top();
        descVal["Type"] = desc->GetTypeName();
        descVal["Hash"] = descHash;
        desc->Serialize(this);
        AddDescribedObject(desc->GetTypeName(), descVal, descHash);
        myTypeStack.pop();
      }
    } break;
    case EBaseDataType::Int:
    {
      currJsonVal = *static_cast<int*>(anObject);
    } break;

    case EBaseDataType::Uint:
    {
      currJsonVal = *static_cast<unsigned int*>(anObject);
    } break;

    case EBaseDataType::Float:
    {
      currJsonVal = *static_cast<float*>(anObject);
    } break;

    case EBaseDataType::Char:
    {
      currJsonVal = *static_cast<char*>(anObject);
    } break;

    case EBaseDataType::Bool:
    {
      currJsonVal = *static_cast<bool*>(anObject);
    } break;

    case EBaseDataType::String:
    {
      currJsonVal = *static_cast<std::string*>(anObject);
    } break;

    case EBaseDataType::CString:
    {
      currJsonVal = *static_cast<const char*>(anObject);
    } break;

    case EBaseDataType::Array:
    {
      MetaTableArray* arrayVtable = reinterpret_cast<MetaTableArray*>(aDataType.myUserData);
      unsigned int numElements = arrayVtable->getSize(anObject);
      for (unsigned int i = 0u; i < numElements; ++i)
        serializeImpl(arrayVtable->getElementDataType(), arrayVtable->getElement(anObject, i), nullptr);
    } break;

    case EBaseDataType::None:
    default:
      handled = false;
      break;
    }

    endName();

    return handled;
  }
//---------------------------------------------------------------------------//
  void JSONwriter::beginName(const char* aName, bool anIsArray)
  {
    myTypeStack.push(anIsArray ? Json::Value(Json::arrayValue) : Json::Value(Json::objectValue));
    myNameStack.push(aName);
  }
//---------------------------------------------------------------------------//
  void JSONwriter::endName()
  {
    assert(!myTypeStack.empty());
    assert(!myNameStack.empty());

    myCurrentEndType = myTypeStack.top();
    myTypeStack.pop();

    const char* name = myNameStack.top();
    myNameStack.pop();

    if (!myTypeStack.empty())
    {
      Json::Value& parentVal = myTypeStack.top();
      const bool isArray = parentVal.type() == Json::arrayValue;

      if (isArray)
        parentVal.append(myCurrentEndType);
      else
        parentVal[name] = myCurrentEndType;
    }
  }
//---------------------------------------------------------------------------//
  void JSONwriter::AddDescribedObject(const char* aTypeName, const Json::Value& aDescriptionVal, unsigned int aHash)
  {
    myHeader.myDescribedObjects.append(aDescriptionVal);
    myHeader.myDescribedObjectHashes.push_back(aHash);
  }
//---------------------------------------------------------------------------//
  bool JSONwriter::HasDescribedObject(unsigned aHash)
  {
    for (unsigned int storedHash : myHeader.myDescribedObjectHashes)
      if (storedHash == aHash)
        return true;

    return false;
  }
//---------------------------------------------------------------------------//
  void JSONwriter::storeHeader(Json::Value& aValue) const
  {
    aValue["myVersion"] = myHeader.myVersion;
    aValue["myDescribedObjects"] = myHeader.myDescribedObjects;
  }
//---------------------------------------------------------------------------//
