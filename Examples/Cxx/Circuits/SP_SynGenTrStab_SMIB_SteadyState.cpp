#include <DPsim.h>


using namespace DPsim;
using namespace CPS;


//-----------Power system-----------//
//Voltage level as Base Voltage
Real Vnom = 230e3;

//-----------Generator-----------//
Real nomPower = 500e6;
Real nomPhPhVoltRMS = 22e3;
Real nomFreq = 60;
Real nomOmega= nomFreq* 2*PI;
Real H = 5;
Real Xpd=0.31;
Real Rs = 0.003*0;
Real Kd = 1;
// Initialization parameters
Real initMechPower= 300e6;
Real initActivePower = 300e6;
Real setPointVoltage=nomPhPhVoltRMS + 0.05*nomPhPhVoltRMS;

//-----------Transformer-----------//
Real t_ratio=Vnom/nomPhPhVoltRMS;

//PiLine parameters calculated from CIGRE Benchmark system
Real lineResistance = 6.7;
Real lineInductance = 47./nomOmega;
Real lineCapacitance = 3.42e-4/nomOmega;
Real lineConductance =0;

// Parameters for powerflow initialization
// Slack voltage: 1pu
Real Vslack = Vnom;

void SP_1ph_SynGenTrStab_SteadyState(String simName, Real timeStep, Real finalTime, bool startFaultEvent, bool endFaultEvent, Real startTimeFault, Real endTimeFault, Real cmdInertia) {
	//  // ----- POWERFLOW FOR INITIALIZATION -----
	Real timeStepPF = finalTime;
	Real finalTimePF = finalTime+timeStepPF;
	String simNamePF = simName + "_PF";
	Logger::setLogDir("logs/" + simNamePF);

	// Components
	auto n1PF = SimNode<Complex>::make("n1", PhaseType::Single);
	auto n2PF = SimNode<Complex>::make("n2", PhaseType::Single);

	//Synchronous generator ideal model
	auto genPF = SP::Ph1::SynchronGenerator::make("Generator", Logger::Level::debug);
	// setPointVoltage is defined as the voltage at the transfomer primary side and should be transformed to network side
	genPF->setParameters(nomPower, nomPhPhVoltRMS, initActivePower, setPointVoltage*t_ratio, PowerflowBusType::PV);
	genPF->setBaseVoltage(Vnom);
	genPF->modifyPowerFlowBusType(PowerflowBusType::PV);

	//Grid bus as Slack
	auto extnetPF = SP::Ph1::NetworkInjection::make("Slack", Logger::Level::debug);
	extnetPF->setParameters(Vslack);
	extnetPF->setBaseVoltage(Vnom);
	extnetPF->modifyPowerFlowBusType(PowerflowBusType::VD);
	
	//Line
	auto linePF = SP::Ph1::PiLine::make("PiLine", Logger::Level::debug);
	linePF->setParameters(lineResistance, lineInductance, lineCapacitance, lineConductance);
	linePF->setBaseVoltage(Vnom);

	// Topology
	genPF->connect({ n1PF });
	linePF->connect({ n1PF, n2PF });
	extnetPF->connect({ n2PF });
	auto systemPF = SystemTopology(60,
			SystemNodeList{n1PF, n2PF},
			SystemComponentList{genPF, linePF, extnetPF});

	// Logging
	auto loggerPF = DataLogger::make(simNamePF);
	loggerPF->addAttribute("v1", n1PF->attribute("v"));
	loggerPF->addAttribute("v2", n2PF->attribute("v"));

	// Simulation
	Simulation simPF(simNamePF, Logger::Level::debug);
	simPF.setSystem(systemPF);
	simPF.setTimeStep(timeStepPF);
	simPF.setFinalTime(finalTimePF);
	simPF.setDomain(Domain::SP);
	simPF.setSolverType(Solver::Type::NRP);
	simPF.doInitFromNodesAndTerminals(false);
	simPF.addLogger(loggerPF);
	simPF.run();

	// ----- Dynamic simulation ------
	String simNameSP = simName + "_SP";
	Logger::setLogDir("logs/"+simNameSP);
	
	// Nodes
	auto n1SP = SimNode<Complex>::make("n1", PhaseType::Single);
	auto n2SP = SimNode<Complex>::make("n2", PhaseType::Single);

	// Components
	auto genSP = SP::Ph1::SynchronGeneratorTrStab::make("SynGen", Logger::Level::debug);
	// Xpd is given in p.u of generator base at transfomer primary side and should be transformed to network side
	genSP->setStandardParametersPU(nomPower, nomPhPhVoltRMS, nomFreq, Xpd*std::pow(t_ratio,2), cmdInertia*H, Rs, Kd );
	// Get actual active and reactive power of generator's Terminal from Powerflow solution
	Complex initApparentPower= genPF->getApparentPower();
	genSP->setInitialValues(initApparentPower, initMechPower);

	//Grid bus as Slack
	auto extnetSP = SP::Ph1::NetworkInjection::make("Slack", Logger::Level::debug);
	extnetSP->setParameters(Vslack);
	// Line
	auto lineSP = SP::Ph1::PiLine::make("PiLine", Logger::Level::debug);
	lineSP->setParameters(lineResistance, lineInductance, lineCapacitance, lineConductance);

	// Topology
	genSP->connect({ n1SP });
	lineSP->connect({ n1SP, n2SP });
	extnetSP->connect({ n2SP });
	auto systemSP = SystemTopology(60,
			SystemNodeList{n1SP, n2SP},
			SystemComponentList{genSP, lineSP, extnetSP});

	// Initialization of dynamic topology
	CIM::Reader reader(simNameSP, Logger::Level::debug);
	reader.initDynamicSystemTopologyWithPowerflow(systemPF, systemSP);


	// Logging
	auto loggerSP = DataLogger::make(simNameSP);
	loggerSP->addAttribute("v1", n1SP->attribute("v"));
	loggerSP->addAttribute("v2", n2SP->attribute("v"));
	//gen
	loggerSP->addAttribute("Ep", genSP->attribute("Ep"));
	loggerSP->addAttribute("v_gen", genSP->attribute("v_intf"));
	loggerSP->addAttribute("i_gen", genSP->attribute("i_intf"));
	loggerSP->addAttribute("wr_gen", genSP->attribute("w_r"));
	loggerSP->addAttribute("delta_r_gen", genSP->attribute("delta_r"));
	loggerSP->addAttribute("P_elec", genSP->attribute("P_elec"));
	loggerSP->addAttribute("P_mech", genSP->attribute("P_mech"));
	//line
	loggerSP->addAttribute("v_line", lineSP->attribute("v_intf"));
	loggerSP->addAttribute("i_line", lineSP->attribute("i_intf"));
	//slack
	loggerSP->addAttribute("v_slack", extnetSP->attribute("v_intf"));
	loggerSP->addAttribute("i_slack", extnetSP->attribute("i_intf"));



	Simulation simSP(simNameSP, Logger::Level::debug);
	simSP.setSystem(systemSP);
	simSP.setTimeStep(timeStep);
	simSP.setFinalTime(finalTime);
	simSP.setDomain(Domain::SP);
	simSP.addLogger(loggerSP);

	simSP.run();
}

int main(int argc, char* argv[]) {	
		

	//Simultion parameters
	String simName="SP_SynGenTrStab_SMIB_SteadyState";
	Real finalTime = 10;
	Real timeStep = 0.001;
	Bool startFaultEvent=false;
	Bool endFaultEvent=false;
	Real startTimeFault=10;
	Real endTimeFault=10.1;
	Real cmdInertia= 1.0;

	SP_1ph_SynGenTrStab_SteadyState(simName, timeStep, finalTime, startFaultEvent, endFaultEvent, startTimeFault, endTimeFault, cmdInertia);
}