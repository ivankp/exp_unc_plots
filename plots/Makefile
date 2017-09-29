PATH := ../bin:$(PATH)

OUT := uncert.pdf uncert_corr.pdf

.PHONY: all burst clean

all: $(OUT)
burst: all

ifeq ($(MAKECMDGOALS),burst)
BURST := --burst
.PHONY: $(OUT)
endif

uncert.pdf: %.pdf: ../data/%.dat
	plot $^ -o $@ $(BURST) \
	  -y "#it{#Delta#sigma}_{fid} / #it{#sigma}_{fid}^{SM}"

uncert_corr.pdf: %.pdf: ../data/%.dat
	plot $^ -o $@ $(BURST) \
	  -y "#it{#Deltacf}/#it{cf}" \
	  -s ../config/brown.sty --y-offset=0.8

clean:
	@rm -fv uncert.pdf uncert_corr.pdf
