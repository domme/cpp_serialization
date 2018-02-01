#include "JSONwriter.h"

//---------------------------------------------------------------------------//
  JSONwriter::JSONwriter(const std::string& anArchivePath) 
    : Serializer(ESerializationMode::STORE)
  {
    unsigned int archiveFlags = 0u;
    archiveFlags |= std::ios::out;

    myArchive.open(anArchivePath, archiveFlags);

    myHeader = RootHeader();
    myHeader.myInstances = Json::Value(Json::arrayValue);
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
    case EBaseDataType::Serializable:
    {
      MetaTableSerializable* metaTable = static_cast<MetaTableSerializable*>(aDataType.myUserData);
      if (!metaTable->IsValid(anObject))
      {
        currJsonVal = NULL;
        break;
      }

      unsigned int hash = metaTable->GetHash(anObject);
      const char* typeName = metaTable->GetTypeName(anObject);

      currJsonVal["Type"] = typeName;
      currJsonVal["Hash"] = hash;

      if (!HasInstanceStored(hash))
      {
        myTypeStack.push(Json::Value(Json::objectValue));
        Json::Value& descVal = myTypeStack.top();
        descVal["Type"] = typeName;
        descVal["Hash"] = hash;
        metaTable->Serialize(this, anObject);
        AddStoredInstance(metaTable->GetTypeName(anObject), descVal, hash);
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
  void JSONwriter::AddStoredInstance(const char* aTypeName, const Json::Value& aDescriptionVal, unsigned int aHash)
  {
    myHeader.myInstances.append(aDescriptionVal);
    myHeader.myInstanceHashes.push_back(aHash);
  }
//---------------------------------------------------------------------------//
  bool JSONwriter::HasInstanceStored(unsigned aHash)
  {
    for (unsigned int storedHash : myHeader.myInstanceHashes)
      if (storedHash == aHash)
        return true;

    return false;
  }
//---------------------------------------------------------------------------//
  void JSONwriter::storeHeader(Json::Value& aValue) const
  {
    aValue["myVersion"] = myHeader.myVersion;
    aValue["myInstances"] = myHeader.myInstances;
  }
//---------------------------------------------------------------------------//
