/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLLinearTransform.cxx $
  Date:      $Date: 2010-11-23 00:58:13 -0500 (Tue, 23 Nov 2010) $
  Version:   $Revision: 15552 $

==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLLinearTransform.h"

// OpenIGTLink includes
#include <igtlTransformMessage.h>

// MRML includes
#include <vtkMRMLLinearTransformNode.h>

#include <vtkMRMLModelNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelDisplayNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkObjectFactory.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>

#include <vtkAppendPolyData.h>
#include <vtkCylinderSource.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>


// VTKSYS includes
#include <vtksys/SystemTools.hxx>

const char LocatorModelReferenceRole[] = "LocatorModel";

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLLinearTransform);


//---------------------------------------------------------------------------
vtkIGTLToMRMLLinearTransform::vtkIGTLToMRMLLinearTransform()
{
    Converter = igtl::TransformConverter::New();
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLLinearTransform::~vtkIGTLToMRMLLinearTransform()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLLinearTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLLinearTransform::CreateNewNode(vtkMRMLScene* scene, const char* name)
{
  vtkMRMLLinearTransformNode* transformNode;

  transformNode = vtkMRMLLinearTransformNode::New();
  transformNode->SetName(name);
  transformNode->SetDescription("Received by OpenIGTLink");

  vtkMatrix4x4* transform = vtkMatrix4x4::New();
  transform->Identity();
  //transformNode->SetAndObserveImageData(transform);
  transformNode->ApplyTransformMatrix(transform);
  transform->Delete();

  vtkMRMLNode* n = scene->AddNode(transformNode);
  transformNode->Delete();

  return n;
}

//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLLinearTransform::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);

  return events;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLLinearTransform::IGTLToMRML(igtl::MessageBase::Pointer buffer, vtkMRMLNode* node)
{
  vtkIGTLToMRMLBase::IGTLToMRML(buffer, node);

  vtkMRMLLinearTransformNode * transformNode = vtkMRMLLinearTransformNode::SafeDownCast(node);
  if (!transformNode)
      return 0;

  igtl::TransformConverter::MessageContent content;
  if (Converter->IGTLToVTK(buffer, &content, this->CheckCRC) == 0)
  {
      return 0;
  }

  transformNode->SetMatrixTransformToParent(content.matrix.GetPointer());

  return 1;

}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLLinearTransform::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg)
{
  if (mrmlNode && event == vtkMRMLTransformableNode::TransformModifiedEvent)
    {
    vtkMRMLLinearTransformNode* transformNode =
      vtkMRMLLinearTransformNode::SafeDownCast(mrmlNode);

    igtl::TransformConverter::MessageContent content;
    content.matrix = vtkMatrix4x4::New();
    transformNode->GetMatrixTransformToParent(content.matrix.GetPointer());
    content.deviceName = mrmlNode->GetName();

    if (this->OutTransformMsg.IsNull())
      {
      this->OutTransformMsg = igtl::TransformMessage::New();
      }

    if (Converter->VTKToIGTL(content,&(this->OutTransformMsg)) == 0)
      {
      return 0;
      }

    *size = this->OutTransformMsg->GetPackSize();
    *igtlMsg = (void*)this->OutTransformMsg->GetPackPointer();

    return 1;
    }

  return 0;
}


