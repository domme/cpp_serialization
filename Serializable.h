#pragma once

#include <memory>

#include "Description.h"
#include "Factory.h"
#include <vector>
#include <cassert>

//---------------------------------------------------------------------------//
  template <typename T>
  class HasSerializeMemFn
  {
    typedef char Yes;
    typedef long No;

    template <typename C> static Yes Test(decltype(&C::Serialize));
    template <typename C> static No Test(...);

  public:
     enum {value = sizeof(Test<T>(0)) == sizeof(Yes) };
  };
//---------------------------------------------------------------------------//
  template <typename T>
  class HasGetDescriptionMemFn
  {
    typedef char Yes;
    typedef long No;

    template <typename C> static Yes Test(decltype(&C::GetDescription));
    template <typename C> static No Test(...);

  public:
    enum { value = sizeof(Test<T>(0)) == sizeof(Yes) };
  };
//---------------------------------------------------------------------------//
  template <typename T>
  class IsSerializable
  {
    typedef char Yes;
    typedef long No;

    template <typename C> static Yes Test(decltype(&C::IsSerializable));
    template <typename C> static No Test(...);

  public:
    enum { value = sizeof(Test<T>(0)) == sizeof(Yes) };
  };
//---------------------------------------------------------------------------//
  class Serializer;
//---------------------------------------------------------------------------//
  enum class EBaseDataType
  {
    None,

    Int,
    Uint,
    Float,
    Char,
    Bool,
    String,
    CString,
    Array,
    Serializable,
    SerializablePtr,
    StructOrClass,
    Description,
    Described,
  };
//---------------------------------------------------------------------------//
  struct MetaTableStructOrClass
  {
    virtual void Serialize(Serializer* aSerializer, void* anObject) = 0;
  };
//---------------------------------------------------------------------------//
  template<class T>
  struct MetaTableStructOrClassImpl : public MetaTableStructOrClass
  {
    void Serialize(Serializer* aSerializer, void* anObject) override
    {
      static_cast<T*>(anObject)->Serialize(aSerializer);
    }

    static MetaTableStructOrClassImpl<T> ourVTable;
  };
  template<class T>
  MetaTableStructOrClassImpl<T> MetaTableStructOrClassImpl<T>::ourVTable;
//---------------------------------------------------------------------------//
  struct MetaTableDescription
  {
    virtual void Serialize(Serializer* aSerializer, void* anObject) = 0;
    virtual void SetFromOtherDesc(void* anObject, void* anOtherObject) = 0;
  };
//---------------------------------------------------------------------------//
  template<class T>
  struct MetaTableDescriptionImpl : public MetaTableDescription
  {
    void Serialize(Serializer* aSerializer, void* anObject) override
    {
      static_cast<T*>(anObject)->Serialize(aSerializer);
    }

    void SetFromOtherDesc(void* anObject, void* anOtherObject) override
    {
      *static_cast<T*>(anObject) = *static_cast<T*>(anOtherObject);
    }
  
    static MetaTableDescriptionImpl<T> ourVTable;
  };
  template<class T>
  MetaTableDescriptionImpl<T> MetaTableDescriptionImpl<T>::ourVTable;
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
  struct DataType
  {
    explicit DataType(EBaseDataType aBaseType, void* aUserData = nullptr) 
      : myBaseType(aBaseType), myUserData(aUserData) {}

    EBaseDataType myBaseType;
    void* myUserData;  // for Serializable types, this pointer hods a metatable-pointer which handles construction/destruction and serialization
  };
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
  template<class T, typename IsEnumT = void, typename HasSerializationMethodT = void, typename HasGetDescriptionMethodT = void>
  struct Get_DataType
  {
    static DataType get()
    {
      return T::template getDataType<void>();
    }
  };
//---------------------------------------------------------------------------//
  // Special case for enum types
  template<class T>
  struct Get_DataType<T, 
      typename std::enable_if<std::is_enum<T>::value>::type>
  {
    static DataType get()
    {
      return DataType(EBaseDataType::Uint);
    }
  };
