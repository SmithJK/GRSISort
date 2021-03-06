#include <iostream>
#include <fstream>
#include <string>

#include "TClass.h"
#include "TPaveStats.h"
#include "TList.h"
#include "TText.h"
#include "TLatex.h"
#include "TH1.h"
#include "TGraphErrors.h"
#include "Buttons.h"
#include "KeySymbols.h" 
#include "TVirtualX.h"
#include "TROOT.h"
#include "TFrame.h"
#include "TF1.h"
#include "TGraph.h"
#include "TPolyMarker.h"
#include "TSpectrum.h"
#include "TMath.h"
#include "TApplication.h"
#include "TContextMenu.h"

#include "Globals.h"

#include "GCanvas.h"
#include "GROOTGuiFactory.h"
#include "GRootObjectManager.h"
#include "GRootGlobals.h"


#ifndef kArrowKeyPress
#define kArrowKeyPress 25
#define kArrowKeyRelease 26
#endif

/// \cond CLASSIMP
ClassImp(GMarker)
/// \endcond

void GMarker::Copy(TObject &object) const {
   TObject::Copy(object);
   static_cast<GMarker&>(object).fX      = fX;
   static_cast<GMarker&>(object).fY      = fY;
   static_cast<GMarker&>(object).fLocalX = fLocalX;
   static_cast<GMarker&>(object).fLocalY = fLocalY;
   static_cast<GMarker&>(object).fLineX  = 0;
   static_cast<GMarker&>(object).fLineY  = 0;
}

int GCanvas::fLastX = 0;
int GCanvas::fLastY = 0;

int GCanvas::fBGSubtraction_type=0;

GCanvas::GCanvas(Bool_t build)
	: TCanvas(build) {
   GCanvasInit();
}


GCanvas::GCanvas(const char* name, const char* title, Int_t form)
	: TCanvas(name,title,form) { 
   GCanvasInit();
}


GCanvas::GCanvas(const char* name, const char* title, Int_t ww, Int_t wh)
	: TCanvas(name,title,ww,wh) { 
   GCanvasInit();
}


GCanvas::GCanvas(const char* name, Int_t ww, Int_t wh, Int_t winid)
	: TCanvas(name,ww,wh,winid) { 
   // this constructor is used to create an embedded canvas
   // I see no reason for us to support this here.  pcb.
   GCanvasInit();
}


GCanvas::GCanvas(const char* name, const char* title, Int_t wtopx, Int_t wtopy, Int_t ww, Int_t wh)
	: TCanvas(name,title,wtopx,wtopy,ww,wh) { 
   GCanvasInit();
}


GCanvas::~GCanvas() {
   //TCanvas::~TCanvas();           
}

void GCanvas::GCanvasInit() {
   printf("GCanvasInit called.\n");
   // ok, to interact with the default TGWindow
   // stuff from the root gui we need our own GRootCanvas.  
   // We make this using GROOTGuiFactory, which replaces the
   // TRootGuiFactory used in the creation of some of the 
   // default gui's (canvas,browser,etc).  
   fStatsDisplayed = true;
   fMarkerMode     = false;

   //if(gVirtualX->InheritsFrom("TGX11")) {
   //    printf("\tusing x11-like graphical interface.\n");
   //}
   //this->SetCrosshair(true);
}

void GCanvas::AddMarker(int x, int y, int dim) {
  GMarker* mark = new GMarker();
  mark->fX = x;
  mark->fY = y;
  if(dim == 1) {
    mark->fLocalX = gPad->AbsPixeltoX(x);
    mark->fLineX = new TLine(mark->fLocalX,GetUymin(),mark->fLocalX,GetUymax());
    mark->fLineX->SetLineColor(kRed);
    mark->fLineX->Draw();
  } else if (dim==2) {
    mark->fLocalX = gPad->AbsPixeltoX(x);
    mark->fLocalY = gPad->AbsPixeltoX(y);
    mark->fLineX = new TLine(mark->fLocalX,GetUymin(),mark->fLocalX,GetUymax());
    mark->fLineX->SetLineColor(kRed);
    mark->fLineY = new TLine(GetUxmin(),mark->fLocalY,GetUxmax(),mark->fLocalY);
    mark->fLineY->SetLineColor(kRed);
    mark->fLineX->Draw();
    mark->fLineY->Draw();
  }
  if(fMarkers.size()>3) {
    delete fMarkers.at(0);
    fMarkers.erase(fMarkers.begin()); 
    //fMarkers.insert(fMarkers.begin(),mark);
  } //else {
    fMarkers.push_back(mark);
  //}
  //printf("MarkerAdded %i | %i",x,y);
  return;
}

void GCanvas::RemoveMarker() {
  if(fMarkers.size()<1)
    return;
  if(fMarkers.at(fMarkers.size()-1))
     delete fMarkers.at(fMarkers.size()-1);
  //printf("Marker %i Removed\n");
  fMarkers.erase(fMarkers.end()-1);
  return;
}



void GCanvas::OrderMarkers() { 
  std::sort(fMarkers.begin(),fMarkers.end());
  return;
}



void GCanvas::AddBGMarker(GMarker* mark) {
  GMarker* bg_mark = new GMarker(*mark);
  fBGMarkers.push_back(bg_mark);
}


void GCanvas::RemoveBGMarker() {
  if(fBGMarkers.size()<1)
    return;
  if(fBGMarkers.at(0))
     delete fBGMarkers.at(0);
  //printf("Marker %i Removed\n");
  fBGMarkers.erase(fBGMarkers.begin());
  return;
}

void GCanvas::ClearBGMarkers() {
  while(fBGMarkers.size()>0)
     RemoveBGMarker();
  return;
}

void GCanvas::OrderBGMarkers() { 
  //std::sort(fBGMarkers.begin(),fBGMarkers.end());
  if(fBGMarkers.size()<2)
     return;
  GMarker mark0(*fBGMarkers.at(0));
  GMarker mark1(*fBGMarkers.at(1));
  if(mark0.fX < mark1.fX) {
    AddBGMarker(&mark0);
    AddBGMarker(&mark1);
  } else {
    AddBGMarker(&mark1);
    AddBGMarker(&mark0);
  }
  return;
}

GCanvas* GCanvas::MakeDefCanvas() { 

  // Static function to build a default canvas.

  const char* defcanvas = gROOT->GetDefCanvasName();
  char* cdef;

  TList* lc = static_cast<TList*>(gROOT->GetListOfCanvases());
  if (lc->FindObject(defcanvas)) {
    Int_t n = lc->GetSize() + 1;
    cdef = new char[strlen(defcanvas)+15];
    do {
      strlcpy(cdef,Form("%s_n%d", defcanvas, n++),strlen(defcanvas)+15);
    } while (lc->FindObject(cdef));
  } else
    cdef = StrDup(Form("%s",defcanvas));
  GCanvas* c = new GCanvas(cdef, cdef, 1);
  //printf("GCanvas::MakeDefCanvas"," created default GCanvas with name %s",cdef);
  delete [] cdef;
  return c;
}