//---------------------------------------------------------------------------
void vtkIGTLToMRMLLinearTransform::SetVisibility(int sw, vtkMRMLScene * scene, vtkMRMLNode * node)
{
  vtkMRMLLinearTransformNode * tnode = vtkMRMLLinearTransformNode::SafeDownCast(node);

  if (!tnode || !scene)
    {
    // If the node is not a linear transform node, do nothing.
    return;
    }

  vtkMRMLModelNode* locatorModel = vtkMRMLModelNode::SafeDownCast(tnode->GetNodeReference(LocatorModelReferenceRole));
 
  if (!locatorModel) // no locator has been created
    {
    if (sw)
      {
      std::stringstream ss;
      ss << "Locator_" << tnode->GetName();
      locatorModel = AddLocatorModel(scene, ss.str().c_str(), 0.0, 1.0, 1.0);
      if (locatorModel)
        {
        tnode->SetNodeReferenceID(LocatorModelReferenceRole, locatorModel->GetID());
        scene->Modified();
        locatorModel->SetAndObserveTransformNodeID(tnode->GetID());
        locatorModel->InvokeEvent(vtkMRMLTransformableNode::TransformModifiedEvent);
        }
      }
    else
      {
      locatorModel = NULL;
      }
    }
  else
    {
    locatorModel->SetAndObserveTransformNodeID(tnode->GetID());
    locatorModel->InvokeEvent(vtkMRMLTransformableNode::TransformModifiedEvent);
    }

  if (locatorModel)
    {
    vtkMRMLDisplayNode* locatorDisp = locatorModel->GetDisplayNode();
    if (locatorDisp)
      {
      locatorDisp->SetVisibility(sw);
      locatorModel->Modified();
      }
    }
}


//---------------------------------------------------------------------------
vtkMRMLModelNode* vtkIGTLToMRMLLinearTransform::AddLocatorModel(vtkMRMLScene * scene, const char* nodeName, double r, double g, double b)
{

  vtkMRMLModelNode           *locatorModel;
  vtkMRMLModelDisplayNode    *locatorDisp;

  locatorModel = vtkMRMLModelNode::New();
  locatorDisp = vtkMRMLModelDisplayNode::New();

  // Cylinder represents the locator stick
  vtkCylinderSource *cylinder = vtkCylinderSource::New();
  cylinder->SetRadius(1.5);
  cylinder->SetHeight(100);
  cylinder->SetCenter(0, 0, 0);
  cylinder->Update();

  // Rotate cylinder
  vtkTransformPolyDataFilter *tfilter = vtkTransformPolyDataFilter::New();
  vtkTransform* trans =   vtkTransform::New();
  trans->RotateX(90.0);
  trans->Translate(0.0, -50.0, 0.0);
  trans->Update();
#if (VTK_MAJOR_VERSION <= 5)
  tfilter->SetInput(cylinder->GetOutput());
#else
  tfilter->SetInputConnection(cylinder->GetOutputPort());
#endif
  tfilter->SetTransform(trans);
  tfilter->Update();

  // Sphere represents the locator tip
  vtkSphereSource *sphere = vtkSphereSource::New();
  sphere->SetRadius(3.0);
  sphere->SetCenter(0, 0, 0);
  sphere->Update();

  vtkAppendPolyData *apd = vtkAppendPolyData::New();
#if (VTK_MAJOR_VERSION <= 5)
  apd->AddInput(sphere->GetOutput());
  //apd->AddInput(cylinder->GetOutput());
  apd->AddInput(tfilter->GetOutput());
#else
  apd->AddInputConnection(sphere->GetOutputPort());
  apd->AddInputConnection(tfilter->GetOutputPort());
#endif
  apd->Update();

  locatorModel->SetAndObservePolyData(apd->GetOutput());

  double color[3];
  color[0] = r;
  color[1] = g;
  color[2] = b;
  //locatorDisp->SetPolyData(locatorModel->GetPolyData());
  locatorDisp->SetColor(color);

  trans->Delete();
  tfilter->Delete();
  cylinder->Delete();
  sphere->Delete();
  apd->Delete();

  scene->SaveStateForUndo();
  scene->AddNode(locatorDisp);
  vtkMRMLNode* lm = scene->AddNode(locatorModel);
  locatorDisp->SetScene(scene);
  locatorModel->SetName(nodeName);
  locatorModel->SetScene(scene);
  locatorModel->SetAndObserveDisplayNodeID(locatorDisp->GetID());
  locatorModel->SetHideFromEditors(0);

  locatorModel->Delete();
  locatorDisp->Delete();

  return vtkMRMLModelNode::SafeDownCast(lm);

}
