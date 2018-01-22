#pragma once

#include <fstream>

#include "Serializable.h"

  //---------------------------------------------------------------------------//
    enum class ESerializationMode
    {
      STORE,
      LOAD
    };
  //---------------------------------------------------------------------------//
    class Serializer
    {
      public:
        Serializer(ESerializationMode aMode);
        virtual ~Serializer();

        ESerializationMode getMode() { return myMode; }

        template<class T> void Serialize(T* anObject, const char* aName = nullptr)
        {
          DataType dataType = Get_DataType<T>::get();
          serializeImpl(dataType, anObject, aName);
        }
        
        template<class T, unsigned int N> void serializeArray(T(&anObject)[N], const char* aName = nullptr)
        {
          serializeImpl(Get_DataTypeBuiltinArray<T, N>::get(), anObject, aName);
        }

    protected:

      virtual bool serializeImpl(DataType aDataType, void* anObject, const char* aName) = 0;

      virtual void beginName(const char* aName, bool anIsArray) = 0;
      virtual void endName() = 0;

      ESerializationMode myMode;
      std::fstream myArchive;
    };
  //---------------------------------------------------------------------------//
  //---------------------------------------------------------------------------//
