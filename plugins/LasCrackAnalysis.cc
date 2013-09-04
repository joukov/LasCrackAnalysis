#include <FWCore/Framework/interface/MakerMacros.h>

#include <DataFormats/Common/interface/DetSetVector.h>

#include <DataFormats/SiStripCommon/interface/SiStripEventSummary.h>
#include <DataFormats/SiStripDigi/interface/SiStripDigi.h>
#include <DataFormats/SiStripDigi/interface/SiStripRawDigi.h>
#include <DataFormats/SiStripDigi/interface/SiStripProcessedRawDigi.h>
#include <FWCore/Framework/interface/EventSetup.h> 
#include <FWCore/Framework/interface/EventSetupRecord.h> 

//#include <Alignment/LaserAlignment/interface/LASGlobalLoop.h>

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TKey.h"

#include "LasCrackAnalysis.h"

///
/// constructors and destructor
///
LasCrackAnalysis::LasCrackAnalysis( const edm::ParameterSet& iConfig ) :
  theOutputFile(0),
  theOutputTree(0),
  latency(-1),
  eventnumber(-1),
  runnumber(-1),
  lumiBlock(-1)
  
{
  std::string analysis_type = iConfig.getParameter<std::string>( "AnalysisType" );
  if     (analysis_type == "Pedestals"   ) theAnalysisType = PEDESTALS;
  else if(analysis_type == "Coarse_Delay") theAnalysisType = COARSE_DELAY;
  else{
    edm::LogError("LasCrackAnalysis") << "Invalid analysis Type: " << analysis_type;
    throw std::runtime_error("No valid Analysis type was specified");
  }
  edm::LogInfo("LasCrackAnalysis") << "The Analysis Type is " << analysis_type;

  DigisModuleLabel = iConfig.getParameter<std::string>( "DigisModuleLabel" );
  DigisInstanceLabel = iConfig.getParameter<std::string>( "DigisInstanceLabel" );

  if      ( DigisInstanceLabel == "VirginRaw" )      theDigiType = VirginRaw;
  else if      ( DigisInstanceLabel == "ProcessedRaw" )   theDigiType = ProcessedRaw;
  else if ( DigisInstanceLabel == "ZeroSuppressed" ) theDigiType = ZeroSuppressed;
  else{
    edm::LogError("LasCrackAnalysis") << "Invalid DigisInstanceLabel: " << DigisInstanceLabel;
    throw std::runtime_error("No valid DigisInstanceLabel was specified");
  }

  //Open the output file
  std::string open_file_type = (theAnalysisType == PEDESTALS ? "RECREATE" : "UPDATE");
  theOutputFile = new TFile( iConfig.getUntrackedParameter<std::string>( "OutputFileName" ).c_str() , open_file_type.c_str() );
  theOutputFile->cd();
}


///
///
///
LasCrackAnalysis::~LasCrackAnalysis() {
}


