/* \class GenJetInputParticleSelector
*
*  Selects particles that are used as input for the GenJet collection.
*  Logic: select all stable particles, except for particles specified in
*  the config file that come from
*  W,Z and H decays, and except for a special list, which can be used for
*  unvisible BSM-particles.
*  It is also possible to only selected the partonic final state, 
*  which means all particles before the hadronization step.
*
*  The algorithm is based on code of Christophe Saout.
*
*  Usage: [example for no resonance from nu an mu, and deselect invisible BSM 
*         particles ]
*
*  module genJetParticles = myGenJetsPartSelector {
*                InputTag src = "genParticles"
*                bool partonicFinalState = false  
*                bool excludeResonances = true   
*                vuint32 excludeFromResonancePids = {13,12,14,16}
*                bool tausAsJets = false
*                vuint32 ignoreParticleIDs = {   1000022, 2000012, 2000014,
*                                                2000016, 1000039, 5000039,
*                                                4000012, 9900012, 9900014,
*                                                9900016, 39}
*        }
*
*
* \author: Christophe Saout, Andreas Oehler
*
*/

#include "myGenJetsPartSelector.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
//#include <iostream>
#include <memory>
#include "CommonTools/CandUtils/interface/pdgIdUtils.h"

using namespace std;

myGenJetsPartSelector::myGenJetsPartSelector(const edm::ParameterSet &params ):
  inTag(params.getParameter<edm::InputTag>("src")),
  partonicFinalState(params.getParameter<bool>("partonicFinalState")),
  excludeResonances(params.getParameter<bool>("excludeResonances")),
  tausAsJets(params.getParameter<bool>("tausAsJets")),
  ptMin(0.0){
  if (params.exists("ignoreParticleIDs"))
    setIgnoredParticles(params.getParameter<std::vector<unsigned int> >
			("ignoreParticleIDs"));
  setExcludeFromResonancePids(params.getParameter<std::vector<unsigned int> >
			("excludeFromResonancePids"));

  if(params.exists("ignoreParticleDecayIDs")) ignoreParticleDecayIDs = params.getParameter<std::vector<int> >("ignoreParticleDecayIDs");
  invertignoreParticleDecay = false;
  if(params.exists("invertignoreParticleDecay")) invertignoreParticleDecay = params.getParameter<bool>("invertignoreParticleDecay");

  produces <reco::GenParticleRefVector> ();
      
}

myGenJetsPartSelector::~myGenJetsPartSelector(){}

void myGenJetsPartSelector::setIgnoredParticles(const std::vector<unsigned int> &particleIDs)
{
  ignoreParticleIDs = particleIDs;
  std::sort(ignoreParticleIDs.begin(), ignoreParticleIDs.end());
}

void myGenJetsPartSelector::setExcludeFromResonancePids(const std::vector<unsigned int> &particleIDs)
{
  excludeFromResonancePids = particleIDs;
  std::sort( excludeFromResonancePids.begin(), excludeFromResonancePids.end());
}

bool myGenJetsPartSelector::isParton(int pdgId) const{
  pdgId = (pdgId > 0 ? pdgId : -pdgId) % 10000;
  return (pdgId > 0 && pdgId < 6) || pdgId == 7 ||
    pdgId == 9 || (tausAsJets && pdgId == 15) || pdgId == 21;
  // tops are not considered "regular" partons
  // but taus eventually are (since they may hadronize later)
}

bool myGenJetsPartSelector::isHadron(int pdgId)
{
  pdgId = (pdgId > 0 ? pdgId : -pdgId) % 10000;
  return (pdgId > 100 && pdgId < 900) ||
    (pdgId > 1000 && pdgId < 9000);
}

bool myGenJetsPartSelector::isResonance(int pdgId)
{
  // gauge bosons and tops
  pdgId = (pdgId > 0 ? pdgId : -pdgId) % 10000;
  return (pdgId > 21 && pdgId <= 42) || pdgId == 6 || pdgId == 8 ;  //BUG! war 21. 22=gamma..
}

