#ifndef ANASAMPLES_SAMPLES_H
#define ANASAMPLES_SAMPLES_H

#include <string>
#include <map>
#include <vector>
#include <set>

//#include "TChain.h"
#include <iostream>

namespace AnaSamples
{
    enum COLORS{kRed = 632, kGreen = 416, kBlack = 1, kMagenta = 616, kBlue = 600, kYellow = 400, kTeal = 840, kPink = 900, kOrange = 800, kSpring = 820, kWhite = 0, kGray = 0, kCyan = 432, kAzure = 860, kViolet = 880};

    class FileSummary
    {
    public:
        std::string tag;
        std::string filePath, treePath;
        double xsec, lumi, kfactor, nEvts;
        int color;
        bool isData_;
        
        FileSummary() {}
        FileSummary(std::string tag, std::string filePath, std::string treePath, double xsec, double lumi, double nEvts, double kfactor, int color = kBlack) : tag(tag), filePath(filePath), treePath(treePath), xsec(xsec), lumi(lumi), kfactor(kfactor), nEvts(nEvts), color(color), isData_(false)
        {
            weight_ = xsec * lumi * kfactor / nEvts;
            readFileList();
        }
	
	// Constructor which doesn't make a xsec*lumi weighted sample, e.g. for use with data.
	//Initialize xsec, lumi, nEvts to 1 so that the comparison operators still work
        //Need a record of the actual data lumi!
        FileSummary(std::string tag, std::string filePath, std::string treePath, double lumi, double kfactor, int color = kBlack) : tag(tag), filePath(filePath), treePath(treePath), xsec(1), lumi(lumi), kfactor(kfactor), nEvts(1), color(color), isData_(true)
        {
            weight_ = kfactor;
            readFileList();
        }

        double getWeight() const {return weight_;}
        const std::vector<std::string>& getFilelist() const {return filelist_;}
        template<class T> void addFilesToChain(T* chain,  int startfile =0, int filerun= -1) const
        {
	  if(filerun<0)filerun=filelist_.size();
	  for(int fn = startfile; fn < startfile+filerun && fn<filelist_.size(); fn++){
	      chain->Add(filelist_[fn].c_str());
	    }
        }
        std::vector<std::string> filelist_;

        void addCollection(std::string);
        const std::set<std::string>& getCollections() const
        {
            return collections_;
        }
        
        void readFileList();

    private:
        double weight_;
        std::set<std::string> collections_;
    };

    bool operator< (const FileSummary& lhs, const FileSummary& rhs);
    bool operator== (const FileSummary& lhs, const FileSummary& rhs);
    bool operator!= (const FileSummary& lhs, const FileSummary& rhs);


    static const double luminosity = 2262.0; // in pb-1
    //static const std::string fileDir = "/eos/uscms/store/user/lpcsusyhad/PHYS14_720_Dec23_2014/";
    //static const std::string fileDir = "/eos/uscms/store/user/lpcsusyhad/PHYS14_720_Mar14_2014_v2/";
    //static const std::string fileDir = "/eos/uscms/store/user/lpcsusyhad/PHYS14_72X_July_2015_v1.1/";
    //static const std::string fileDir = "/eos/uscms/store/user/lpcsusyhad/Spring15_74X_July_2015_v1.1/";
    //static const std::string fileDir = "/eos/uscms/store/user/lpcsusyhad/Spring15_74X_Oct_2015_Ntp_v2X/";
    static const std::string fileDir = "/eos/uscms/store/user/lpcsusyhad/";

    template<class T>
    class SampleBase
    {
    public:
        SampleBase() : nullT_() {};
        const T& operator[](const std::string& key) const
        {
            auto iter = sampleSet_.find(key);
            if(iter != sampleSet_.end()) return iter->second;
            else                         return nullT_;
        }

        const T& null() const {return nullT_;}
    protected:
        std::map<std::string, T> sampleSet_;
        const T nullT_;

    public:
        decltype(sampleSet_.cbegin()) begin() const { return sampleSet_.cbegin(); }
        decltype(sampleSet_.cend())     end() const { return sampleSet_.cend(); }
        decltype(sampleSet_.size())    size() const { return sampleSet_.size(); }
    };

    class SampleCollection;

    class SampleSet : public SampleBase<FileSummary>
    {
        friend class SampleCollection;
        
    public:
        SampleSet(std::string fDir = fileDir, double lumi = luminosity);

        void addSample(std::string tag, std::string filePath, std::string treePath, double xsec, double lumi, double nEvts, double kfactor, int color = kBlack) 
        {
            sampleSet_[tag] = FileSummary(tag, filePath, treePath, xsec, lumi, nEvts, kfactor, color);
        }

        void addSample(std::string tag, std::string filePath, std::string treePath, double lumi, double kfactor, int color = kBlack) 
        {
            sampleSet_[tag] = FileSummary(tag, filePath, treePath, lumi, kfactor, color);
        }

    private:
        std::string fDir_;        
	double lumi_;

        std::map<std::string, FileSummary>& getMap();
    };

    class SampleCollection : public SampleBase<std::vector<FileSummary>>
    {
    public:
        SampleCollection(SampleSet& samples);
        std::vector<std::string>& getSampleLabels(std::string name);
        inline double getSampleLumi(std::string name)
        {
            return totalLumiMap_[name];
        }
    private:
        std::map<std::string, double> totalLumiMap_;
        std::map<std::string, std::vector<std::string>> nameVec_;
        void addSampleSet(SampleSet& samples, std::string name, std::vector<std::string> vss);
    };
}

#endif
