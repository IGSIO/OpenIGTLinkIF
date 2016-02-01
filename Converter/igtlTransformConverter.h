/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLImage.h $
  Date:      $Date: 2010-11-23 00:58:13 -0500 (Tue, 23 Nov 2010) $
  Version:   $Revision: 15552 $

==========================================================================*/

#ifndef __igtlTransformConverter_h
#define __igtlTransformConverter_h

#include "igtlSupportModuleConverterExport.h"

// OpenIGTLink includes
#include <igtlTransformMessage.h>

// VTK includes
#include <vtkObject.h>
#include <vtkSmartPointer.h>

class vtkMatrix4x4;

#include "igtlLightObject.h"

namespace igtl
{

/** Conversion between igtl::TransformMessage and vtk classes.
 *
 */
class OPENIGTLINK_SUPPORT_MODULE_CONVERTER_EXPORT TransformConverter : public LightObject
{
public:
  /** Standard class typedefs. */
  typedef TransformConverter       Self;
  typedef LightObject              Superclass;
  typedef SmartPointer<Self>       Pointer;
  typedef SmartPointer<const Self> ConstPointer;

  /** Method for creation through the object factory. */
  igtlNewMacro(Self);

  /** Run-time type information (and related methods). */
  igtlTypeMacro(TransformConverter, LightObject);

  /**
   * This structure contains everything that igtl::PolyMessage is able to contain,
   * in a vtk-friendly format.
   */
  struct MessageContent
  {
    vtkSmartPointer<vtkMatrix4x4> matrix;
    std::string deviceName;
  };

  virtual void PrintSelf(std::ostream& os) const;

  virtual const char*  GetIGTLName() { return "TRANSFORM"; };

  virtual int IGTLToVTK(igtl::MessageBase::Pointer source, MessageContent* dest, bool checkCRC);
  virtual int VTKToIGTL(const MessageContent& source, igtl::TransformMessage::Pointer* dest);

protected:
  TransformConverter();
  ~TransformConverter();

private:
  // Extract vtkMatrix4x4 from existing transformMsg, insert into existing vtkMatrix4x4.
  int IGTLToVTKMatrix4x4(TransformMessage::Pointer transformMsg, vtkSmartPointer<vtkMatrix4x4> mat);

};

}


#endif //__igtlTransformConverter_h
