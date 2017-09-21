#/bin/bash

# trac url:
# https://svnweb.cern.ch/trac/atlasoff/browser/
# PhysicsAnalysis/HiggsPhys/Run2/HGamma/xAOD/HGamCrossSections/trunk/
# HGamEFTScanner

mkdir -p data

printf "username: "
read user

file=${1:-ATLAS_Run2_v4.HepData}

svn export svn+ssh://${user}@svn.cern.ch/reps/atlasoff/\
PhysicsAnalysis/HiggsPhys/Run2/HGamma/xAOD/HGamCrossSections/trunk/\
HGamEFTScanner/$file \
data/$file
