#include "JSONreader.h"

#if 0

#include "GpuProgram.h"

#include "Model.h"

#include "ObjectName.h"

namespace Fancy { namespace IO {
//---------------------------------------------------------------------------//
  Json::Value nullVal = Json::Value(NULL);
//---------------------------------------------------------------------------//
  JSONreader::JSONreader(const String& anArchivePath, GraphicsWorld& aGraphicsWorld) 
    : Serializer(ESerializationMode::LOAD)
    , myGraphicsWorld(aGraphicsWorld)
  {
    uint archiveFlags = 0u;

    archiveFlags |= std::ios::in;

    String archivePath = anArchivePath + ".json";
    myArchive.open(archivePath, archiveFlags);

    if (myArchive.good())
    {
      myArchive >> myDocumentVal;
    }

    myTypeStack.push(&myDocumentVal);
    loadHeader();
  }
//---------------------------------------------------------------------------//
  JSONreader::~JSONreader()
  {
    
  }
//---------------------------------------------------------------------------//
  void JSONreader::SerializeDescription(DescriptionBase* aDescription)
  {
    aDescription->Serialize(this);
  }
//---------------------------------------------------------------------------//
  DescriptionBase* JSONreader::GetResourceDesc(uint64 aHash)
  {
    for (DescriptionBase* desc : myHeader.myLoadedDecscs)
      if (desc->GetHash() == aHash)
        return desc;

    return nullptr;
  }
//---------------------------------------------------------------------------//
  Json::Value* JSONreader::GetResourceVal(const ObjectName& aTypeName, uint64 aHash) const
  {
    for (int i = 0; i < (int) myHeader.myResources->size(); ++i)
    {
      Json::ArrayIndex index(i);
      Json::Value& val = (*myHeader.myResources)[index];
      if (val["Type"].asString() == aTypeName.toString() 
       && val["Hash"].asUInt64() == aHash)
      {
        return &val;
      }
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
    case EBaseDataType::ResourceDesc:
    {
      DescriptionBase* desc = static_cast<DescriptionBase*>(anObject);

      ObjectName typeName = ObjectName(currJsonVal["Type"].asString());
      uint64 hash = currJsonVal["Hash"].asUInt64();

      if (DescriptionBase* loadedDesc = GetResourceDesc(hash))
      {
        MetaTableResourceDesc* metaTable = static_cast<MetaTableResourceDesc*>(aDataType.myUserData);
        metaTable->SetFromOtherDesc(anObject, loadedDesc);
      }
      else
      {
        Json::Value* jsonValInHeader = GetResourceVal(typeName, hash);
        ASSERT(jsonValInHeader != nullptr);

        myTypeStack.push(jsonValInHeader);
        desc->Serialize(this);
        myTypeStack.pop();

        myHeader.myLoadedDecscs.push_back(desc);
      }
    } break;
    case EBaseDataType::ResourcePtr:
    {
      MetaTableResource* metaTable = static_cast<MetaTableResource*>(aDataType.myUserData);

      if (currJsonVal.type() == Json::nullValue
        || ( currJsonVal.type() == Json::intValue && currJsonVal.asInt() == 0))  // nullptrs in arrays will appear as "0"
      {
        metaTable->Invalidate(anObject);
        break;
      }

      ObjectName typeName = ObjectName(currJsonVal["Type"].asString());
      uint64 hash = currJsonVal["Hash"].asUInt64();

      DescriptionBase* description = GetResourceDesc(hash);
      if (description == nullptr)
      {
        SharedPtr<DescriptionBase> newDesc = metaTable->CreateDescription();
        myHeader.myCreatedDescs.push_back(newDesc);
        Serialize(newDesc.get(), "Description");
        description = GetResourceDesc(hash);
        ASSERT(description != nullptr);
      }

      metaTable->Create(anObject, typeName, *description, &myGraphicsWorld);
      
    } break;
    case EBaseDataType::Serializable:
    case EBaseDataType::SerializablePtr:
    {
      MetaTable* metaTable = static_cast<MetaTable*>(aDataType.myUserData);

      if (currJsonVal.type() == Json::nullValue)
      {
        metaTable->invalidate(anObject);
        break;
      }

      ObjectName typeName = ObjectName(currJsonVal["Type"].asString());
      uint64 instanceHash = currJsonVal["Hash"].asUInt64();

      bool wasCreated = false;
      metaTable->create(anObject, &myGraphicsWorld, typeName, wasCreated, instanceHash);

      if (wasCreated)
        metaTable->Serialize(this, anObject);

    } break;

    case EBaseDataType::Int:
    {
      *static_cast<int*>(anObject) = currJsonVal.asInt();
    } break;

    case EBaseDataType::Uint:
    {
      *static_cast<uint*>(anObject) = currJsonVal.asUInt();
    } break;

    case EBaseDataType::Uint8:
    {
      uint val32 = currJsonVal.asUInt();
      *static_cast<uint8*>(anObject) = static_cast<uint8>(val32);
    } break;

    case EBaseDataType::Uint16:
    {
      uint val32 = currJsonVal.asUInt();
      *static_cast<uint16*>(anObject) = static_cast<uint16>(val32);
    } break;

    case EBaseDataType::Uint64:
    {
      uint64 val = currJsonVal.asUInt64();
      *static_cast<uint64*>(anObject) = static_cast<uint64>(val);
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
      *static_cast<String*>(anObject) = currJsonVal.asString();
    } break;

    case EBaseDataType::CString:
    {
      *static_cast<const char**>(anObject) = currJsonVal.asCString();
    } break;

    case EBaseDataType::ObjectName:
    {
      String name = currJsonVal.asString();
      *static_cast<ObjectName*>(anObject) = name;
    } break;

    case EBaseDataType::Array:
    {
      MetaTableArray* arrayVtable = reinterpret_cast<MetaTableArray*>(aDataType.myUserData);
      uint numElements = currJsonVal.size();
      arrayVtable->resize(anObject, numElements);
      for (uint i = 0u; i < numElements; ++i)
        serializeImpl(arrayVtable->getElementDataType(), arrayVtable->getElement(anObject, i), nullptr);
    } break;

    case EBaseDataType::Map: break;

    case EBaseDataType::Vector3:
    {
      glm::vec3& val = *static_cast<glm::vec3*>(anObject);
      for (Json::ArrayIndex i = 0u; i < (uint) val.length(); ++i)
        val[i] = currJsonVal[i].asFloat();
    } break;

    case EBaseDataType::Vector4:
    {
      glm::vec4& val = *static_cast<glm::vec4*>(anObject);
      for (Json::ArrayIndex i = 0u; i < (uint) val.length(); ++i)
        val[i] = currJsonVal[i].asFloat();
    } break;

    case EBaseDataType::Quaternion:
    {
      glm::quat& val = *static_cast<glm::quat*>(anObject);
      for (Json::ArrayIndex i = 0u; i < (uint) val.length(); ++i)
        val[i] = currJsonVal[i].asFloat();
    } break;

    case EBaseDataType::Matrix3x3:
    {
      glm::mat3& val = *static_cast<glm::mat3*>(anObject);
      for (Json::ArrayIndex y = 0u; y < (uint) val.length(); ++y)
        for (Json::ArrayIndex x = 0u; x < (uint) val[y].length(); ++x)
          val[x][y] = currJsonVal[y * val[y].length() + x].asFloat();
    } break;

    case EBaseDataType::Matrix4x4:
    {
      glm::mat4& val = *static_cast<glm::mat4*>(anObject);
      for (Json::ArrayIndex y = 0u; y < (uint) val.length(); ++y)
        for (Json::ArrayIndex x = 0u; x < (uint) val[y].length(); ++x)
          val[x][y] = currJsonVal[y * val[y].length() + x].asFloat();
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
    ASSERT(!myTypeStack.empty(), "Mismatching number of beginType() / endType() calls");

    Json::Value& val = *myTypeStack.top();
    if (val.isArray())
      myArrayIndexStack.pop();

    myTypeStack.pop();
  }
//---------------------------------------------------------------------------//
  void JSONreader::loadHeader()
  {
    myHeader.myVersion = myDocumentVal["myVersion"].asUInt();
    myHeader.myResources = &myDocumentVal["myResources"];
  }
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
} }  // end of namespace Fancy::IO

#endif