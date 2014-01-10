// Created by: Peter KURNEV
// Copyright (c) 1999-2012 OPEN CASCADE SAS
//
// The content of this file is subject to the Open CASCADE Technology Public
// License Version 6.5 (the "License"). You may not use the content of this file
// except in compliance with the License. Please obtain a copy of the License
// at http://www.opencascade.org and read it completely before using this file.
//
// The Initial Developer of the Original Code is Open CASCADE S.A.S., having its
// main offices at: 1, place des Freres Montgolfier, 78280 Guyancourt, France.
//
// The Original Code and all software distributed under the License is
// distributed on an "AS IS" basis, without warranty of any kind, and the
// Initial Developer hereby disclaims all such warranties, including without
// limitation, any warranties of merchantability, fitness for a particular
// purpose or non-infringement. Please see the License for the specific terms
// and conditions governing the rights and limitations under the License.


#include <BOPAlgo_WireSplitter.ixx>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>

#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>

#include <BOPCol_ListOfShape.hxx>
#include <BOPCol_IndexedMapOfShape.hxx>
#include <BOPCol_MapOfShape.hxx>
#include <BOPCol_IndexedDataMapOfShapeListOfShape.hxx>
#include <BOPCol_TBB.hxx>
#include <BOPCol_NCVector.hxx>

#include <BOPTools.hxx>


//=======================================================================
//function : 
//purpose  : 
//=======================================================================
BOPAlgo_WireSplitter::BOPAlgo_WireSplitter()
:
  BOPAlgo_Algo(),
  myWES(NULL),
  myLCB(myAllocator)
{
}
//=======================================================================
//function : 
//purpose  : 
//=======================================================================
BOPAlgo_WireSplitter::BOPAlgo_WireSplitter
  (const Handle(NCollection_BaseAllocator)& theAllocator)
:
  BOPAlgo_Algo(theAllocator),
  myWES(NULL),
  myLCB(myAllocator)
{
}
//=======================================================================
//function : ~
//purpose  : 
//=======================================================================
BOPAlgo_WireSplitter::~BOPAlgo_WireSplitter()
{
}
//=======================================================================
//function : SetWES
//purpose  : 
//=======================================================================
void BOPAlgo_WireSplitter::SetWES(const BOPAlgo_WireEdgeSet& theWES)
{
  myWES=(BOPAlgo_WireEdgeSet*)&theWES;
}
//=======================================================================
//function : WES
//purpose  : 
//=======================================================================
BOPAlgo_WireEdgeSet& BOPAlgo_WireSplitter::WES()
{
  return *myWES;
}
//=======================================================================
// function: CheckData
// purpose: 
//=======================================================================
void BOPAlgo_WireSplitter::CheckData()
{
  myErrorStatus=0;
  if (!myWES) {
    myErrorStatus=10;
    return;
  }
}
//=======================================================================
//function : Perform
//purpose  : 
//=======================================================================
void BOPAlgo_WireSplitter::Perform()
{
  myErrorStatus=0;
  //
  CheckData();
  if (myErrorStatus) {
    return;
  }
  //
  MakeConnexityBlocks();
  MakeWires();
}