bool myGenJetsPartSelector::isIgnored(int pdgId) const
{
  pdgId = pdgId > 0 ? pdgId : -pdgId;
  std::vector<unsigned int>::const_iterator pos =
    std::lower_bound(ignoreParticleIDs.begin(),
		     ignoreParticleIDs.end(),
		     (unsigned int)pdgId);
  return pos != ignoreParticleIDs.end() && *pos == (unsigned int)pdgId;
}

bool myGenJetsPartSelector::isExcludedFromResonance(int pdgId) const
{
  pdgId = pdgId > 0 ? pdgId : -pdgId;
  std::vector<unsigned int>::const_iterator pos =
    std::lower_bound(excludeFromResonancePids.begin(),
		     excludeFromResonancePids.end(),
		     (unsigned int)pdgId);
  return pos != excludeFromResonancePids.end() && *pos == (unsigned int)pdgId;
 
}

static unsigned int partIdx(const myGenJetsPartSelector::ParticleVector &p,
			    //const reco::GenParticle *particle)
			    const reco::GenParticle *particle)
{
  myGenJetsPartSelector::ParticleVector::const_iterator pos =
    std::lower_bound(p.begin(), p.end(), particle);
  if (pos == p.end() || *pos != particle)
    throw cms::Exception("CorruptedData")
      << "reco::GenEvent corrupted: Unlisted particles"
      " in decay tree." << std::endl;

  return pos - p.begin();
}
    
static void invalidateTree(myGenJetsPartSelector::ParticleBitmap &invalid,
			   const myGenJetsPartSelector::ParticleVector &p,
			   const reco::GenParticle *particle)
{
  unsigned int npart=particle->numberOfDaughters();
  if (!npart) return;

  for (unsigned int i=0;i<npart;++i){
    unsigned int idx=partIdx(p,dynamic_cast<const reco::GenParticle*>(particle->daughter(i)));
    if (invalid[idx])
      continue;
    invalid[idx] = true;
    //cout<<"Invalidated: ["<<setw(4)<<idx<<"] With pt:"<<particle->daughter(i)->pt()<<endl;
    invalidateTree(invalid, p, dynamic_cast<const reco::GenParticle*>(particle->daughter(i)));
  }
}
  
  
int myGenJetsPartSelector::testPartonChildren
(myGenJetsPartSelector::ParticleBitmap &invalid,
 const myGenJetsPartSelector::ParticleVector &p,
 const reco::GenParticle *particle) const
{
  unsigned int npart=particle->numberOfDaughters();
  if (!npart) {return 0;}

  for (unsigned int i=0;i<npart;++i){
    unsigned int idx = partIdx(p,dynamic_cast<const reco::GenParticle*>(particle->daughter(i)));
    if (invalid[idx])
      continue;
    if (isParton((particle->daughter(i)->pdgId()))){
      return 1;
    }
    if (isHadron((particle->daughter(i)->pdgId()))){
      return -1;
    }
    int result = testPartonChildren(invalid,p,dynamic_cast<const reco::GenParticle*>(particle->daughter(i)));
    if (result) return result;
  }
  return 0;
}

myGenJetsPartSelector::ResonanceState
myGenJetsPartSelector::fromResonance(ParticleBitmap &invalid,
							   const ParticleVector &p,
							   const reco::GenParticle *particle) const
{
  unsigned int idx = partIdx(p, particle);
  int id = particle->pdgId();

  if (invalid[idx]) return kIndirect;
      
  if (isResonance(id) && particle->status() == 3){
    return kDirect;
  }

  
  if (!isIgnored(id) && (isParton(id)))
    return kNo;
  
  
  
  unsigned int nMo=particle->numberOfMothers();
  if (!nMo)
    return kNo;
  

  for(unsigned int i=0;i<nMo;++i){
    ResonanceState result = fromResonance(invalid,p,dynamic_cast<const reco::GenParticle*>(particle->mother(i)));
    switch(result) {
    case kNo:
      break;
    case kDirect:
      if (dynamic_cast<const reco::GenParticle*>(particle->mother(i))->pdgId()==id || isResonance(id))
	return kDirect;
      if(!isExcludedFromResonance(id))
	break;
    case kIndirect:
      return kIndirect;
    }
  }
return kNo;
}

    
bool myGenJetsPartSelector::hasPartonChildren(ParticleBitmap &invalid,
						     const ParticleVector &p,
						     const reco::GenParticle *particle) const {
  return testPartonChildren(invalid, p, particle) > 0;
}
    
