#include "JSONreader.h"

#if 1

//---------------------------------------------------------------------------//
  Json::Value nullVal = Json::Value(NULL);
//---------------------------------------------------------------------------//
  JSONreader::JSONreader(const std::string& anArchivePath, Factory* aFactory) 
    : Serializer(ESerializationMode::LOAD)
    , myFactory(aFactory)
  {
    myArchive.open(anArchivePath, std::ios::in);

    if (myArchive.good())
    {
      myArchive >> myDocumentVal;
      myTypeStack.push(&myDocumentVal);
      loadHeader();
    }
  }
//---------------------------------------------------------------------------//
  JSONreader::~JSONreader() 
  {
    
  }
//---------------------------------------------------------------------------//
  Json::Value* JSONreader::GetInstanceVal(unsigned int aHash) const
  {
    for (int i = 0; i < (int) myHeader.myInstances->size(); ++i)
    {
      const Json::ArrayIndex index(i);
      Json::Value& val = (*myHeader.myInstances)[index];
      if (val["Hash"].asUInt64() == aHash)
        return &val;
    }

    return nullptr;
  }
//---------------------------------------------------------------------------//
  bool JSONreader::serializeImpl(DataType aDataType, void* anObject, const char* aName)
  {
    beginName(aName, aDataType.myBaseType == EBaseDataType::Array);

    Json::Value& currJsonVal = *myTypeStack.top();

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
      if (currJsonVal.type() == Json::nullValue
        || (currJsonVal.type() == Json::intValue && currJsonVal.asInt() == 0))  // nullptrs in arrays will appear as "0"
      {
        metaTable->Invalidate(anObject);
        break;
      }

      unsigned int hash = currJsonVal["Hash"].asUInt();

      auto it = myHeader.myCreatedInstances.find(hash);
      if (it != myHeader.myCreatedInstances.end())
      {
        metaTable->SetFromOther(anObject, &(*it));
        break;
      }

      Json::Value* val = GetInstanceVal(hash);
      assert(val != nullptr);  // Instance wasn't stored properly. Should never happen

      const char* typeName = (*val)["Type"].asCString();
      metaTable->Create(anObject, myFactory, typeName);

      // Serialize the new object
      myTypeStack.push(val);
      metaTable->Serialize(this, anObject);
      myTypeStack.pop();

      myHeader.myCreatedInstances[hash] = *static_cast<std::shared_ptr<void>*>(anObject);
    } break;
    case EBaseDataType::Int:
    {
      *static_cast<int*>(anObject) = currJsonVal.asInt();
    } break;
    case EBaseDataType::Uint:
    {
      *static_cast<unsigned int*>(anObject) = currJsonVal.asUInt();
    } break;
    case EBaseDataType::Float:
    {
      *static_cast<float*>(anObject) = currJsonVal.asFloat();
    } break;
    case EBaseDataType::Char:
    {
      *static_cast<char*>(anObject) = currJsonVal.asUInt();
    } break;
    case EBaseDataType::Bool:
    {
      *static_cast<bool*>(anObject) = currJsonVal.asBool();
    } break;
    case EBaseDataType::String:
    {
      *static_cast<std::string*>(anObject) = currJsonVal.asString();
    } break;
    case EBaseDataType::CString:
    {
      *static_cast<const char**>(anObject) = currJsonVal.asCString();
    } break;
    case EBaseDataType::Array:
    {
      MetaTableArray* arrayVtable = reinterpret_cast<MetaTableArray*>(aDataType.myUserData);
      const unsigned int numElements = currJsonVal.size();
      arrayVtable->resize(anObject, numElements);
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
  void JSONreader::beginName(const char* aName, bool anIsArray)
  {
    Json::Value& parentVal = *myTypeStack.top();

    Json::Value* newVal = nullptr;
    if (parentVal.isArray())
    {
      Json::ArrayIndex& index = myArrayIndexStack.top();
      Json::Value& val = parentVal[index];
      newVal = &val;
      ++index;
    }
    else
    {
      Json::Value& val = parentVal[aName];
      newVal = &val;
    }

    if ((*newVal).type() == Json::nullValue)
      return;

    myTypeStack.push(newVal);

    if (newVal->isArray())
      myArrayIndexStack.push(0);
  }
//---------------------------------------------------------------------------//
  void JSONreader::endName()
  {
    assert(!myTypeStack.empty()); //  "Mismatching number of beginType() / endType() calls"

    Json::Value& val = *myTypeStack.top();
    if (val.isArray())
      myArrayIndexStack.pop();

    myTypeStack.pop();
  }
//---------------------------------------------------------------------------//
  void JSONreader::loadHeader()
  {
    myHeader.myVersion = myDocumentVal["myVersion"].asUInt();
    myHeader.myInstances = &myDocumentVal["myInstances"];
  }
//---------------------------------------------------------------------------//

#endif