//=======================================================================
//function : MakeConnexityBlocks
//purpose  : 
//=======================================================================
void BOPAlgo_WireSplitter::MakeConnexityBlocks()
{
  Standard_Boolean bRegular, bClosed;
  Standard_Integer i, j, aNbV, aNbVS, aNbVP, k;
  TopoDS_Iterator aItE;
  TopoDS_Shape aER;
  BOPCol_ListIteratorOfListOfShape aIt;
  BOPCol_MapIteratorOfMapOfShape aItM;
  //
  BOPCol_IndexedDataMapOfShapeListOfShape aMVE(100, myAllocator);
  BOPCol_IndexedMapOfShape aMVP(100, myAllocator);
  BOPCol_IndexedMapOfShape aMEC(100, myAllocator);
  BOPCol_MapOfShape aMER(100, myAllocator);
  BOPCol_MapOfShape aMEP(100, myAllocator);
  BOPCol_IndexedMapOfShape aMVAdd(100, myAllocator);
  BOPCol_MapOfShape aMVS(100, myAllocator);
  //
  myLCB.Clear();
  //
  const BOPCol_ListOfShape& aLSE=myWES->StartElements();
  aIt.Initialize(aLSE);
  for (; aIt.More(); aIt.Next()) {
    const TopoDS_Shape& aE=aIt.Value();
    if (aMEP.Add(aE)) {
      BOPTools::MapShapesAndAncestors(aE, TopAbs_VERTEX, TopAbs_EDGE, aMVE);
    }
    else {
      aMER.Add(aE);
    }
  }
  //
  // 2
  aNbV=aMVE.Extent();
  for (i=1; i<=aNbV; ++i) {
    aNbVS=aMVS.Extent();
    if (aNbVS==aNbV) {
      break;
    }
    //
    const TopoDS_Shape& aV=aMVE.FindKey(i);
    //
    if (!aMVS.Add(aV)) {
      continue;
    }
        // aMVS - globally processed vertices
    //
    //------------------------------------- goal: aMEC
    aMEC.Clear();    // aMEC - edges of CB
    aMVP.Clear();    // aMVP - vertices to process right now 
    aMVAdd.Clear();  // aMVAdd vertices to process on next step of while(1)
    //
    aMVP.Add(aV);
    //
    for(;;) {
      aNbVP=aMVP.Extent();
      for (k=1; k<=aNbVP; ++k) {
        const TopoDS_Shape& aVP=aMVP(k);
        const BOPCol_ListOfShape& aLE=aMVE.FindFromKey(aVP);
        aIt.Initialize(aLE);
        for (; aIt.More(); aIt.Next()) {
          const TopoDS_Shape& aE=aIt.Value();
          if (aMEC.Add(aE)) {
            aItE.Initialize(aE);
            for (; aItE.More(); aItE.Next()) {
              const TopoDS_Shape& aVE=aItE.Value();
              if (aMVS.Add(aVE)) {
                aMVAdd.Add(aVE);
              }
            }
          }
        }
      }//for (; aItM.More(); aItM.Next()) {
      //
      aNbVP=aMVAdd.Extent();
      if (!aNbVP) {
        break; // from while(1)
      }
      //
      aMVP.Clear();
      //
      for (k=1; k<=aNbVP; ++k) {
        const TopoDS_Shape& aVE=aMVAdd(k);
        aMVP.Add(aVE);
      }
      aMVAdd.Clear();
    }// while(1) {

    //-------------------------------------
    BOPTools_ConnexityBlock aCB(myAllocator);
    BOPCol_ListOfShape& aLEC=aCB.ChangeShapes();
    
    BOPCol_IndexedDataMapOfShapeListOfShape aMVER(100, myAllocator);
    //
    bRegular=Standard_True;
    Standard_Integer aNbCB = aMEC.Extent();
    for (j = 1; j <= aNbCB; j++) {
      aER = aMEC(j);
      //
      if (aMER.Contains(aER)) {
        aER.Orientation(TopAbs_FORWARD);
        aLEC.Append(aER);
        aER.Orientation(TopAbs_REVERSED);
        aLEC.Append(aER);
        bRegular=Standard_False;
      }
      else {
        aLEC.Append(aER);
      }
      //
      if (bRegular) {
        BOPTools::MapShapesAndAncestors(aER, TopAbs_VERTEX, TopAbs_EDGE, aMVER);
      }
    }
    //
    if (bRegular) {
      Standard_Integer k, aNbVR, aNbER;
      //
      aNbVR=aMVER.Extent();
      for (k=1; k<=aNbVR; ++k) {
        const BOPCol_ListOfShape& aLER=aMVER(k);
        aNbER=aLER.Extent();
        if (aNbER==1) {
          const TopoDS_Edge& aEx=TopoDS::Edge(aER);
          bClosed=BRep_Tool::IsClosed(aEx, myWES->Face());
          if (!bClosed) {
            bRegular=!bRegular;
            break;
          }
        }
        if (aNbER>2) {
          bRegular=!bRegular;
          break;
        }
      }
    }
    //
    aCB.SetRegular(bRegular);
    myLCB.Append(aCB);
  }
}
/////////////////////////////////////////////////////////////////////////