//---------------------------------------------------------------------------//
  template<class T>
  struct Get_DataType<T, 
    void,  // No enum type
    std::enable_if_t<HasSerializeMemFn<T>::value && !std::is_base_of<Description, T>::value>, // Serialize() but no Description
    void>  // No GetDescription()
  {
    static DataType get()
    {
      return DataType(EBaseDataType::StructOrClass, &MetaTableStructOrClassImpl<T>::ourVTable);
    }
  };
//---------------------------------------------------------------------------//
  // Special case if T is a Description
  template<class T>
  struct Get_DataType<T, void,
    std::enable_if_t<std::is_base_of<Description, T>::value>>
  {
    static DataType get()
    {
      return DataType(EBaseDataType::Description, &MetaTableDescriptionImpl<T>::ourVTable);
    }
  };
//---------------------------------------------------------------------------//
  template<class T>
  struct Get_DataType<std::shared_ptr<T>>
  {
    static DataType get()
    {
      return T::template getDataTypePtr<void>();
    }
  };
//---------------------------------------------------------------------------//
  template<class T>
  struct Get_DataType<std::shared_ptr<T>*>
  {
    static DataType get()
    {
      return T::template getDataTypePtr<void>();
    }
  };
//---------------------------------------------------------------------------//
  template<class T>
  struct Get_DataType<T*>
  {
    static DataType get()
    {
      return T::template getDataTypeRawPtr<void>();
    }
  };
//---------------------------------------------------------------------------//
#define DECLARE_DATATYPE(T, ET) \
  template<> \
  struct Get_DataType<T> \
  { \
    static DataType get() \
    { \
      return DataType(EBaseDataType::ET); \
    } \
  };

  // Base types (without meta-table)
  DECLARE_DATATYPE(int, Int);
  DECLARE_DATATYPE(unsigned int, Uint);
  DECLARE_DATATYPE(float, Float);
  DECLARE_DATATYPE(char, Char);
  DECLARE_DATATYPE(const char*, CString);
  DECLARE_DATATYPE(std::string, String);
  DECLARE_DATATYPE(bool, Bool);
#undef DECLARE_DATATYPE

//---------------------------------------------------------------------------//
  struct MetaTable
  {
    virtual ~MetaTable() {}
    virtual void create(void* anObject, Factory* aFactory, const char* aTypeName, void* aUserData) = 0;
    virtual const char* getTypeName(void* anObject) { return ""; }
    virtual unsigned int getHash(void* anObject) { return 0u; }
    virtual void Serialize(Serializer* aSerializer, void* anObject) = 0;
    virtual bool isValid(void* anObject) { return true; }
    virtual void invalidate(void* anObject) {}
  };
//---------------------------------------------------------------------------//
  struct MetaTableArray
  {
    virtual ~MetaTableArray() {}
    virtual void* getElement(void* anObject, unsigned int anIndex) = 0;
    virtual unsigned int getSize(void* anObject) = 0;
    virtual void resize(void* anObject, unsigned int aNewSize) = 0;
    virtual DataType getElementDataType() = 0;
  };
//---------------------------------------------------------------------------//
  struct MetaTableDescribed
  {
    virtual ~MetaTableDescribed() {}
    virtual void Create(void* anObject, Factory* aFactory, const char* aTypeName, const Description& aDescription, void* aUserData) = 0;
    virtual const char* GetTypeName(void* anObject) = 0;
    virtual unsigned int GetHash(void* anObject) = 0;
    virtual bool IsValid(void* anObject) = 0;
    virtual void Invalidate(void* anObject) = 0;
    virtual std::shared_ptr<Description> GetDescription(void* anObject) = 0;
    virtual std::shared_ptr<Description> CreateDescription() = 0;
  };
