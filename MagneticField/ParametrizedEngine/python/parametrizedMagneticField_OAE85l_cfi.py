import FWCore.ParameterSet.Config as cms

idealMagneticFieldRecordSource = cms.ESSource("EmptyESSource",
    recordName = cms.string('IdealMagneticFieldRecord'),
    iovIsRunNotTime = cms.bool(True),
    firstValid = cms.vuint32(1)
)

ParametrizedMagneticFieldProducer = cms.ESProducer("ParametrizedMagneticFieldProducer",
    version = cms.string('OAE_85l_030919'),
    parameters = cms.PSet(
        b0 = cms.double(40.681)
        l = cms.double(15.284)
        a = cms.double(4.6430)
    ),
    label = cms.untracked.string('')
)


