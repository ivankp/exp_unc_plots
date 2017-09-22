#/bin/bash

# trac url:
# https://svnweb.cern.ch/trac/atlasoff/browser/
# PhysicsAnalysis/HiggsPhys/Run2/HGamma/xAOD/HGamCrossSections/trunk/

for f in \
  HGamEFTScanner/ATLAS_Run2_v3.HepData \
  HGamEFTScanner/ATLAS_Run2_v4.HepData \
  HiggsXSecPlotting/MCpredictions_v3.data
do
  if [ ! -f "$(basename $f)" ]; then
    if [ -z "$user" ]; then
      read -p "Username for svn.cern.ch: " user
    fi
    echo "Getting $f"
    svn export svn+ssh://${user}@svn.cern.ch/reps/atlasoff/\
PhysicsAnalysis/HiggsPhys/Run2/HGamma/xAOD/HGamCrossSections/trunk/$f
  fi
done
