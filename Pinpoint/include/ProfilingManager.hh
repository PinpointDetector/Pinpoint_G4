#ifndef ProfilingManager_hh
#define ProfilingManager_hh

#include "G4Timer.hh"
#include "G4String.hh"
#include <map>
#include <vector>

class ProfilingManager
{
  public:
    static ProfilingManager* GetInstance();
    
    void StartTimer(const G4String& name);
    void StopTimer(const G4String& name);
    void PrintReport();
    void Reset();
    
  private:
    ProfilingManager();
    ~ProfilingManager();
    
    static ProfilingManager* fInstance;
    
    struct TimerData {
      G4Timer timer;
      G4int count;
      G4double totalTime;
      G4bool isRunning;
      
      TimerData() : count(0), totalTime(0.0), isRunning(false) {}
    };
    
    std::map<G4String, TimerData> fTimers;
};

// Helper macros for easy profiling
#define PROFILE_START(name) ProfilingManager::GetInstance()->StartTimer(name)
#define PROFILE_STOP(name) ProfilingManager::GetInstance()->StopTimer(name)

#endif
