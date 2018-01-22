#include "Serializer.h"

//---------------------------------------------------------------------------//
  Serializer::Serializer(ESerializationMode aMode) :
    myMode(aMode) 
  {
    
  }
//---------------------------------------------------------------------------//
  Serializer::~Serializer()
  {
    if (myArchive.good())
    {
      myArchive.close();
    }
  }  
//---------------------------------------------------------------------------//
