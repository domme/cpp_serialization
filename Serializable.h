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
    Description,
    Described,
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
  template<class T, typename IsEnumT = void, typename HasSerializationMethodT = void, typename HasGetDescriptionMethodT = void>
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
  };
//---------------------------------------------------------------------------//
  struct MetaTableDescription
  {
    virtual void Serialize(Serializer* aSerializer, void* anObject) = 0;
    virtual void SetFromOtherDesc(void* anObject, void* anOtherObject) = 0;
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
    template<class T>
    struct MetaTableDescribedImpl : public MetaTableDescribed
    { };  // Default template for described objects (dummy - should never be instanciated)
//---------------------------------------------------------------------------//
     // Shared pointers to descibed objects. This is the only specialization we provide for described objects. Feel free to add more (e.g. raw pointers, unique ptrs, ...)
    template<class T>
    struct MetaTableDescribedImpl<std::shared_ptr<T>> : public MetaTableDescribed
    {
      typedef std::result_of_t<decltype(&T::GetDescription)(T)> DescT;

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
        std::shared_ptr<T>* serializable = static_cast<std::shared_ptr<T>*>(anObject);
        std::shared_ptr<DescT> desc(new DescT((*serializable)->GetDescription()));
        return desc;
      }

      std::shared_ptr<Description> CreateDescription() override
      {
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
    std::enable_if_t<HasSerializeMemFn<T>::value && !std::is_base_of<Description, T>::value>, // Serialize() but no Description
    void>  // No GetDescription()
  {
    static DataType get()
    {
      return DataType(EBaseDataType::StructOrClass, &Internal::MetaTableStructOrClassImpl<T>::ourVTable);
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
      return DataType(EBaseDataType::Description, &Internal::MetaTableDescriptionImpl<T>::ourVTable);
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
  // Specialization for shared_ptrs to described objects
  template<class T>
  struct Get_DataType<std::shared_ptr<T>,
    void, // No enum type
    void, // No Serialize()
    std::enable_if_t<HasGetDescriptionMemFn<T>::value>>  // GetDescription() 
  {
    static DataType get()
    {
      return DataType(EBaseDataType::Described, &Internal::MetaTableDescribedImpl<std::shared_ptr<T>>::ourVTable);
    }
  };
//---------------------------------------------------------------------------//