///
///
///
void LasCrackAnalysis::beginJob() 
{  
  theOutputTree = new TTree( "lasRawDataTree", "lasRawDataTree" );
  theOutputTree->Branch( "latency", &latency, "latency/I" );
  theOutputTree->Branch( "eventnumber", &eventnumber, "eventnumber/I" );
  theOutputTree->Branch( "runnumber", &runnumber, "runnumber/I" );
  theOutputTree->Branch( "lumiblock", &lumiBlock, "lumiblock/I" );
  
  // Read in the existing pedestals
  if(theAnalysisType == COARSE_DELAY){

    theOutputFile->cd("Pedestals"); // Switch to Directory where pedestals should be (all stored in TH1D objects)

    TList * full_list = gDirectory->GetListOfKeys(); // Get a list of all entries in the directory
    if( !full_list) throw std::runtime_error("Tried to find pedestal histos and got no list");
    LogDebug("LasCrackAnalysis") << "The Pedestal List has " << full_list->GetEntries() << " entries";

    std::ostringstream debug_message;
    for(Int_t idx = 0; idx < full_list->GetEntries(); idx++){ // Loop over all entries
      TH1D *hist = dynamic_cast<TH1D*>( ((TKey*)( full_list->At(idx) ))->ReadObj()); // Read in the objects from the file

      // The name of the Histogram is the DetId
      edm::det_id_type id;
      std::istringstream inp(hist->GetName());
      inp >> id;

      std::vector<double>& buffer = pedmap[id]; // Get a reference to the pedestal buffer
      for(Int_t idx = 1; idx <= hist->GetEntries(); idx++) buffer.push_back(hist->GetBinContent(idx)); // Copy all entries from histogram to buffer
      debug_message << hist->ClassName() << " / " << hist->GetName() << " / " << id << " / " << hist->GetEntries() << "\n";
    }
    LogDebug("LasCrackAnalysis") << "ClassName  /  Name  /  Id  /  Entries\n" << debug_message.str();

    if(pedmap.empty()){
      edm::LogError("LasCrackAnalysis") << "Did not find any pedestals!";
    }
    else edm::LogInfo("LasCrackAnalysis") << "Loaded " << pedmap.size() << " pedestals!";
  }
}



///
///
///
void LasCrackAnalysis::beginRun(edm::Run const & theRun, edm::EventSetup const & theEventSetup) 
{
}


///
///
///
void LasCrackAnalysis::analyze( const edm::Event& iEvent, const edm::EventSetup& iSetup )
{

  // Retrieve SiStripEventSummary produced by the digitizer
  edm::Handle<SiStripEventSummary> summary;
  iEvent.getByLabel( "siStripDigis", summary );
  latency = static_cast<int32_t>( summary->latency() );
  eventnumber = iEvent.id().event();
  runnumber = iEvent.run();
  lumiBlock = iEvent.luminosityBlock();

  // Get the Digis as defined by theDigiType
  // Currently only VirginRaw is implemented properly
  switch( theDigiType ){
  case ZeroSuppressed:
    //AnalyzeZeroSuppressed( iEvent );
    throw std::runtime_error("LasCrackAnalysis is not yet able to process ZeroSuppressed Data");
    break;
  case VirginRaw:
    AnalyzeVirginRaw( iEvent );
    //throw std::runtime_error("LasCrackAnalysis is not yet able to process VirginRaw Data");
    break;
  case ProcessedRaw:
    throw std::runtime_error("LasCrackAnalysis is not yet able to process ProcessedRaw Data");
    break;
  default:
    throw std::runtime_error("No valid Digi type");
  }

  // Push Container into the Tree
  theOutputTree->Fill();

  return;
}




// Copy the Digis into the local container (theData)
// Currently this has only been implemented and tested for SiStripDigis */
// SiStripRawDigis and SiStripProcessedRawDigis will need some changes to work (this is the final goal) */
void LasCrackAnalysis::AnalyzeZeroSuppressed( const edm::Event& iEvent)
{
  LogDebug("LasCrackAnalysis") << "Fill ZeroSuppressed Digis into the Tree";

  // Get the DetSetVector for the SiStripDigis
  // This is a vector with all the modules, each module containing zero or more strips with signal (Digis)
  edm::Handle< edm::DetSetVector< SiStripDigi > > detSetVector;  // Handle for holding the DetSetVector
  iEvent.getByLabel( DigisModuleLabel , DigisInstanceLabel , detSetVector );
  if( ! detSetVector.isValid() ) {
    edm::LogError("LasCrackAnalysis") << "Could not find Digis for Module " << DigisModuleLabel << " and Instance " << DigisInstanceLabel;;
    throw std::runtime_error("Could not find the Digis");
  }
  edm::LogInfo("LasCrackAnalysis") << "detSetVector has size: " << detSetVector->size();
}