//void GCanvas::ProcessEvent(Int_t event,Int_t x,Int_t y,TObject* obj) {
//   printf("{GCanvas} ProcessEvent:\n");
//   printf("\tevent: \t0x%08x\n",event);
//   printf("\tobject:\t0x%08x\n",obj);
//   printf("\tx:     \t0x%i\n",x);
//   printf("\ty:     \t0x%i\n",y);
//}

//void GCanvas::ExecuteEvent(Int_t event,Int_t x,Int_t y) { 
//  printf("exc event called.\n");
//}


void GCanvas::HandleInput(EEventType event,Int_t x,Int_t y) {
  //If the below switch breaks. You need to upgrade your version of ROOT
  //Version 5.34.24 works.


  bool used = false;
  switch(event) {
    case 0x00000001:
      used = HandleMousePress(event,x,y);
      break;
    default:
      break;
  };
  if(!used)
    TCanvas::HandleInput(event,x,y);
  


  return;
}


void GCanvas::UpdateStatsInfo(int x, int y) {
   TIter next(this->GetListOfPrimitives());
   TObject* obj;
   while((obj=next())) {
      if(obj->InheritsFrom("TH1")) {
         static_cast<TH1*>(obj)->SetBit(TH1::kNoStats);
         printf("found : %s\n",obj->GetName());
         TPaveStats* st = static_cast<TPaveStats*>(static_cast<TH1*>(obj)->GetListOfFunctions()->FindObject("stats"));
         st->GetListOfLines()->Delete();
         st->AddText(Form("X      %i",x));
         st->AddText(Form("Counts %i",y));
         //st->Paint();
         //gPad->Modified();
         //gPad->Update();
      }
   }
}

//void GCanvas::HandleKeyPress(int event,int x,int key,TObject* obj) {
//
//}

void GCanvas::Draw(Option_t* opt) {
   printf("GCanvas Draw was called.\n");
   TCanvas::Draw(opt);
   this->FindObject("TFrame")->SetBit(TBox::kCannotMove);
}


std::vector<TH1*> GCanvas::Find1DHists() {
  std::vector<TH1*> tempvec;
  TIter iter(gPad->GetListOfPrimitives());
  while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        tempvec.push_back(static_cast<TH1*>(obj)); 
     }
  }
  return tempvec;
}

std::vector<TH1*> GCanvas::FindAllHists() {
  std::vector<TH1*> tempvec;
  TIter iter(gPad->GetListOfPrimitives());
  while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1"))
        tempvec.push_back(static_cast<TH1*>(obj)); 
  }
  return tempvec;
}


bool GCanvas::HandleArrowKeyPress(Event_t* event,UInt_t* keysym) {

  
  std::vector<TH1*> hists = Find1DHists();
  if(hists.size()==0)
     return false;
  int first = hists.at(0)->GetXaxis()->GetFirst();
  int last = hists.at(0)->GetXaxis()->GetLast();
 
  int min = std::min(first,0);
  int max = std::max(last,hists.at(0)->GetXaxis()->GetNbins()+1);


  //printf("first = %i  |  last = %i\n", first,last);
  //printf("min   = %i  |  max  = %i\n", min,max);

  int xdiff = last-first;
  int mdiff = max-min-2;
  //if(xdiff==mdiff)
  //   return;
  TH1* temph = 0;
  switch (*keysym) {
    case 0x1012: // left
     {
        if(mdiff>xdiff) {
          if(first==(min+1)) {
            //
          }
          else if((first-(xdiff/2))<min) {
            first = min+1;
            last  = min + (xdiff) + 1;
            //last  = first-min-1 + (xdiff/2); 
          } else {
            first = first-(xdiff/2); 
            last  = last -(xdiff/2);
          }
        }
        for(size_t i=0;i<hists.size();i++)
          hists.at(i)->GetXaxis()->SetRange(first,last);
        gPad->Modified();
        gPad->Update();
      }
      //printf("LEFT\n");
      break;
    case 0x1013: // up
      //printf("UP\n");
		 temph = GRootObjectManager::Instance()->GetNext1D(static_cast<TObject*>(hists.at(0)));
      if(temph) {
        temph->GetXaxis()->SetRange(first,last);
        temph->Draw();
        gPad->Modified();
        gPad->Update();
      }
      break;
    case 0x1014: // right
     {
        //int xdiff = last-first;
        //int mdiff = max-min;
        if(mdiff>xdiff) {
          if(last== (max-1)) {
            // 
          }else if((last+(xdiff/2))>max) {
            first = max - 1 - (xdiff); 
            last  = max - 1;
          } else {
            last  = last +(xdiff/2); 
            first = first+(xdiff/2); 
          }
        }
        for(size_t i=0;i<hists.size();i++)
          hists.at(i)->GetXaxis()->SetRange(first,last);
        gPad->Modified();
        gPad->Update();
      }
      //printf("RIGHT\n");
      break;
    case 0x1015: // down
      //printf("DOWN\n");
		 temph = GRootObjectManager::Instance()->GetLast1D(static_cast<TObject*>(hists.at(0)));
      if(temph) {
        temph->GetXaxis()->SetRange(first,last);
        temph->Draw();
        gPad->Modified();
        gPad->Update();
      }
      break;
    default:
      printf("keysym = %i\n",*keysym);
      break;
  }
  return true;

}