typedef BOPCol_NCVector<BOPTools_ConnexityBlock> \
  BOPTools_VectorOfConnexityBlock;

//=======================================================================
//class    : WireSplitterFunctor
//purpose  : 
//=======================================================================
class BOPAlgo_WireSplitterFunctor {
 protected:
  TopoDS_Face myFace;
  BOPTools_VectorOfConnexityBlock* myPVCB;
  //
 public:
  //
  BOPAlgo_WireSplitterFunctor(const TopoDS_Face& aF,
			      BOPTools_VectorOfConnexityBlock& aVCB) 
    : myFace(aF), myPVCB(&aVCB) {
  }
  //
  void operator()( const flexible_range<Standard_Size>& aBR ) const{
    Standard_Size i, iBeg, iEnd;
    //
    BOPTools_VectorOfConnexityBlock& aVCB=*myPVCB;
    //
    iBeg=aBR.begin();
    iEnd=aBR.end();
    for(i=iBeg; i!=iEnd; ++i) {
      BOPTools_ConnexityBlock& aCB=aVCB((Standard_Integer)i);
      //
      BOPAlgo_WireSplitter::SplitBlock(myFace, aCB);
    }
  }
};
//=======================================================================
//class    : BOPAlgo_WireSplitterCnt
//purpose  : 
//=======================================================================
class BOPAlgo_WireSplitterCnt {
 public:
  //-------------------------------
  // Perform
  Standard_EXPORT 
    static void Perform(const Standard_Boolean bRunParallel,
			const TopoDS_Face& aF, 
			BOPTools_VectorOfConnexityBlock& aVCB) {
    //
    BOPAlgo_WireSplitterFunctor aWSF(aF, aVCB);
    Standard_Size aNbVCB=aVCB.Extent();
    //
    if (bRunParallel) {
      flexible_for(flexible_range<Standard_Size>(0,aNbVCB), aWSF);
    }
    else {
      aWSF.operator()(flexible_range<Standard_Size>(0,aNbVCB));
    }
  }
  //
};
//=======================================================================
//function : MakeWires
//purpose  : 
//=======================================================================
void BOPAlgo_WireSplitter::MakeWires()
{
  Standard_Boolean bIsRegular;
  Standard_Integer aNbVCB, k;
  TopoDS_Wire aW;
  BOPTools_ListIteratorOfListOfConnexityBlock aItCB;
  BOPCol_ListIteratorOfListOfShape aIt;
  BOPTools_VectorOfConnexityBlock aVCB;
  //
  aItCB.Initialize(myLCB);
  for (; aItCB.More(); aItCB.Next()) {
    BOPTools_ConnexityBlock& aCB=aItCB.ChangeValue();
    bIsRegular=aCB.IsRegular();
    if (bIsRegular) {
      BOPCol_ListOfShape& aLE=aCB.ChangeShapes();
      BOPAlgo_WireSplitter::MakeWire(aLE, aW);
      myWES->AddShape(aW);
    }
    else {
      aVCB.Append(aCB);
    }
  }
  //
  aNbVCB=aVCB.Extent();
  const TopoDS_Face& aF=myWES->Face();
  //===================================================
  BOPAlgo_WireSplitterCnt::Perform(myRunParallel, aF, aVCB);
  //===================================================
  for (k=0; k<aNbVCB; ++k) {
    const BOPTools_ConnexityBlock& aCB=aVCB(k);
    const BOPCol_ListOfShape& aLW=aCB.Loops();
    aIt.Initialize(aLW);
    for (; aIt.More(); aIt.Next()) {
      const TopoDS_Shape& aWx=aIt.Value();
      myWES->AddShape(aWx);
    }
  }
}
