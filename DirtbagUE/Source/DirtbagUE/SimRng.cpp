// Bridge: compiles the engine-free sim RNG into the game module. UBT only
// builds .cpp files under the module, so each sim translation unit gets one
// of these — the sim itself stays in /Sim, tested by Sim/run-tests.sh.
#include "../../../Sim/DirtbagRng.cpp"