double sub_ped(const SiStripRawDigi& digi, const double& data){ return data - digi.adc(); }
double add_digis(const SiStripRawDigi& digi, const double& sum){ return sum + digi.adc(); }
double copy_digi(const SiStripRawDigi& digi){ return digi.adc(); }
void LasCrackAnalysis::AnalyzeVirginRaw( const edm::Event& iEvent)
{
  // Get the DetSetVector for the SiStripDigis
  // This is a vector with all the modules, each module containing zero or more strips with signal (Digis)
  edm::Handle< edm::DetSetVector< SiStripRawDigi > > detSetVector;  // Handle for holding the DetSetVector
  iEvent.getByLabel( DigisModuleLabel , DigisInstanceLabel , detSetVector );
  if( ! detSetVector.isValid() ) {
    edm::LogError("LasCrackAnalysis") << "Could not find Digis for Module " << DigisModuleLabel << " and Instance " << DigisInstanceLabel;;
    throw std::runtime_error("Could not find the Digis");
  }
  for(edm::DetSetVector< SiStripRawDigi >::const_iterator it = detSetVector->begin(); it != detSetVector->end(); it++){
    std::vector<double>& buffer = pedmap[it->detId()];
    switch(theAnalysisType){
    case PEDESTALS:{
      if(buffer.empty()){
	std::transform(it->begin(), it->end(), std::back_insert_iterator<std::vector<double> >(buffer), copy_digi);
	norm[it->detId()] = 1;
      }
      else{
	std::transform(it->begin(), it->end(), buffer.begin(), buffer.begin(), add_digis);
	norm[it->detId()]++;
      }
      break;
    }
    case COARSE_DELAY:{
      if(buffer.empty()){
	edm::LogError("LasCrackAnalysis") << "Empty pedestal for DetId " << it->detId() << " (Loaded " << pedmap.size() << " pedestals!)";
	throw std::runtime_error("Trying to use an empty pedestal");
      }
      std::vector<double> pedestal_subtracted;
      std::transform(it->begin(), it->end(), buffer.begin(), std::back_insert_iterator<std::vector<double> >(pedestal_subtracted), sub_ped);
      double max_signal = *(std::max_element(pedestal_subtracted.begin(), pedestal_subtracted.end()));
      signalmap[it->detId()].push_back( max_signal );
      break;
    }
    default:
      throw std::runtime_error("This analysis type is not yet fully implemented");
    }
  }
}


///
///
///
void LasCrackAnalysis::endJob()
{
  theOutputFile->cd();

  if( theAnalysisType == PEDESTALS ){
    theOutputFile->mkdir("Pedestals");
    theOutputFile->cd("Pedestals");
    
    std::map<edm::det_id_type, std::vector<double> >::iterator it;
    for(it = pedmap.begin(); it != pedmap.end(); it++){
      edm::det_id_type id = it->first;
      if( norm[id] != 0){
	std::ostringstream o;
	o << id;
	TH1D* hist = new TH1D(o.str().c_str(), o.str().c_str(), it->second.size(), 1, it->second.size());
	for(std::vector<double>::size_type i = 0; i < it->second.size(); i++) hist->SetBinContent( i+1, it->second[i]/norm[id] );
	hist->Write();
      }
    }
  }

  if( theAnalysisType == COARSE_DELAY ){
    theOutputFile->mkdir("Coarse_Delay");
    theOutputFile->cd("Coarse_Delay");

    std::map<edm::det_id_type, std::vector<double> >::iterator it;
    for(it = signalmap.begin(); it != signalmap.end(); it++){
      if(it->second.empty()) edm::LogError("LasCrackAnalysis") << "empty entry in signalmap";
      std::ostringstream o;
      o << it->first;
      TH1D* hist = new TH1D(o.str().c_str(), o.str().c_str(), it->second.size(), 1, it->second.size());
      for(std::vector<double>::size_type i = 0; i < it->second.size(); i++) hist->SetBinContent(i+1, it->second[i]);
      hist->Write();
    }
  }

  theOutputFile->Write();
  theOutputFile->Close();
}



//define this as a plug-in
DEFINE_FWK_MODULE(LasCrackAnalysis);

