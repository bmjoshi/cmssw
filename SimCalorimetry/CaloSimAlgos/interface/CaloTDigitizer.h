#ifndef CaloSimAlgos_CaloTDigitizer_h
#define CaloSimAlgos_CaloTDigitizer_h

/** Turns hits into digis.  Assumes that 
    there's an ElectroncsSim class with the
    interface analogToDigital(CLHEP::HepRandomEngine*, const CaloSamples &, Digi &);

*/
#include "SimCalorimetry/CaloSimAlgos/interface/CaloHitResponse.h"
#include "SimCalorimetry/CaloSimAlgos/interface/CaloVNoiseSignalGenerator.h"
#include "SimDataFormats/CrossingFrame/interface/MixCollection.h"
#include "SimDataFormats/CaloHit/interface/PCaloHit.h"
#include <cassert>
#include <vector>

namespace CLHEP {
  class HepRandomEngine;
}

template<class Traits>
class CaloTDigitizerDefaultRun {
public:
  typedef typename Traits::ElectronicsSim ElectronicsSim;
  typedef typename Traits::Digi Digi;
  typedef typename Traits::DigiCollection DigiCollection;

  void operator()(DigiCollection & output, CLHEP::HepRandomEngine* engine, CaloSamples * analogSignal, std::vector<DetId>::const_iterator idItr, ElectronicsSim* theElectronicsSim){
    Digi digi(*idItr);
    theElectronicsSim->analogToDigital(engine, *analogSignal , digi);
    output.push_back(std::move(digi));
  }
  
};

//second parameter changes the operation of run() slightly (default value for old-style with edm::SortedCollection instead of edm::DataFrameContainer)
template<class Traits, template <class> class runHelper=CaloTDigitizerDefaultRun>
class CaloTDigitizer
{
public:
  /// these are the types that need to be defined in the Traits
  /// class.  The ElectronicsSim needs to have an interface
  /// that you'll see in the run() method
  typedef typename Traits::ElectronicsSim ElectronicsSim;
  typedef typename Traits::Digi Digi;
  typedef typename Traits::DigiCollection DigiCollection;

  CaloTDigitizer(CaloHitResponse * hitResponse, ElectronicsSim * electronicsSim, bool addNoise)
  :  theHitResponse(hitResponse),
     theNoiseSignalGenerator(0),
     theElectronicsSim(electronicsSim),
     theDetIds(0),
     addNoise_(addNoise)
  {
  }


  /// doesn't delete the pointers passed in
  ~CaloTDigitizer() {}

  /// tell the digitizer which cells exist
  const std::vector<DetId>&  detIds() const {assert( 0 != theDetIds ) ; return *theDetIds;}
  void setDetIds(const std::vector<DetId> & detIds) {theDetIds = &detIds;}

  void setNoiseSignalGenerator(CaloVNoiseSignalGenerator * generator)
  {
    theNoiseSignalGenerator = generator;
  }

  void add(const std::vector<PCaloHit> & hits, int bunchCrossing, CLHEP::HepRandomEngine* engine) {
    if(theHitResponse->withinBunchRange(bunchCrossing)) {
      for(std::vector<PCaloHit>::const_iterator it = hits.begin(), itEnd = hits.end(); it != itEnd; ++it) {
        theHitResponse->add(*it, engine);
      }
    }
  }

  void initializeHits() {
     theHitResponse->initializeHits();
  }

  /// turns hits into digis
  void run(MixCollection<PCaloHit> &, DigiCollection &) {
    assert(0);
  }

  /// Collects the digis

  void run(DigiCollection & output, CLHEP::HepRandomEngine* engine) {
    theHitResponse->finalizeHits(engine);
    //std::cout << " In CaloTDigitizer, after finalize hits " << std::endl;

    assert(theDetIds->size() != 0);

    if(theNoiseSignalGenerator != 0) addNoiseSignals(engine);

    theElectronicsSim->newEvent(engine);

    // reserve space for how many digis we expect
    int nDigisExpected = addNoise_ ? theDetIds->size() : theHitResponse->nSignals();
    output.reserve(nDigisExpected);

    // make a raw digi for evey cell
    for(std::vector<DetId>::const_iterator idItr = theDetIds->begin();
        idItr != theDetIds->end(); ++idItr)
    {
       CaloSamples * analogSignal = theHitResponse->findSignal(*idItr);
       bool needToDeleteSignal = false;
       // don't bother digitizing if no signal and no noise
       if(analogSignal == 0 && addNoise_) {
         // I guess we need to make a blank signal for this cell.
         // Don't bother storing it anywhere.
         analogSignal = new CaloSamples(theHitResponse->makeBlankSignal(*idItr));
         needToDeleteSignal = true;
       }
       if(analogSignal != 0) { 
         runAnalogToDigital(output,engine,analogSignal,idItr,theElectronicsSim);
         if(needToDeleteSignal) delete analogSignal;
      }
    }

    // free up some memory
    theHitResponse->clear();
  }

  void addNoiseSignals(CLHEP::HepRandomEngine* engine)
  {
    std::vector<CaloSamples> noiseSignals;
    // noise signals need to be in units of photoelectrons.  Fractional is OK
    theNoiseSignalGenerator->fillEvent(engine);
    theNoiseSignalGenerator->getNoiseSignals(noiseSignals);
    for(std::vector<CaloSamples>::const_iterator signalItr = noiseSignals.begin(),
        signalEnd = noiseSignals.end(); signalItr != signalEnd; ++signalItr)
    {
      theHitResponse->add(*signalItr);
    }
  }

private:
  runHelper<Traits> runAnalogToDigital;
  CaloHitResponse * theHitResponse;
  CaloVNoiseSignalGenerator * theNoiseSignalGenerator;
  ElectronicsSim * theElectronicsSim;
  const std::vector<DetId>* theDetIds;
  bool addNoise_;
};

#endif

