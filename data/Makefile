export PATH := ../bin:../scr:$(PATH)

.PHONY: all clean cleanall

DATA1 := ATLAS_Run2_v3.HepData ATLAS_Run2_v4.HepData MCpredictions_v3.data
DATA2 := hepdata1.dat hepdata2.dat mc.dat uncert.dat uncert_corr.dat

all: $(DATA2)

$(DATA1):
	dl.sh

hepdata1.dat: ATLAS_Run2_v3.HepData
	convert_hepdata $^ | edit -o $@ --sym

hepdata2.dat: hepdata1.dat
	edit $^ -o $@ --qadd=fit fit bkg_model_uncorr

mc.dat: MCpredictions_v3.data hepdata2.dat
	mc.sh $^ | edit -o $@ --add-except=xsec --prec=4

uncert.dat: hepdata2.dat mc.dat
	edit $^ -o $@ --qadd-except=corr xsec stat lumi fit --tol=0.05 \
	  --order xsec lumi corr fit stat

uncert_corr.dat: hepdata2.dat
	edit $^ -o $@ --rm stat lumi fit --top=4:others

clean:
	@rm -fv $(DATA2)

cleanall: clean
	@rm -fv $(DATA1)
