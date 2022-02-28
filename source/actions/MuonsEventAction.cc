// ----------------------------------------------------------------------------
// nexus | MuonsEventAction.cc
//
// This class is based on DefaultEventAction and modified to produce
// control histograms for muon generation.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "MuonsEventAction.h"
#include "Trajectory.h"
#include "PersistencyManager.h"
#include "IonizationHit.h"
#include "FactoryBase.h"

#include <G4Event.hh>
#include <G4VVisManager.hh>
#include <G4Trajectory.hh>
#include <G4GenericMessenger.hh>
#include <G4HCofThisEvent.hh>
#include <G4SDManager.hh>
#include <G4HCtable.hh>
#include <globals.hh>
#include "g4root_defs.hh"


#include "CLHEP/Units/SystemOfUnits.h"
#include "AddUserInfoToPV.h"

using namespace CLHEP;
namespace nexus {

REGISTER_CLASS(MuonsEventAction, G4UserEventAction)

  MuonsEventAction::MuonsEventAction():
    G4UserEventAction(), nevt_(0), nupdate_(10), energy_threshold_(0.), stringHist_("")
  {
    msg_ = new G4GenericMessenger(this, "/Actions/MuonsEventAction/");
    msg_->DeclareProperty("stringHist", stringHist_, "");

    G4GenericMessenger::Command& thresh_cmd =
       msg_->DeclareProperty("energy_threshold", energy_threshold_,
                             "Minimum deposited energy to save the event to file.");
    thresh_cmd.SetParameterName("energy_threshold", true);
    thresh_cmd.SetUnitCategory("Energy");
    thresh_cmd.SetRange("energy_threshold>0.");


    // Get analysis manager
    fG4AnalysisMan = G4AnalysisManager::Instance();
    
    // Create histogram(s) for muons
    fG4AnalysisMan->CreateH1("Edepo","Energy_deposited",100,-1.0,3.4);
    fG4AnalysisMan->CreateH1("Theta","Theta generated",100,-pi,pi);
    fG4AnalysisMan->CreateH1("Phi","Phi generated",100,0.,twopi);

    // Open a CSV file to write the muon theta and phi events to
    // Currently G4 Ntuple Functionality does not work
    fThetaPhi.open ("Muon_Theta_Phi_Events.csv");
    fThetaPhi << "Theta" << "," << "Phi" << "\n";

  }



  MuonsEventAction::~MuonsEventAction()
  {
    // Open an output file and write histogram to file
    fG4AnalysisMan->OpenFile(stringHist_);
    fG4AnalysisMan->Write();
    fG4AnalysisMan->CloseFile();

    // Close the CSV file
    fThetaPhi.close();

  }

  void MuonsEventAction::BeginOfEventAction(const G4Event* /*event*/)
  {
   // Print out event number info
    if ((nevt_ % nupdate_) == 0) {
      G4cout << " >> Event no. " << nevt_  << G4endl;
      if (nevt_  == (10 * nupdate_)) nupdate_ *= 10;
    }
  }


  void MuonsEventAction::EndOfEventAction(const G4Event* event)
  {
    nevt_++;

    // Determine whether total energy deposit in ionization sensitive
    // detectors is above threshold
    if (energy_threshold_ >= 0.) {

      // Get the trajectories stored for this event and loop through them
      // to calculate the total energy deposit

      G4double edep = 0.;

      G4TrajectoryContainer* tc = event->GetTrajectoryContainer();
      if (tc) {
        for (unsigned int i=0; i<tc->size(); ++i) {
          Trajectory* trj = dynamic_cast<Trajectory*>((*tc)[i]);
          edep += trj->GetEnergyDeposit();
          // Draw tracks in visual mode
          if (G4VVisManager::GetConcreteInstance()) trj->DrawTrajectory();
        }
      }
      // Control plot for energy
      fG4AnalysisMan->FillH1(0, edep);

      PersistencyManager* pm = dynamic_cast<PersistencyManager*>
        (G4VPersistencyManager::GetPersistencyManager());

      if (edep > energy_threshold_) pm->StoreCurrentEvent(true);
      else pm->StoreCurrentEvent(false);

    }

    // Retrieving muon generation information
    G4PrimaryVertex* my_vertex = event->GetPrimaryVertex();
    G4VUserPrimaryVertexInformation *getinfo2 = my_vertex->GetUserInformation();
    AddUserInfoToPV *my_getinfo2 = dynamic_cast<AddUserInfoToPV*>(getinfo2);

    G4double my_theta = my_getinfo2->GetTheta();
    G4double my_phi = my_getinfo2->GetPhi();

    fG4AnalysisMan->FillH1(1, my_theta);
    fG4AnalysisMan->FillH1(2, my_phi);

    fThetaPhi << my_theta << "," << my_phi<< "\n"; 

  }


} // end namespace nexus
