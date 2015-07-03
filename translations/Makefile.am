trdir=${pkgdatadir}/translations
dist_tr_DATA=manitou_fr.ts manitou_fr.qm \
	manitou_es.ts manitou_es.qm \
	manitou_de.ts manitou_de.qm

%.qm : %.ts
	$(LRELEASE) $<

