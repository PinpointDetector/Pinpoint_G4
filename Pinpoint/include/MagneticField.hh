#ifndef MagneticField_h
#define MagneticField_h 1

#include "G4MagneticField.hh"
#include "MagneticField.hh"
#include "G4GenericMessenger.hh"
#include "globals.hh"

#include <CLHEP/Units/SystemOfUnits.h>

class G4GenericMessenger;

/// Uniform magnetic field in x direction with 0.57 T strength

class MagneticField : public G4MagneticField
{
  public:
    MagneticField() { DefineCommands(); }
    ~MagneticField() override { delete fMessenger; }

    void GetFieldValue(const G4double point[4], double* bField) const override {
      bField[0] = fBx;
      bField[1] = 0.;
      bField[2] = 0.;
    }

    void SetField(G4double val) { 
      fBx = val; 
      G4cout << "MagneticField::SetField called - new field strength: " << fBx/CLHEP::tesla << " T" << G4endl;
    }
    G4double GetField() const { return fBx; }

  private:
    void DefineCommands()
    {
      // Define /Pinpoint/field command directory using generic messenger class
      fMessenger = new G4GenericMessenger(this, "/Pinpoint/field/", "Field control");

      // fieldValue command
      auto& valueCmd = fMessenger->DeclareMethodWithUnit("value", "tesla", &MagneticField::SetField,
                                                        "Set field strength.");
      valueCmd.SetParameterName("field", true);
      valueCmd.SetDefaultValue("0.57");
    }

    G4GenericMessenger* fMessenger = nullptr;
    G4double fBx = 0.57 * CLHEP::tesla;
};

#endif