bool GCanvas::HandleKeyboardPress(Event_t* event,UInt_t* keysym) {

  //printf("keysym = %i\n",*keysym);
  TIter iter(gPad->GetListOfPrimitives());
  TGraphErrors*  ge = 0;
  bool edit = false;
  while(TObject* obj = iter.Next()) {
     if(obj->InheritsFrom("TGraphErrors")){
		  ge = static_cast<TGraphErrors*>(obj);
     }
  }
  std::vector<TH1*> hists = Find1DHists();
  if(hists.size()==0)
     return false;

   if(hists.size()>0){
      switch(*keysym) {
         case kKey_b: {
              GMarker* markers[4] = {0};
              for(int i=0;i<GetNMarkers();i++) 
                 markers[i] = fMarkers.at(i);
              edit = SetBackGround(markers[0],markers[1],markers[2],markers[3]);
            }
            break;
         case kKey_B:
            SetBackGroundSubtractionType();
            break;
         case kKey_e:
            if(GetNMarkers()<2)
               break;
            if(fMarkers.at(fMarkers.size()-1)->fLocalX < fMarkers.at(fMarkers.size()-2)->fLocalX) 
               for(size_t i=0;i<hists.size();i++)
                 hists.at(i)->GetXaxis()->SetRangeUser(fMarkers.at(fMarkers.size()-1)->fLocalX,fMarkers.at(fMarkers.size()-2)->fLocalX);
            else
               for(size_t i=0;i<hists.size();i++)
                 hists.at(i)->GetXaxis()->SetRangeUser(fMarkers.at(fMarkers.size()-2)->fLocalX,fMarkers.at(fMarkers.size()-1)->fLocalX);
            edit = true;
            while(GetNMarkers())
               RemoveMarker();
            ClearBGMarkers();
            break;
         case kKey_E:
            GetContextMenu()->Action(hists.back()->GetXaxis(),hists.back()->GetXaxis()->Class()->GetMethodAny("SetRangeUser"));
            for(size_t i=0;i<hists.size()-1;i++)
               hists.at(i)->GetXaxis()->SetRangeUser(hists.back()->GetXaxis()->GetFirst(),hists.back()->GetXaxis()->GetLast());
            edit = true;
            break;
         case kKey_g:
            edit = GausFit();
            break;
         case kKey_G:
            edit = GausBGFit();
            break;
         case kKey_l:
            for(size_t i=0;i<hists.size();i++) {
               hists.at(i)->GetYaxis()->UnZoom();
            }
            SetLogy(0);
            edit = true;
            break;
         case kKey_L:
            for(size_t i=0;i<hists.size();i++) {
              if(hists.at(i)->GetYaxis()->GetXmin()<0)
                 hists.at(i)->GetYaxis()->SetRangeUser(0,hists.at(i)->GetYaxis()->GetXmax());
            }
            SetLogy(1);
            edit = true;
            break;
         case kKey_m:
            SetMarkerMode(true);
            break;
         case kKey_M:
            SetMarkerMode(false);
         case kKey_n: 
            while(GetNMarkers())
               RemoveMarker();
            ClearBGMarkers();
            for(size_t i=0;i<hists.size();i++)
              hists.at(i)->GetListOfFunctions()->Delete();
            edit = true;
            break; 
         case kKey_N:
            while(GetNMarkers())  
               RemoveMarker();
            ClearBGMarkers();
            if(hists.back()->GetListOfFunctions()->Last())   
               hists.back()->GetListOfFunctions()->Last()->Delete();
            edit = true;
            break;
         case kKey_o:
            for(size_t i=0;i<hists.size();i++)
              hists.at(i)->GetXaxis()->UnZoom();
            edit = true;    
            while(GetNMarkers())
               RemoveMarker();
            break;
         case kKey_p: //project.
            //printf("\n  %p\n",GRootObjectManager::Instance()->FindMemObject(hists.at(0)->GetName()));
            if(GMemObj* mobj = GRootObjectManager::Instance()->FindMemObject(hists.at(0)->GetName())) {
              //printf("object parent:  %p\n",mobj->GetParent());
              if(mobj->GetParent()) {
                 //printf("parent mobj:  %p\n", GRootObjectManager::Instance()->FindMemObject(mobj->GetParent())) ;
                 //printf("parent name:  %s\n",mobj->GetParent()->GetName());
                 if(!mobj->GetParent()->InheritsFrom("TH2"))
                   break;
                 TH1D* temphist = 0;
                 TH1*  tempbg   = 0;
                 if(GetNMarkers()<2)
                   break;
                 if(!strcmp(mobj->GetOption(),"ProjY")) {  // if we are working with a y projection, useX axis.
                   int yvalue1 = static_cast<TH2*>(mobj->GetParent())->GetYaxis()->FindBin(fMarkers.at(fMarkers.size()-1)->fLocalX);
                   int yvalue0 = static_cast<TH2*>(mobj->GetParent())->GetYaxis()->FindBin(fMarkers.at(fMarkers.size()-2)->fLocalX);
                   if(yvalue1<yvalue0) {
                      double temp = yvalue0;
                      yvalue0 = yvalue1;
                      yvalue1 = temp;
                   }
                   temphist = ProjectionX(static_cast<TH2*>(mobj->GetParent()),yvalue0,yvalue1); 

                   tempbg = GetBackGroundHist(fMarkers.at(fMarkers.size()-1),
                                              fMarkers.at(fMarkers.size()-2));
                   
                 } else {  // if we are working with a x projection, use y axis
                   int xvalue1 = static_cast<TH2*>(mobj->GetParent())->GetYaxis()->FindBin(fMarkers.at(fMarkers.size()-1)->fLocalX);
                   int xvalue0 = static_cast<TH2*>(mobj->GetParent())->GetYaxis()->FindBin(fMarkers.at(fMarkers.size()-2)->fLocalX);
                   if(xvalue1<xvalue0) {
                      double temp = xvalue0;
                      xvalue0 = xvalue1;
                      xvalue1 = temp;
                   }
                   temphist = ProjectionY(static_cast<TH2*>(mobj->GetParent()),xvalue0,xvalue1); 
                   
                   tempbg = GetBackGroundHist(fMarkers.at(fMarkers.size()-1),
                                              fMarkers.at(fMarkers.size()-2));

                 }
                 //printf("addgate: %i\n",fMarkers.at(0)->fX);
                 //printf("addgate: %i\n",fMarkers.at(1)->fX);
                 //printf("subgate: %i\n",fBGMarkers.at(0)->fX);
                 //printf("subgate: %i\n",fBGMarkers.at(1)->fX);
                 
                 //printf("i am here.\n");
                 if(tempbg){
                    temphist->Add(tempbg,-1);
                    temphist->SetTitle(Form("%s %s",temphist->GetTitle(),tempbg->GetTitle()));
                 }
                 temphist->Draw();
                 edit = true;
              }  
              while(GetNMarkers())
                 RemoveMarker();
              ClearBGMarkers();
            }
            break;
         case kKey_f:
            edit = PeakFitQ();
            break;
         case kKey_F:
            edit = PeakFit();
            break;
         case kKey_i:
            edit = Integrate();
            break;
         case kKey_I:
            edit = IntegrateBG();
            break;
         case kKey_s:
            edit = ShowPeaks(hists.data(),hists.size());
            break;
         case kKey_S:
            edit = RemovePeaks(hists.data(),hists.size());
            break;
         /*case kKey_S:
            if(fStatsDisplayed)
               fStatsDisplayed = false;
            else
               fStatsDisplayed = true;
            for(size_t i=0;i<hists.size();i++)
              hists.at(i)->SetStats(fStatsDisplayed);
            edit = true;
            break;
         */
         case kKey_F10:{
            std::ofstream outfile;
            for(int i=0;i<hists.back()->GetListOfFunctions()->GetSize();i++) {
               //printf("\n\n%s | %s\n",hist->GetListOfFunctions()->At(i)->IsA()->GetName(),static_cast<TF1*>(hist->GetListOfFunctions()->At(i))->GetName());
               if(hists.back()->GetListOfFunctions()->At(i)->InheritsFrom("TPeak")) {
                  if(!outfile.is_open())
                     outfile.open(Form("%s.fits",hists.back()->GetName()));
                  outfile << static_cast<TPeak*>(hists.back()->GetListOfFunctions()->At(i))->PrintString();
                  outfile << "\n\n";
               }     
            } 
            if(!outfile.is_open())
               outfile.close();
         }    
         break;

      };
   }
   if(ge){
      switch(*keysym) {
         case kKey_p:
            ge->Print();
            break;
      };
   }


   if(edit) {
      gPad->Modified();
      gPad->Update();
   }
   return true;
}