//---------------------------------------------------------------------------//
  namespace Internal
  {
    //---------------------------------------------------------------------------//
    // Default Serializable objects
    //---------------------------------------------------------------------------//
    template<class T>
    struct MetaTableImpl : public MetaTable
    {
      void create(void* anObject, Factory* aFactory, const char* aTypeName, void* aUserData) override { }

      const char* getTypeName(void* anObject) override
      {
        T* serializable = static_cast<T*>(anObject);
        return serializable->getTypeName();
      }

      void Serialize(Serializer* aSerializer, void* anObject) override
      {
        T* serializable = static_cast<T*>(anObject);
        serializable->Serialize(aSerializer);
      }

      static MetaTableImpl<T> ourVTable;
    };
    template<class T>
    MetaTableImpl<T> MetaTableImpl<T>::ourVTable;
//---------------------------------------------------------------------------//
    // Raw ptrs to Serializable objects
    template<class T>
    struct MetaTableImpl<T*> : public MetaTable
    {
      void create(void* anObject, Factory* aFactory, const char* aTypeName, void* aUserData) override
      {
        T** serializable = static_cast<T**>(anObject);
        (*serializable) = static_cast<T*>(aFactory->Create(aTypeName, aUserData));
      }

      bool isValid(void* anObject) override
      {
        T** serializable = static_cast<T**>(anObject);
        return (*serializable) != nullptr;
      }

      void invalidate(void* anObject) override
      {
        T** serializable = static_cast<T**>(anObject);
        (*serializable) = nullptr;
      }

      const char* getTypeName(void* anObject) override
      {
        T** serializable = static_cast<T**>(anObject);
        return (*serializable)->getTypeName();
      }

      unsigned int getHash(void* anObject) override
      {
        T** serializable = static_cast<T**>(anObject);
        return (*serializable)->GetHash();
      }

      void Serialize(Serializer* aSerializer, void* anObject) override
      {
        T** serializable = static_cast<T**>(anObject);
        (*serializable)->Serialize(aSerializer);
      }

      static MetaTableImpl<T*> ourVTable;
    };
    template<class T>
    MetaTableImpl<T*> MetaTableImpl<T*>::ourVTable;
//---------------------------------------------------------------------------//
    // Shared ptrs to Serializable objects
    template<class T>
    struct MetaTableImpl<std::shared_ptr<T>> : public MetaTable
    {
      virtual ~MetaTableImpl<std::shared_ptr<T>>() {}
      
      void create(void* anObject, Factory* aFactory, const char* aTypeName, void* aUserData) override
      {
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        (*serializable) = std::shared_ptr<T>(static_cast<T*>(aFactory->Create(aTypeName, aUserData)));
      }

      void invalidate(void* anObject) override
      {
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        serializable->reset();
      }

      const char* getTypeName(void* anObject) override
      {
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        return (*serializable)->getTypeName();
      }

      unsigned int getHash(void* anObject) override
      {
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        return (*serializable)->GetHash();
      }

      void Serialize(Serializer* aSerializer, void* anObject) override
      {
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        (*serializable)->Serialize(aSerializer);
      }

      static MetaTableImpl<std::shared_ptr<T>> ourVTable;
    };
    template<class T>
    MetaTableImpl<std::shared_ptr<T>> MetaTableImpl<std::shared_ptr<T>>::ourVTable;
  //---------------------------------------------------------------------------//
    // Default template for described objects (dummy - should never be instanciated)
    template<class T>
    struct MetaTableDescribedImpl : public MetaTableDescribed
    {
    
    };
//---------------------------------------------------------------------------//
     // Shared pointers to descibed objects
    template<class T>
    struct MetaTableDescribedImpl<std::shared_ptr<T>> : public MetaTableDescribed
    {
      void Create(void* anObject, Factory* aFactory, const char* aTypeName, const Description& aDescription, void* aUserData) override
      {
        std::shared_ptr<T> createdObject = std::static_pointer_cast<T>(aFactory->Create(aTypeName, aDescription, aUserData));
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        serializable->swap(createdObject);
      }

      const char* GetTypeName(void* anObject) override
      {
        return GetDescription(anObject)->GetTypeName();
      }

      unsigned int GetHash(void* anObject) override
      {
        return GetDescription(anObject)->GetHash();
      }

      bool IsValid(void* anObject) override
      {
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        return (*serializable) != nullptr;
      }

      void Invalidate(void* anObject) override
      {
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        serializable->reset();
      }

      std::shared_ptr<Description> GetDescription(void* anObject) override 
      { 
        typedef typename T::DescT DescT;
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        std::shared_ptr<DescT> desc(new DescT((*serializable)->GetDescription()));
        return desc;
      }

      std::shared_ptr<Description> CreateDescription() override
      {
        typedef typename T::DescT DescT;
        return std::make_shared<DescT>();
      }
      
      static MetaTableDescribedImpl<std::shared_ptr<T>> ourVTable;
    };
    template<class T>
    MetaTableDescribedImpl<std::shared_ptr<T>> MetaTableDescribedImpl<std::shared_ptr<T>>::ourVTable;
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
  // Array metatables
//---------------------------------------------------------------------------//
    // Dummy general template:
    template<class T>
    struct MetaTableArrayImpl : public MetaTableArray
    {
      void* getElement(void* anObject, unsigned int anIndex) override { return nullptr; }
      unsigned int getSize(void* anObject) override { return 0u; }
      void resize(void* anObject, unsigned int aNewSize) override { }
      DataType getElementDataType() override { return DataType(EBaseDataType::None); }
      static MetaTableArrayImpl<T> ourVTable;
    };
    template<class T>
    MetaTableArrayImpl<T> MetaTableArrayImpl<T>::ourVTable;
//---------------------------------------------------------------------------//
    // Specialization for std::vector
    template<class T>
    struct MetaTableArrayImpl<std::vector<T>> : public MetaTableArray
    {
      void* getElement(void* anObject, unsigned int anIndex) override
      {
        std::vector<T>* array = reinterpret_cast<std::vector<T>*>(anObject);
        return &((*array)[anIndex]);
      }

      unsigned int getSize(void* anObject) override
      {
        std::vector<T>* array = reinterpret_cast<std::vector<T>*>(anObject);
        return static_cast<unsigned int>(array->size());
      }

      void resize(void* anObject, unsigned int aNewSize) override
      {
        std::vector<T>* array = reinterpret_cast<std::vector<T>*>(anObject);
        return array->resize(aNewSize);
      }

      DataType getElementDataType() override
      {
        return Get_DataType<T>::get();
      }

      static MetaTableArrayImpl<std::vector<T>> ourVTable;
    };
    template<class T>
    MetaTableArrayImpl<std::vector<T>> MetaTableArrayImpl<std::vector<T>>::ourVTable;
//---------------------------------------------------------------------------//
    // Specialization for C-Arrays
    template<class T, unsigned int Capacity>
    struct MetaTableArrayImpl<T[Capacity]> : public MetaTableArray
    {
      using BuitinArrayT = T[Capacity];

      void* getElement(void* anObject, unsigned int anIndex) override
      {
        BuitinArrayT* array = reinterpret_cast<BuitinArrayT*>(anObject);
        return &((*array)[anIndex]);
      }

      unsigned int getSize(void* anObject) override
      {
        return Capacity;
      }

      void resize(void* anObject, unsigned int aNewSize) override
      {
        assert(aNewSize == Capacity);
      }

      DataType getElementDataType() override
      {
        return Get_DataType<T>::get();
      }

      static MetaTableArrayImpl<T[Capacity]> ourVTable;
    };
    template<class T, unsigned int Capacity>
    MetaTableArrayImpl<T[Capacity]> MetaTableArrayImpl<T[Capacity]>::ourVTable;
//---------------------------------------------------------------------------//
  }  // end of namespace Internal
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
  template<class T>
  struct Get_DataType<std::vector<T>>
  {
    static DataType get()
    {
      return DataType(EBaseDataType::Array, &Internal::MetaTableArrayImpl<std::vector<T>>::ourVTable);
    }
  };
//---------------------------------------------------------------------------//
  template<class T, unsigned int Capacity>
  struct Get_DataTypeBuiltinArray
  {
    static DataType get()
    {
      return DataType(EBaseDataType::Array, &Internal::MetaTableArrayImpl<T[Capacity]>::ourVTable);
    }
  };
  //---------------------------------------------------------------------------//
  // Special case if T has a GetDescription() method and is no Description
  template<class T>
  struct Get_DataType<T,
    void, // No enum type
    void, // No Serialize()
    std::enable_if_t<HasGetDescriptionMemFn<T>::value>>  // GetDescription() 
  {
    static DataType get()
    {
      return DataType(EBaseDataType::Described, &Internal::MetaTableDescribedImpl<T>::ourVTable);
    }
  };
