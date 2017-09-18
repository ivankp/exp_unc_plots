#/bin/bash

# trac url:
# https://svnweb.cern.ch/trac/atlasoff/browser/PhysicsAnalysis/HiggsPhys/Run2/HGamma/xAOD/HGamCrossSections/trunk/HGamEFTScanner

mkdir -p data

printf "username: "
read name

svn export svn+ssh://${name}@svn.cern.ch/reps/atlasoff/\
PhysicsAnalysis/HiggsPhys/Run2/HGamma/xAOD/HGamCrossSections/trunk/\
HGamEFTScanner/ATLAS_Run2_v4.HepData \
data/