bool GCanvas::HandleMousePress(Int_t event,Int_t x,Int_t y) {
  //printf("Mouse clicked  %i   %i\n",x,y);
  if(!GetSelected())
    return false;
  if(GetSelected()->InheritsFrom("TCanvas"))
     static_cast<TCanvas*>(GetSelected())->cd();

  TIter iter(gPad->GetListOfPrimitives());
  TH1* hist = 0;
  while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        hist = static_cast<TH1*>(obj);
     }
  }
  if(!hist)
     return false;

  bool used = false;

  if(!strcmp(GetSelected()->GetName(),"TFrame") && fMarkerMode) {
    //((TFrame*)GetSelected())->SetBit(TBox::kCannotMove);
    //if(GetNMarkers()==4)
    //   RemoveMarker();
    AddMarker(x,y);
    //int px = gPad->AbsPixeltoX(x);
    //TLine* line = new TLine(px,GetUymin(),px,GetUymax());
    //line->Draw();
    used = true;
  }

  if(used) {
    gPad->Modified();
    gPad->Update();
  }
  return used;
}


TF1* GCanvas::GetLastFit() { 
  TH1* hist = 0;
  TIter iter(gPad->GetListOfPrimitives());
  while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        hist = static_cast<TH1*>(obj);
     }
  }
  if(!hist)
     return 0;
  if(hist->GetListOfFunctions()->GetSize()>0){
     TF1* tmpfit = static_cast<TF1*>(hist->GetListOfFunctions()->Last());
     std::string tmpname = tmpfit->GetName();
     while(tmpname.find("background") != std::string::npos ){
         tmpfit = static_cast<TF1*>(hist->GetListOfFunctions()->Before(tmpfit));
         tmpname = tmpfit->GetName();
     }
     return tmpfit; 
  }
  return 0;
}


bool GCanvas::SetLinearBG(GMarker* m1,GMarker* m2) {
  TIter iter(gPad->GetListOfPrimitives());
  TH1* hist = 0;
  while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        hist = static_cast<TH1*>(obj);
     }
  }
  if(!hist)
     return false;
  if(!m1 || !m2) {
    if(GetNMarkers()<2) {
       return false;
    } else { 
       m1 = fMarkers.at(fMarkers.size()-1);
       m2 = fMarkers.at(fMarkers.size()-2);
    }
  }
  TF1* bg = hist->GetFunction("linbg");
  if(bg)
     bg->Delete();
  double x[2];
  if(m1->fLocalX < m2->fLocalX) {
    x[0]=m1->fLocalX; x[1]=m2->fLocalX;
  } else {
    x[1]=m1->fLocalX; x[0]=m2->fLocalX;
  }
  printf("x[0] = %.02f   x[1] = %.02f\n",x[0],x[1]);
  bg = new TF1("linbg","pol1",x[0],x[1]);
  hist->Fit(bg,"QR+");
  bg->SetRange(gPad->GetUxmin(),gPad->GetUxmax());
  bg->Draw("SAME");
  hist->GetListOfFunctions()->Add(bg);
  //bg->Draw("same");
  return true;
}

bool GCanvas::GausBGFit(GMarker* m1,GMarker* m2) {
  TIter iter(gPad->GetListOfPrimitives());
  TH1* hist = 0;
  while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        hist = static_cast<TH1*>(obj);
     }
  }
  if(!hist)
     return false;
  if(!m1 || !m2) {
    if(GetNMarkers()<2) {
       return false;
    } else { 
       m1 = fMarkers.at(fMarkers.size()-1);
       m2 = fMarkers.at(fMarkers.size()-2);
    }
  }
  
  TF1* gausfit = hist->GetFunction("gausfit");
  if(gausfit)
     gausfit->Delete();
  double x[2];
  double y[2];
  if(m1->fLocalX < m2->fLocalX) {
    x[0]=m1->fLocalX; x[1]=m2->fLocalX;
    y[0]=hist->GetBinContent(m1->fX); y[1]=hist->GetBinContent(m2->fX); 
  } else {
    x[1]=m1->fLocalX; x[0]=m2->fLocalX;
    y[1]=hist->GetBinContent(m1->fX); y[0]=hist->GetBinContent(m2->fX); 
  }
  //printf("x[0] = %.02f   x[1] = %.02f\n",x[0],x[1]);
  gausfit = new TF1("gausfit","pol1(0)+gaus(2)",x[0],x[1]);
  TF1* gfit = new TF1("gaus","gaus",x[0],x[1]);
  hist->Fit(gfit,"QR+");

  gausfit->SetParameters(y[0],0,gfit->GetParameter(0),gfit->GetParameter(1),gfit->GetParameter(2));
  
  gfit->Delete();
  hist->GetFunction("gaus")->Delete();

  hist->Fit(gausfit,"QR+");
  TF1* bg = new TF1("bg","pol1",x[0],x[1]);
  bg->SetParameters(gausfit->GetParameter(0),gausfit->GetParameter(1));
  bg->Draw("same");
  hist->GetListOfFunctions()->Add(bg);
  
  double param[5];
  double error[5];
   
  gausfit->GetParameters(param);
  error[0] = gausfit->GetParError(0);
  error[1] = gausfit->GetParError(1);
  error[2] = gausfit->GetParError(2);
  error[3] = gausfit->GetParError(3);
  error[4] = gausfit->GetParError(4);
  
  printf("\nIntegral from % 4.01f to % 4.01f: %f\n",x[0],x[1],gausfit->Integral(x[0],x[1])/hist->GetBinWidth(1));
  printf("Centroid:  % 4.02f  +/- %.02f\n",param[3],error[3]);
  printf("FWHM:      % 4.02f  +/- %.02f\n",fabs(param[4]*2.35),error[4]*2.35);
  double integral = gausfit->Integral(x[0],x[1])/hist->GetBinWidth(1);
  double int_err  = integral*TMath::Sqrt(((error[2]/param[2])*(error[2]/param[2]))+
                                         ((error[4]/param[4])*(error[4]/param[4])));
  printf("Area:      % 4.02f  +/- %.02f\n",
         integral - (bg->Integral(x[0],x[1])/hist->GetBinWidth(1)),int_err);
  return true;
  
}

