import FWCore.ParameterSet.Config as cms


LasCrackAnalysis = cms.EDAnalyzer(
    "LasCrackAnalysis",
    # list of digi producers
    AnalysisType       = cms.string( 'UNDEFINED' ),
    DigisInstanceLabel = cms.string( 'VirginRaw' ),
    DigisModuleLabel   = cms.string( 'siStripDigis' ), #simSiStripDigis
    )

