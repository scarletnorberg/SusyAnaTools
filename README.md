## Instructions

The following installation instructions assume the user wants to process Run2015B prompt-reco data or Spring15 MC.

```
cmsrel CMSSW_7_4_15
cd CMSSW_7_4_15/src/
cmsenv
git clone -b TestMiniAOD git@github.com:susy2015/recipeAUX.git
git clone -b Ntp_74X_08Nov2015_v3.0 git@github.com:susy2015/SusyAnaTools.git
scram b -j9
```

To submit jobs, go to SusyAnaTools/SkimsAUX/workdir/prod/74X_crab_example/
Then modify the MultiCrab3.py file for the line:
selSubmitKey = 'TEST ALL'
to:
selSubmitKey = 'TTJets' for all the TTJets* samples or
selSubmitKey = 'WJetsToLNu_HT-100To200' for just the WJetsToLNu_HT-100To200 sample