bool GCanvas::Integrate(GMarker* m1, GMarker* m2){
   TIter iter(gPad->GetListOfPrimitives());
   TH1* hist = 0;
   while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        hist = static_cast<TH1*>(obj);
     }
   }
   if(!hist)
      return false;

   if(!m1 || !m2) {
      if(GetNMarkers()<2) {
         return false;
      } else { 
         m1 = fMarkers.at(fMarkers.size()-1);
         m2 = fMarkers.at(fMarkers.size()-2);
      }
  }
   Double_t low_x,high_x;
   low_x = m1->fLocalX;
   high_x = m2->fLocalX;
   if(m1->fLocalX < m2->fLocalX){
      low_x = m1->fLocalX;
      high_x = m2->fLocalX;
   }
   else{
      low_x = m2->fLocalX;
      high_x = m1->fLocalX;
   }
   Int_t low_bin = hist->FindBin(low_x);
   Int_t high_bin = hist->FindBin(high_x);

   printf("Integral: %lf\n",hist->Integral(low_bin,high_bin));
   return true;

}

bool GCanvas::IntegrateBG(GMarker* m1, GMarker* m2){
   TIter iter(gPad->GetListOfPrimitives());
   TH1* hist = 0;
   while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        hist = static_cast<TH1*>(obj);
     }
   }
   if(!hist)
      return false;

   if(!m1 || !m2) {
      if(GetNMarkers()<2) {
         return false;
      } else { 
         m1 = fMarkers.at(fMarkers.size()-1);
         m2 = fMarkers.at(fMarkers.size()-2);
      }
   }

   Double_t low_counts, high_counts;
   Double_t low_x,high_x;
   low_x = m1->fLocalX;
   high_x = m2->fLocalX;
   if(m1->fLocalX < m2->fLocalX){
      low_x = m1->fLocalX;
      high_x = m2->fLocalX;
   }
   else{
      low_x = m2->fLocalX;
      high_x = m1->fLocalX;
   }
   Int_t low_bin = hist->FindBin(low_x);
   Int_t high_bin = hist->FindBin(high_x);
   TF1* background = new TF1("background","pol1",low_x,high_x);
   low_counts = hist->GetBinContent(low_bin);
   high_counts = hist->GetBinContent(high_bin);
   
   background->SetParameter(1,(high_counts - low_counts)/(high_x - low_x));
   background->SetParameter(0,high_counts - background->GetParameter(1)*high_x);

   background->Draw("same");
   Double_t integral = hist->Integral(low_bin,high_bin);
   Double_t bglevel = 0.5*background->GetParameter(1)*(TMath::Power(high_x,2.) - TMath::Power(low_x,2.)) + background->GetParameter(0)*(high_x - low_x) ;
   bglevel /= hist->GetBinWidth(high_x);
   printf("Total Counts: %lf\n",integral);
   printf("   BG Counts: %lf\n",bglevel);
   printf("    Integral: %lf\n",integral - bglevel);
   return true;

}

bool GCanvas::GausFit(GMarker* m1,GMarker* m2) {
  TIter iter(gPad->GetListOfPrimitives());
  TH1* hist = 0;
  while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        hist = static_cast<TH1*>(obj);
     }
  }
  if(!hist)
     return false;
  if(!m1 || !m2) {
    if(GetNMarkers()<2) {
       return false;
    } else { 
       m1 = fMarkers.at(fMarkers.size()-1);
       m2 = fMarkers.at(fMarkers.size()-2);
    }
  }
  
  TF1* gausfit = hist->GetFunction("gausfit");
  if(gausfit)
     gausfit->Delete();
  double x[2];
  if(m1->fLocalX < m2->fLocalX) {
    x[0]=m1->fLocalX; x[1]=m2->fLocalX;
  } else {
    x[1]=m1->fLocalX; x[0]=m2->fLocalX;
  }
  //printf("x[0] = %.02f   x[1] = %.02f\n",x[0],x[1]);
  gausfit = new TF1("gausfit","gaus",x[0],x[1]);
//  TF1* gfit = new TF1("gaus","gaus",x[0],x[1]);
//  hist->Fit(gfit,"QR+");

  ///gausfit->SetParameters(y[0],0,gfit->GetParameter(0),gfit->GetParameter(1),gfit->GetParameter(2));
  
//  gfit->Delete();
  //hist->GetFunction("gaus")->Delete();

  hist->Fit(gausfit,"QR+");
  
  double param[3];
  double error[3];
   
  gausfit->GetParameters(param);
  error[0] = gausfit->GetParError(0);
  error[1] = gausfit->GetParError(1);
  error[2] = gausfit->GetParError(2);
  
  printf("\nIntegral from % 4.01f to % 4.01f: %f\n",x[0],x[1],gausfit->Integral(x[0],x[1])/hist->GetBinWidth(1));
  printf("Centroid:  % 4.02f  +/- %.02f\n",param[1],error[1]);
  printf("FWHM:      % 4.02f  +/- %.02f\n",param[2]*2.35,error[2]*2.35);
  double integral = gausfit->Integral(x[0],x[1])/hist->GetBinWidth(1);
  double int_err  = integral*TMath::Sqrt((error[0]/param[0])*(error[0]/param[0]) +
                                         ((error[2]/param[2])*(error[2]/param[2])));
  printf("Area:      % 4.02f  +/- %.02f\n",
         integral,int_err);
  return true;
  
}

bool GCanvas::PeakFit(GMarker* m1,GMarker* m2) {
  TIter iter(gPad->GetListOfPrimitives());
  TH1* hist = 0;
  while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        hist = static_cast<TH1*>(obj); 
     }
  }
  if(!hist)
     return false;
  if(!m1 || !m2) {
    if(GetNMarkers()<2) {
       return false;
    } else { 
       m1 = fMarkers.at(fMarkers.size()-1);
       m2 = fMarkers.at(fMarkers.size()-2);
    }
  }
  
 // TPeak* mypeak = (TPeak*)(hist->GetFunction("peak"));
 // if(mypeak)
  //   mypeak->Delete();
  double x[2];
  if(m1->fLocalX < m2->fLocalX) {
    x[0]=m1->fLocalX; x[1]=m2->fLocalX;
  } else {
    x[1]=m1->fLocalX; x[0]=m2->fLocalX;
  }
  //printf("x[0] = %.02f   x[1] = %.02f\n",x[0],x[1]);
  TPeak* mypeak = new TPeak((x[0]+x[1])/2.0,x[0],x[1]);