//######################################################
//function NEEDED and called per EVENT by FRAMEWORK:
void myGenJetsPartSelector::produce (edm::Event &evt, const edm::EventSetup &evtSetup){

 
  std::auto_ptr<reco::GenParticleRefVector> selected_ (new reco::GenParticleRefVector);
    
  evt.getByLabel(inTag, genParticles );
    
    
  ParticleVector particles;
  for (reco::GenParticleCollection::const_iterator iter=genParticles->begin();iter!=genParticles->end();++iter){
    particles.push_back(&*iter); 
  }
  
  std::sort(particles.begin(), particles.end());
  unsigned int size = particles.size();
  
  ParticleBitmap selected(size, false);
  ParticleBitmap invalid(size, false);

  for(unsigned int i = 0; i < size; i++) {
    const reco::GenParticle *particle = particles[i];
    if (invalid[i])
      continue;
    if (particle->status() == 1)
      selected[i] = true;
    if (partonicFinalState && isParton(particle->pdgId())) {
	  
      if (particle->numberOfDaughters()==0 &&
	  particle->status() != 1) {
	// some brokenness in event...
	invalid[i] = true;
      }
      else if (!hasPartonChildren(invalid, particles,
				  particle)) {
	selected[i] = true;
	invalidateTree(invalid, particles,particle); //this?!?
      }
    }
	
  }
 unsigned int count=0;
  for(size_t idx=0;idx<genParticles->size();++idx){ 
    const reco::GenParticle *particle = particles[idx];

    if (!selected[idx] || invalid[idx]){
      continue;
    }
	
    if (excludeResonances &&
	fromResonance(invalid, particles, particle)) {
      invalid[idx] = true;
//      cout<<"[INPUTSELECTOR] Invalidates FROM RESONANCE!: ["<<setw(4)<<idx<<"] "<<particle->pdgId()<<" "<<particle->pt()<<endl;
      continue;
    }
	
    if (isIgnored(particle->pdgId())){
      continue;
    }

    bool isFoundMother = false;
    for(size_t id =0; id< ignoreParticleDecayIDs.size(); id++){
       if( find_mother(particle, ignoreParticleDecayIDs[id]) ) isFoundMother = true;
    }

    if( (invertignoreParticleDecay && !isFoundMother) || (!invertignoreParticleDecay && isFoundMother) ) continue;
   
    if (particle->pt() >= ptMin){
      edm::Ref<reco::GenParticleCollection> particleRef(genParticles,idx);
      selected_->push_back(particleRef);
//      cout<<"Finally we have: ["<<setw(4)<<idx<<"] "<<setw(4)<<particle->pdgId()<<" "<<particle->pt()<<endl;
      count++;
    }
  }
  evt.put(selected_);
}


bool myGenJetsPartSelector::find_mother(const reco::Candidate* genp, const int motherID){
  for(size_t im=0; im<genp->numberOfMothers(); im++){
    if( (genp->mother(im))->pdgId()==motherID ) return true;
  }
  bool result = false;
  for(size_t im=0; im<genp->numberOfMothers(); im++){
    if( (genp->mother(im))->numberOfMothers()>0 ) result |= find_mother(genp->mother(im), motherID);
    if( result ) return true;
  }
  return false;
}


int myGenJetsPartSelector::find_idx(const reco::Candidate* target){
   int pickedIdx = -1;
   for(size_t ig=0; ig<genParticles->size(); ig++){
      const reco::GenParticle& gen = genParticles->at(ig);
      if( target->p4() == gen.p4() && target->vertex() == gen.vertex() && target->charge() == gen.charge() ){
         pickedIdx = (int)ig;
         break;
      }
   }
   return pickedIdx;
}
  
//define this as a plug-in
DEFINE_FWK_MODULE(myGenJetsPartSelector);
