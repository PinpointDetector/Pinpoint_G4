#include "ProfilingManager.hh"
#include "G4ios.hh"
#include <iomanip>
#include <algorithm>

ProfilingManager* ProfilingManager::fInstance = nullptr;

ProfilingManager* ProfilingManager::GetInstance()
{
  if (!fInstance) {
    fInstance = new ProfilingManager();
  }
  return fInstance;
}

ProfilingManager::ProfilingManager()
{
}

ProfilingManager::~ProfilingManager()
{
}

void ProfilingManager::StartTimer(const G4String& name)
{
  TimerData& data = fTimers[name];
  if (!data.isRunning) {
    data.timer.Start();
    data.isRunning = true;
  }
}

void ProfilingManager::StopTimer(const G4String& name)
{
  auto it = fTimers.find(name);
  if (it != fTimers.end() && it->second.isRunning) {
    it->second.timer.Stop();
    it->second.totalTime += it->second.timer.GetRealElapsed();
    it->second.count++;
    it->second.isRunning = false;
  }
}

void ProfilingManager::PrintReport()
{
  G4cout << "\n========================================" << G4endl;
  G4cout << "     PROFILING REPORT" << G4endl;
  G4cout << "========================================" << G4endl;
  
  // Calculate total time
  G4double totalTime = 0.0;
  for (const auto& entry : fTimers) {
    totalTime += entry.second.totalTime;
  }
  
  // Create sorted list by total time
  std::vector<std::pair<G4String, const TimerData*>> sortedTimers;
  for (const auto& entry : fTimers) {
    sortedTimers.push_back({entry.first, &entry.second});
  }
  
  std::sort(sortedTimers.begin(), sortedTimers.end(),
            [](const auto& a, const auto& b) {
              return a.second->totalTime > b.second->totalTime;
            });
  
  G4cout << std::left << std::setw(35) << "Operation" 
         << std::right << std::setw(12) << "Time (s)" 
         << std::setw(12) << "Calls" 
         << std::setw(15) << "Avg (ms)" 
         << std::setw(12) << "Percent" << G4endl;
  G4cout << "------------------------------------------------------------------------" << G4endl;
  
  for (const auto& entry : sortedTimers) {
    const G4String& name = entry.first;
    const TimerData* data = entry.second;
    
    G4double avgTime = data->count > 0 ? (data->totalTime / data->count) * 1000.0 : 0.0;
    G4double percent = totalTime > 0 ? (data->totalTime / totalTime) * 100.0 : 0.0;
    
    G4cout << std::left << std::setw(35) << name
           << std::right << std::fixed << std::setprecision(3)
           << std::setw(12) << data->totalTime
           << std::setw(12) << data->count
           << std::setw(15) << avgTime
           << std::setw(11) << percent << "%" << G4endl;
  }
  
  G4cout << "------------------------------------------------------------------------" << G4endl;
  G4cout << std::left << std::setw(35) << "TOTAL"
         << std::right << std::setw(12) << totalTime << G4endl;
  G4cout << "========================================\n" << G4endl;
}

void ProfilingManager::Reset()
{
  fTimers.clear();
}