//  TF1* gfit = new TF1("gaus","gaus",x[0],x[1]);
//  hist->Fit(gfit,"QR+");

  ///gausfit->SetParameters(y[0],0,gfit->GetParameter(0),gfit->GetParameter(1),gfit->GetParameter(2));
  
//  gfit->Delete();
  //hist->GetFunction("gaus")->Delete();

  mypeak->Fit(hist,"+");
  hist->GetListOfFunctions()->Add(mypeak->Background()->Clone());
 // hist->GetListOfFunctions()->Add(mypeak);
  //TPeak* peakfit = (TPeak*)(hist->GetListOfFunctions()->Last());
//  mypeak->Background()->Draw("SAME");
  /*
  double param[3];
  double error[3];
   
  gausfit->GetParameters(param);
  error[0] = gausfit->GetParError(0);
  error[1] = gausfit->GetParError(1);
  error[2] = gausfit->GetParError(2);
  
  printf("\nIntegral from % 4.01f to % 4.01f: %f\n",x[0],x[1],gausfit->Integral(x[0],x[1])/hist->GetBinWidth(1));
  printf("Centroid:  % 4.02f  +/- %.02f\n",param[1],error[1]);
  printf("FWHM:      % 4.02f  +/- %.02f\n",param[2]*2.35,error[2]*2.35);
  double integral = gausfit->Integral(x[0],x[1])/hist->GetBinWidth(1);
  double int_err  = integral*TMath::Sqrt((error[0]/param[0])*(error[0]/param[0]) +
                                         ((error[2]/param[2])*(error[2]/param[2])));
  printf("Area:      % 4.02f  +/- %.02f\n",
         integral,int_err);*/
  return true;
  
}


bool GCanvas::PeakFitQ(GMarker* m1,GMarker* m2) {
  TIter iter(gPad->GetListOfPrimitives());
  TH1* hist = 0;
  while(TObject* obj = iter.Next()) {
     if( obj->InheritsFrom("TH1") &&
        !obj->InheritsFrom("TH2") &&  
        !obj->InheritsFrom("TH3") ) {  
        hist = static_cast<TH1*>(obj); 
     }
  }
  if(!hist)
     return false;
  if(!m1 || !m2) {
    if(GetNMarkers()<2) {
       return false;
    } else { 
       m1 = fMarkers.at(fMarkers.size()-1);
       m2 = fMarkers.at(fMarkers.size()-2);
    }
  }
  
  double x[2];
  if(m1->fLocalX < m2->fLocalX) {
    x[0]=m1->fLocalX; x[1]=m2->fLocalX;
  } else {
    x[1]=m1->fLocalX; x[0]=m2->fLocalX;
  }
  //printf("x[0] = %.02f   x[1] = %.02f\n",x[0],x[1]);
  TPeak*  mypeak = new TPeak((x[0]+x[1])/2.0,x[0],x[1]);
/*  if(hist->FindObject(mypeak->GetName())){
     //delete mypeak;
     mypeak = (TPeak*)(hist->FindObject(mypeak->GetName()));
  }*/
//  TF1* gfit = new TF1("gaus","gaus",x[0],x[1]);
//  hist->Fit(gfit,"QR+");

  ///gausfit->SetParameters(y[0],0,gfit->GetParameter(0),gfit->GetParameter(1),gfit->GetParameter(2));
  
//  gfit->Delete();
  //hist->GetFunction("gaus")->Delete();

  mypeak->Fit(hist,"Q+");
  hist->GetListOfFunctions()->Add(mypeak->Background()->Clone());
 // hist->GetListOfFunctions()->Add(mypeak);
  TPeak* peakfit = static_cast<TPeak*>(hist->GetListOfFunctions()->Last());
  //hist->GetListOfFunctions()->Print();
  if(!peakfit) {
    printf("peakfit not found??\n");
    return false;
  }
//  mypeak->Background()->Draw("SAME");
  mypeak->Print();
  
     
/* 
  double param[10];
  double error[10];
  peakfit->GetParameters(param);
  error[0] = peakfit->GetParError(0);
  error[1] = peakfit->GetParError(1);
  error[2] = peakfit->GetParError(2);
  error[3] = peakfit->GetParError(3);
  error[4] = peakfit->GetParError(4);
  error[4] = peakfit->GetParError(5);
  error[4] = peakfit->GetParError(6);
  error[4] = peakfit->GetParError(7);
  error[4] = peakfit->GetParError(8);
  error[4] = peakfit->GetParError(9);
  
  printf("\nIntegral from % 4.01f to % 4.01f: %f\n",x[0],x[1],peakfit->Integral(x[0],x[1])/hist->GetBinWidth(1));
  printf("Centroid:  % 4.02f  +/- %.02f\n",param[1],error[1]);
  printf("FWHM:      % 4.02f  +/- %.02f\n",fabs(param[2]*2.35),error[2]*2.35);
 // double integral = gausfit->Integral(x[0],x[1])/hist->GetBinWidth(1);
 // double int_err  = integral*TMath::Sqrt(((error[2]/param[2])*(error[2]/param[2]))+
 //                                        ((error[4]/param[4])*(error[4]/param[4])));
 // printf("Area:      % 4.02f  +/- %.02f\n",
 //        integral - (bg->Integral(x[0],x[1])/hist->GetBinWidth(1)),int_err);
 */ 
  return true;
  
}


void GCanvas::SetBackGroundSubtractionType() {
  /// used to set the background subtraction type
  /// used for the p command. Current configurations 
  /// are:
  ///
  /// 0.  No background subtraction.
  /// 1.  Fraction of the total projection. setting a bg level estimates the fraction.
  /// 2.  From marker #3         -> make a subtract gate the same width as the project gate.
  /// 3.  From marker #3 & #4    -> make a suntract gate from maker 3 and 4 the same total widthe as the project gate. Odd numebrs default to marker #4.
  /// 4.  Between marker #3 & #4 -> make a subtract gate between marker 3 and 4. 
  /// 5.  Use marker #1 & #2     -> use the 'b' key to create a subtract projection.  Projection is not drawn but last projection made will be subtracted
  ///                               in the next projection.
  ///

  fBGSubtraction_type++;
  //if(fBGSubtraction_type >5)
  if(fBGSubtraction_type >4)
     fBGSubtraction_type = 0;
  printf("\n");
  switch(fBGSubtraction_type) {
    case 0:
     printf("BG subtraction off, project will not automatically subtract background.\n");
     break;
    case 1:
     printf("BG subtraction set to fraction of total projection, use b to set fraction.\n");
     break;
    case 2:
     printf("BG subtraction set to ( marker3->| ), use b to confirm subtraction gate.\n");
     break;
    case 3:
     printf("BG subtraction set to ( marker3->| ) & ( marker4->| ), use b to confirm subtraction gates.\n");
     break;
    case 4:
     printf("BG subtraction set to ( marker1->marker2 ), use b to confirm subtraction gates.\n");
     break;
    default:
     printf("Changing BG subtraction type, type is now: %i\n",fBGSubtraction_type);
  };
  Prompt();
  return;
}

