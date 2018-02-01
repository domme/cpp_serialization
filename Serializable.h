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
    StructOrClass,
    Serializable,
  };
//---------------------------------------------------------------------------//
  struct DataType
  {
    explicit DataType(EBaseDataType aBaseType, void* aUserData = nullptr) 
      : myBaseType(aBaseType), myUserData(aUserData) {}

    EBaseDataType myBaseType;
    void* myUserData;  // pointer to meta-table for complex types
  };
//---------------------------------------------------------------------------//
  template<class T, typename IsEnumT = void, typename HasSerializationMethodT = void, typename IsSerializable = void>
  struct Get_DataType { };  // Dummy base type, should never be instantiated
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
  struct MetaTableStructOrClass
  {
    virtual void Serialize(Serializer* aSerializer, void* anObject) = 0;
    virtual void SetFromOther(void* anObject, const void* anOtherObject) = 0;
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
  struct MetaTableSerializable
  {
    virtual ~MetaTableSerializable() {}
    virtual void Create(void* anObject, Factory* aFactory, const char* aTypeName, void* aUserData) = 0;
    virtual void Serialize(Serializer* aSerializer, void* anObject) = 0;
    virtual const char* GetTypeName(void* anObject) = 0;
    virtual unsigned int GetHash(void* anObject) = 0;
    virtual bool IsValid(void* anObject) = 0;
    virtual void Invalidate(void* anObject) = 0;
  };
//---------------------------------------------------------------------------//
  namespace Internal
  {
  //---------------------------------------------------------------------------//
    template<class T>
    struct MetaTableStructOrClassImpl : public MetaTableStructOrClass
    {
      void Serialize(Serializer* aSerializer, void* anObject) override
      {
        static_cast<T*>(anObject)->Serialize(aSerializer);
      }

      void SetFromOther(void* anObject, const void* anOtherObject) override
      {
        *static_cast<T*>(anObject) = *static_cast<const T*>(anOtherObject);
      }

      static MetaTableStructOrClassImpl<T> ourVTable;
    };
    template<class T>
    MetaTableStructOrClassImpl<T> MetaTableStructOrClassImpl<T>::ourVTable;
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Serializable Meta Tables
//---------------------------------------------------------------------------//
    template<class T>
    struct MetaTableSerializableImpl : public MetaTableSerializable
    { };  // Default template for described objects (dummy - should never be instanciated)
//---------------------------------------------------------------------------//
    // Shared pointers to descibed objects. This is the only specialization we provide for described objects. Feel free to add more (e.g. raw pointers, unique ptrs, ...)
    template<class T>
    struct MetaTableSerializableImpl<std::shared_ptr<T>> : public MetaTableSerializable
    {
      void Create(void* anObject, Factory* aFactory, const char* aTypeName, void* aUserData) override
      {
        std::shared_ptr<T> createdObject = std::static_pointer_cast<T>(aFactory->Create(aTypeName, aUserData));
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        serializable->swap(createdObject);

        // Still needs to be serialized when loading!
      }

      void Serialize(Serializer* aSerializer, void* anObject) override
      {
        static_cast<T*>(anObject)->Serialize(aSerializer);
      }

      const char* GetTypeName(void* anObject) override
      {
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        return (*serializable)->GetTypeName();
      }

      unsigned int GetHash(void* anObject) override
      {
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        return reinterpret_cast<unsigned int>((*serializable).get());
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

      static MetaTableSerializableImpl<std::shared_ptr<T>> ourVTable;
    };
    template<class T>
    MetaTableSerializableImpl<std::shared_ptr<T>> MetaTableSerializableImpl<std::shared_ptr<T>>::ourVTable;
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
  // Array metatables
//---------------------------------------------------------------------------//
    template<class T>
    struct MetaTableArrayImpl : public MetaTableArray
    {
      // Dummy general template, should never be instantiated
    };  
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
  struct Get_DataType<T,
    void,  // No enum type
    std::enable_if_t<HasSerializeMemFn<T>::value>> // Serialize()
  {
    static DataType get()
    {
      return DataType(EBaseDataType::StructOrClass, &Internal::MetaTableStructOrClassImpl<T>::ourVTable);
    }
  };
//---------------------------------------------------------------------------//
  template<class T>
  struct Get_DataType<std::shared_ptr<T>,
    void,  // No enum type
    std::enable_if_t<HasSerializeMemFn<T>::value>, // Serialize() but no Description
    std::enable_if_t<T::IsSerializable::Val>>
  {
    static DataType get()
    {
      return DataType(EBaseDataType::Serializable, &Internal::MetaTableSerializableImpl<std::shared_ptr<T>>::ourVTable);
    }
  };
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
#define SERIALIZABLE(T) \
public: \
  enum IsSerializable { Val = true }; \
  const char* GetTypeName() const { return #T; }
//---------------------------------------------------------------------------//
#define SERIALIZABLE_VIRTUAL(T) \
public: \
  enum IsSerializable { Val = true }; \
  virtual const char* GetTypeName() const { return #T; }
//---------------------------------------------------------------------------//