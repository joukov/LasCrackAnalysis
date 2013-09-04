#include "FWCore/Framework/interface/EDAnalyzer.h"
#include <DataFormats/Common/interface/DetSetVector.h>
#include <FWCore/Framework/interface/Event.h> 

// Forward declarations 
class TFile;
class TTree;

class LasCrackAnalysis : public edm::EDAnalyzer {
  
 public:
  explicit LasCrackAnalysis(const edm::ParameterSet&);
  ~LasCrackAnalysis();
  
  //Private methods  
 private:
  enum DigiType {ZeroSuppressed, VirginRaw, ProcessedRaw, Unknown};
  enum AnalysisType {PEDESTALS, COARSE_DELAY};

  virtual void beginJob() ;
  virtual void beginRun(edm::Run const &, edm::EventSetup const &) ;
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
  virtual void endJob() ;

  void AnalyzeZeroSuppressed(const edm::Event&);
  void AnalyzeVirginRaw(const edm::Event&);

  //Private Members
 private:

  std::string DigisModuleLabel;
  std::string DigisInstanceLabel;

  AnalysisType theAnalysisType;
  DigiType theDigiType;

  TFile* theOutputFile;
  TTree* theOutputTree;

  int latency;
  int eventnumber;
  int runnumber;
  int lumiBlock;
  
  std::map<edm::det_id_type, std::vector<double> > signalmap; // Maximum signal 
  std::map<edm::det_id_type, std::vector<double> > pedmap; // Pedestals
  std::map<edm::det_id_type, unsigned int > norm; // Pedestal norm
};