bool GCanvas::SetBackGround(GMarker* m1,GMarker* m2,GMarker* m3,GMarker* m4) {
  ClearBGMarkers();  //removes all BG markers... 
  bool edit = false;
  switch(fBGSubtraction_type) {   
    case 0:
      printf(RED "\nBackground Subtraction type not set, no Background subtraction will be performed.\n" RESET_COLOR );
      break;
    case 1:
      //if(!m1) {
      if(fMarkers.size()<1) {
        printf(RED "\nPlace at least one marker to set background level.\n" RESET_COLOR );
        break;
      } else if(fMarkers.size()<2) {
         AddBGMarker(fMarkers.at(fMarkers.size()-1));
         RemoveMarker();
      } else {
         AddBGMarker(fMarkers.at(fMarkers.size()-1));
         AddBGMarker(fMarkers.at(fMarkers.size()-2));
         RemoveMarker();
         RemoveMarker();
      }

      edit = SetConstantBG();
      break;
    case 2:
      //printf(RED "\nWork in progress, check back soon; no Background subtraction will be performed.\n" RESET_COLOR );
      if(!m3) {
        printf(RED "\nThree markers need.  First two peak, three for bg.\n" RESET_COLOR );
        Prompt();
        break;
      }
      edit = SetBGGate(m1,m2,m3,0);
      break;
    case 3:
      if(!m3 || !m4) {
        printf(RED "\nFour markers need.  First two peak, three and four for split bg.\n" RESET_COLOR );
        Prompt();
        break;
      }
      edit = SetBGGate(m1,m2,m3,m4);
      break;
    case 4:
      if(!m3 || !m4) {
        printf(RED "\nTwo markers need.  BG gate between marker1 and marker2.\n" RESET_COLOR );
        Prompt();
        break;
      }
      edit = SetBGGate(m3,m4);
      //printf(RED "\nWork in progress, check back soon; no Background subtraction will be performed.\n" RESET_COLOR );
      break;
    case 5:
      printf(RED "\nWork in progress, check back soon; no Background subtraction will be performed.\n" RESET_COLOR );
      break;
  };
  return edit;
}

bool GCanvas::SetBGGate(GMarker* m1, GMarker* m2, GMarker* m3, GMarker* m4) {
  ClearBGMarkers();
  switch(fBGSubtraction_type) {   
    case 2:
      if(!m1 || !m2 || !m3)
         return false;
      else {
        AddBGMarker(m3);
        
        GMarker* mark = new GMarker(*m3);
        mark->fX = m3->fX + (abs(m1->fX - m2->fX)+1);
        mark->fLocalX = gPad->AbsPixeltoX(mark->fX);
        AddBGMarker(mark);
        
        mark = fBGMarkers.at(0);
        mark->fLineX = new TLine(mark->fLocalX,GetUymin(),mark->fLocalX,GetUymax());
        mark->fLineX->SetLineColor(kBlue);
        mark->fLineX->Draw();
        
        mark = fBGMarkers.at(1);
        mark->fLineX = new TLine(mark->fLocalX,GetUymin(),mark->fLocalX,GetUymax());
        mark->fLineX->SetLineColor(kBlue);
        mark->fLineX->Draw();

        RemoveMarker(); // remove marker #3 so the project will work...
      }
      return true;
   case 3:
     if(!m1 || !m2 || !m3 || !m4)
        return false;
     else {
        AddBGMarker(m3);

        GMarker* mark = new GMarker(*m3);
        if((abs(m1->fX - m2->fX)%2) != 0)
          mark->fX = m3->fX + ((abs(m1->fX - m2->fX)+1)/2 + 1 );
        else 
          mark->fX = m3->fX + (abs(m1->fX - m2->fX)/2 + 1);
        mark->fLocalX = gPad->AbsPixeltoX(mark->fX);
        AddBGMarker(mark);

        AddBGMarker(m4);
        mark = new GMarker(*m3);
        mark->fX = m4->fX + (abs(m1->fX - m2->fX)/2 + 1);
        mark->fLocalX = gPad->AbsPixeltoX(mark->fX);
        AddBGMarker(mark);

        for(int x=0;x<4;x++) {
           mark = fBGMarkers.at(x);
           mark->fLineX = new TLine(mark->fLocalX,GetUymin(),mark->fLocalX,GetUymax());
           mark->fLineX->SetLineColor(kBlue);
           mark->fLineX->Draw();
        } 
        RemoveMarker(); // remove marker #4 so the project will work...
        RemoveMarker(); // remove marker #3 so the project will work...
     }
     return true;
   case 4:
     if(!m1 || !m2 )
        return false;
     else {
       AddBGMarker(m1);
       AddBGMarker(m2);
       for(int x=0;x<2;x++) {
         GMarker* mark = fBGMarkers.at(x);
         mark->fLineX = new TLine(mark->fLocalX,GetUymin(),mark->fLocalX,GetUymax());
         mark->fLineX->SetLineColor(kBlue);
         mark->fLineX->Draw();
       } 
       RemoveMarker(); // remove marker #4 so the project will work...
       RemoveMarker(); // remove marker #3 so the project will work...
     }  
     return true;
     default:
        return false;
  };
}

bool GCanvas::SetConstantBG() {
  bool edit = false;
  std::vector<TH1*> hists = Find1DHists();
  if(hists.size()<1)
     return edit;
  if(GetNBG_Markers()<1)
     return edit;
  OrderBGMarkers();
  TF1* const_bg = hists.at(0)->GetFunction("const_bg");
  if(const_bg)
     const_bg->Delete();
  double x[2];
  if(GetNBG_Markers()==1) {
    x[0]=fBGMarkers.at(0)->fLocalX; x[1]=fBGMarkers.at(0)->fLocalX;
  } else {
    x[0]=fBGMarkers.at(0)->fLocalX; x[1]=fBGMarkers.at(1)->fLocalX;
  }
  const_bg = new TF1("const_bg","pol0",x[0],x[1]);
  hists.at(0)->Fit(const_bg,"QR+");
  TAxis* xaxis = hists.at(0)->GetXaxis();
  const_bg->SetRange(xaxis->GetFirst(),xaxis->GetLast());
  const_bg->Draw("SAME");
  hists.at(0)->GetListOfFunctions()->Add(const_bg);
  edit = true;
  return edit;

}

TH1* GCanvas::GetBackGroundHist(GMarker* addlow,GMarker* addhigh) {
  std::vector<TH1*> hists = Find1DHists();
  if(hists.size()<1)
     return 0;
  TH1* hist = hists.at(0);

  switch(fBGSubtraction_type) {   
    case 0:
      //printf(RED "\nBackground Subtraction type not set, no Background subtraction will be performed.\n" RESET_COLOR );
      return 0;
    case 1: {
        // check that bg was been set:
        TF1* const_bg = hist->GetFunction("const_bg");
        if(!const_bg) // not yet set.
           return 0;
        Double_t pj_total = hist->Integral(0,hist->GetNbinsX(),"width");
        if(pj_total<1)
           return 0;
        Double_t bg_frac  = (addhigh->fLocalX-addlow->fLocalX +1)*const_bg->GetParameter(0)/pj_total;
        //GMemObj = *mobj = GRootObjectManager::Instance()->FindObject(hist->GetName());
        //if(!mobj || !mobj->GetParent() || !mobj->GetParent()->InheritsFrom("TH2"))
        //   return 0;
        TH1* temp  = static_cast<TH1*>(hist->Clone(Form("%s_bg",hist->GetName())));
        temp ->SetTitle(Form(" - bg(frac %0.4f)",bg_frac));
        temp->Scale(bg_frac);
        return temp;
      }
    case 2: {
      TH1* temp_bg =0;
      if(GetNBG_Markers()<2)
         return temp_bg;
      OrderBGMarkers();
      GMemObj* mobj = GRootObjectManager::Instance()->FindMemObject(hist->GetName());
      if(!mobj || !mobj->GetParent() || !mobj->GetParent()->InheritsFrom("TH2"))
         return temp_bg;
      int bin0,bin1;
      if(!strcmp(mobj->GetOption(),"ProjY")) { 
        bin1 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(fBGMarkers.size()-1)->fLocalX);
        bin0 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(fBGMarkers.size()-2)->fLocalX);
        temp_bg = static_cast<TH2*>(mobj->GetParent())->ProjectionX(Form("%s_bg",hist->GetName()),bin0,bin1);
      } else {
        bin1 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(fBGMarkers.size()-1)->fLocalX);
        bin0 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(fBGMarkers.size()-2)->fLocalX);
        temp_bg = static_cast<TH2*>(mobj->GetParent())->ProjectionY(Form("%s_bg",hist->GetName()),bin0,bin1);
      }
      temp_bg->SetTitle(Form(" - bg(%.0f to %.0f)",fBGMarkers.at(0)->fLocalX,fBGMarkers.at(1)->fLocalX));
      return temp_bg;
      }
      //printf(RED "\nWork in progress, check back soon; no Background subtraction will be performed.\n" RESET_COLOR );
    case 3: {
      TH1* temp_bg  =0;
      TH1* temp_bg1 =0;
      if(GetNBG_Markers()<4)
         return temp_bg;
      OrderBGMarkers();
      GMemObj* mobj = GRootObjectManager::Instance()->FindMemObject(hist->GetName());
      if(!mobj || !mobj->GetParent() || !mobj->GetParent()->InheritsFrom("TH2"))
         return temp_bg;
      int bin0,bin1;
      if(!strcmp(mobj->GetOption(),"ProjY")) { 
        bin1 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(0)->fLocalX);
        bin0 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(1)->fLocalX);
        temp_bg = static_cast<TH2*>(mobj->GetParent())->ProjectionX(Form("%s_bg",hist->GetName()),bin0,bin1);
        bin1 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(2)->fLocalX);
        bin0 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(3)->fLocalX);
        temp_bg1 = static_cast<TH2*>(mobj->GetParent())->ProjectionX(Form("%s_bg",hist->GetName()),bin0,bin1);
      } else {
        bin1 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(0)->fLocalX);
        bin0 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(1)->fLocalX);
        temp_bg = static_cast<TH2*>(mobj->GetParent())->ProjectionY(Form("%s_bg",hist->GetName()),bin0,bin1);
        bin1 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(2)->fLocalX);
        bin0 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(3)->fLocalX);
        temp_bg1 = static_cast<TH2*>(mobj->GetParent())->ProjectionY(Form("%s_bg",hist->GetName()),bin0,bin1);
      }
      temp_bg->Add(temp_bg1,1);
      temp_bg->SetTitle(Form(" - bg(%.0f to %.0f and %.0f to %.0f)",fBGMarkers.at(0)->fLocalX,fBGMarkers.at(1)->fLocalX,
                                                                    fBGMarkers.at(2)->fLocalX,fBGMarkers.at(3)->fLocalX));
      return temp_bg;
      }
      //printf(RED "\nWork in progress, check back soon; no Background subtraction will be performed.\n" RESET_COLOR );
    case 4: {
      TH1* temp_bg  =0;
      if(GetNBG_Markers()<2)
         return temp_bg;
      OrderBGMarkers();
      GMemObj* mobj = GRootObjectManager::Instance()->FindMemObject(hist->GetName());
      if(!mobj || !mobj->GetParent() || !mobj->GetParent()->InheritsFrom("TH2"))
         return temp_bg;
      int bin0,bin1;
      if(!strcmp(mobj->GetOption(),"ProjY")) { 
        bin1 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(0)->fLocalX);
        bin0 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(1)->fLocalX);
        temp_bg = static_cast<TH2*>(mobj->GetParent())->ProjectionX(Form("%s_bg",hist->GetName()),bin0,bin1);
      } else {
        bin1 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(0)->fLocalX);
        bin0 = static_cast<TH2*>(mobj->GetParent())->GetXaxis()->FindBin(fBGMarkers.at(1)->fLocalX);
        temp_bg = static_cast<TH2*>(mobj->GetParent())->ProjectionY(Form("%s_bg",hist->GetName()),bin0,bin1);
      }
      temp_bg->SetTitle(Form(" - bg(%.0f to %.0f)",fBGMarkers.at(0)->fLocalX,fBGMarkers.at(1)->fLocalX));
      return temp_bg;
      }
      //printf(RED "\nWork in progress, check back soon; no Background subtraction will be performed.\n" RESET_COLOR );
    case 5:
      printf(RED "\nWork in progress, check back soon; no Background subtraction will be performed.\n" RESET_COLOR );
      break;
      
  };
  return 0;
}
