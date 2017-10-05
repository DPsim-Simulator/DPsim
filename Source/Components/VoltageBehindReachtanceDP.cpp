#include "VoltageBehindReactanceDP.h"

using namespace DPsim;

VoltageBehindReactanceDP::VoltageBehindReactanceDP(std::string name, int node1, int node2, int node3,
	Real nomPower, Real nomVolt, Real nomFreq, int poleNumber, Real nomFieldCur,
	Real Rs, Real Ll, Real Lmd, Real Lmd0, Real Lmq, Real Lmq0,
	Real Rfd, Real Llfd, Real Rkd, Real Llkd,
	Real Rkq1, Real Llkq1, Real Rkq2, Real Llkq2,
	Real inertia) {

	this->mNode1 = node1 - 1;
	this->mNode2 = node2 - 1;
	this->mNode3 = node3 - 1;

	mNomPower = nomPower;
	mNomVolt = nomVolt;
	mNomFreq = nomFreq;
	mPoleNumber = poleNumber;
	mNomFieldCur = nomFieldCur;

	// base stator values
	mBase_V_RMS = mNomVolt / sqrt(3);
	mBase_v = mBase_V_RMS * sqrt(2);
	mBase_I_RMS = mNomPower / (3 * mBase_V_RMS);
	mBase_i = mBase_I_RMS * sqrt(2);
	mBase_Z = mBase_v / mBase_i;
	mBase_OmElec = 2 * DPS_PI * mNomFreq;
	mBase_OmMech = mBase_OmElec / (mPoleNumber / 2);
	mBase_L = mBase_Z / mBase_OmElec;
	mBase_Psi = mBase_L * mBase_i;
	mBase_T = mNomPower / mBase_OmMech;

	// steady state per unit initial value
	initWithPerUnitParam(Rs, Ll, Lmd, Lmd0, Lmq, Lmq0, Rfd, Llfd, Rkd, Llkd, Rkq1, Llkq1, Rkq2, Llkq2, inertia);

}
void VoltageBehindReactanceDP::initWithPerUnitParam(
	Real Rs, Real Ll, Real Lmd, Real Lmd0, Real Lmq, Real Lmq0,
	Real Rfd, Real Llfd, Real Rkd, Real Llkd,
	Real Rkq1, Real Llkq1, Real Rkq2, Real Llkq2,
	Real H) {

	// base rotor values
	mBase_ifd = Lmd * mNomFieldCur;
	mBase_vfd = mNomPower / mBase_ifd;
	mBase_Zfd = mBase_vfd / mBase_ifd;
	mBase_Lfd = mBase_Zfd / mBase_OmElec;

	mRs = Rs;
	mLl = Ll;
	mLmd = Lmd;
	mLmd0 = Lmd0;
	mLmq = Lmq;
	mLmq0 = Lmq0;
	mRfd = Rfd;
	mLlfd = Llfd;
	mRkd = Rkd;
	mLlkd = Llkd;
	mRkq1 = Rkq1;
	mLlkq1 = Llkq1;
	mRkq2 = Rkq2;
	mLlkq2 = Llkq2;
	mH = H;

	if (mRkq2 == 0 & mLlkq2 == 0)
	{
		DampingWinding = 1;
	}

	//Dynamic mutual inductances
	mDLmd = 1. / (1. / mLmd + 1. / mLlfd + 1. / mLlkd);
	if (DampingWinding == 2)
		mDLmq = 1. / (1. / mLmq + 1. / mLlkq1 + 1. / mLlkq2);
	else
		mDLmq = 1. / (1. / mLmq + 1. / mLlkq1);

	mLa = (mDLmq + mDLmd) / 3.;
	mLb = (mDLmd - mDLmq) / 3.;

	LD0 <<
		(mLl + mLa), -mLa / 2, - mLa / 2,
		-mLa / 2, mLl + mLa, -mLa / 2,
		-mLa / 2, -mLa / 2, mLl + mLa;

	//alpha = (cos((2. * PI) / 3.), sin((2. * PI) / 3.));
	//LD1 <<
	//	1, pow(alpha, 2), alpha,
	//	pow(alpha, 2), alpha, 1,
	//	alpha, 1, pow(alpha, 2);
	//LD1 = (-mLb/2.)* LD1;

}


void VoltageBehindReactanceDP::init(Real om, Real dt,
	Real initActivePower, Real initReactivePower, Real initTerminalVolt, Real initVoltAngle) {

	mResistanceMat <<
		mRs, 0, 0,
		0, mRs, 0,
		0, 0, mRs;

	// steady state per unit initial value
	initStatesInPerUnit(initActivePower, initReactivePower, initTerminalVolt, initVoltAngle);

	mVaRe = dq0ToAbcTransform(mThetaMech, mVd, mVq, mV0)(0);
	mVbRe = dq0ToAbcTransform(mThetaMech, mVd, mVq, mV0)(1);
	mVcRe = dq0ToAbcTransform(mThetaMech, mVd, mVq, mV0)(2);
	mVaIm = dq0ToAbcTransform(mThetaMech, mVd, mVq, mV0)(3);
	mVbIm = dq0ToAbcTransform(mThetaMech, mVd, mVq, mV0)(4);
	mVcIm = dq0ToAbcTransform(mThetaMech, mVd, mVq, mV0)(5);


	mIaRe = dq0ToAbcTransform(mThetaMech, mId, mIq, mI0)(0);
	mIbRe = dq0ToAbcTransform(mThetaMech, mId, mIq, mI0)(1);
	mIcRe = dq0ToAbcTransform(mThetaMech, mId, mIq, mI0)(2);
	mIaIm = dq0ToAbcTransform(mThetaMech, mId, mIq, mI0)(3);
	mIbIm = dq0ToAbcTransform(mThetaMech, mId, mIq, mI0)(4);
	mIcIm = dq0ToAbcTransform(mThetaMech, mId, mIq, mI0)(5);


	mVabc <<
		mVaRe,
		mVbRe,
		mVcRe,
		mVaIm,
		mVbIm,
		mVcIm;

	mIabc <<
		mIaRe,
		mIbRe,
		mIcRe,
		mIaIm,
		mIbIm,
		mIcIm;
}

void VoltageBehindReactanceDP::initStatesInPerUnit(Real initActivePower, Real initReactivePower,
	Real initTerminalVolt, Real initVoltAngle) {

	double init_P = initActivePower / mNomPower;
	double init_Q = initReactivePower / mNomPower;
	double init_S = sqrt(pow(init_P, 2.) + pow(init_Q, 2.));
	double init_vt = initTerminalVolt / mBase_v;
	double init_it = init_S / init_vt;

	// power factor
	double init_pf = acos(init_P / init_S);

	// load angle
	double init_delta = atan(((mLmq + mLl) * init_it * cos(init_pf) - mRs * init_it * sin(init_pf)) /
		(init_vt + mRs * init_it * cos(init_pf) + (mLmq + mLl) * init_it * sin(init_pf)));
	double init_delta_deg = init_delta / DPS_PI * 180;

	// dq stator voltages and currents
	double init_vd = init_vt * sin(init_delta);
	double init_vq = init_vt * cos(init_delta);
	double init_id = -init_it * sin(init_delta + init_pf);
	double init_iq = -init_it * cos(init_delta + init_pf);

	// rotor voltage and current
	double init_ifd = (init_vq + mRs * init_iq + (mLmd + mLl) * init_id) / mLmd;
	double init_vfd = mRfd * init_ifd;

	// flux linkages
	double init_psid = init_vq + mRs * init_iq;
	double init_psiq = -init_vd - mRs * init_id;
	double init_psifd = (mLmd + mLlfd) * init_ifd - mLmd * init_id;
	double init_psid1 = mLmd * (init_ifd - init_id);
	double init_psiq1 = -mLmq * init_iq;
	double init_psiq2 = -mLmq * init_iq;

	// rotor mechanical variables
	double init_Te = init_P + mRs * pow(init_it, 2.);
	mOmMech = 1;

	mIq = init_iq;
	mId = init_id;
	mI0 = 0;
	mIfd = init_ifd;
	mIkd = 0;
	mIkq1 = 0;
	mIkq2 = 0;

	mVd = init_vd;
	mVq = init_vq;
	mV0 = 0;
	mVfd = init_vfd;
	mVkd = 0;
	mVkq1 = 0;
	mVkq2 = 0;

	mPsiq = init_psiq;
	mPsid = init_psid;
	mPsi0 = 0;
	mPsifd = init_psifd;
	mPsikd = init_psid1;
	mPsikq1 = init_psiq1;
	mPsikq2 = init_psiq2;

	if (DampingWinding == 2) {
		mDPsiq = mDLmq*(mPsikq1 / mLlkq1) + mDLmq*(mPsikq2 / mLlkq2);
	}
	else {
		mDPsiq = mDLmq*(mPsikq1 / mLlkq1);
	}
	mDPsid = mDLmd*(mPsifd / mLlfd) + mDLmd*(mPsikd / mLlkd);

	if (DampingWinding == 2) {
		mDVq = mOmMech*mDPsid + mDLmq*mRkq1*(mDPsiq - mPsikq1) / (mLlkq1*mLlkq1) +
			mDLmq*mRkq2*(mDPsiq - mPsikq2) / (mLlkq2*mLlkq2) + (mRkq1 / (mLlkq1*mLlkq1) + mRkq2 / (mLlkq2*mLlkq2))*mDLmq*mDLmq*mIq;
	}
	else {
		mDVq = mOmMech*mDPsid + mDLmq*mRkq1*(mDPsiq - mPsikq1) / (mLlkq1*mLlkq1) + (mRkq1 / (mLlkq1*mLlkq1))*mDLmq*mDLmq*mIq;
	}
	mDVd = -mOmMech*mDPsiq + mDLmd*mRkd*(mDPsid - mPsikd) / (mLlkd*mLlkd) + (mDLmd / mLlfd)*mVfd +
		mDLmd*mRfd*(mDPsid - mPsifd) / (mLlfd*mLlfd) + (mRfd / (mLlfd*mLlfd) + mRkd / (mLlkd*mLlkd))*mDLmd*mDLmd*mId;

	// Initialize mechanical angle
	mThetaMech = initVoltAngle + init_delta - PI / 2.;
	mThetaMech2 = initVoltAngle + init_delta;

	mDVaRe = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(0);
	mDVbRe = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(1);
	mDVcRe = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(2);
	mDVaIm = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(3);
	mDVbIm = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(4);
	mDVcIm = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(5);

	R_load <<
		1037.8378 / mBase_Z, 0, 0, 0, 0, 0,
		0, 1037.8378 / mBase_Z, 0, 0, 0, 0,
		0, 0, 1037.8378 / mBase_Z, 0, 0, 0,
		0, 0, 0, 1037.8378 / mBase_Z, 0, 0,
		0, 0, 0, 0, 1037.8378 / mBase_Z, 0,
		0, 0, 0, 0, 0, 1037.8378 / mBase_Z;

	CalculateLandR(mThetaMech2, 1, mOmMech);

	if (DampingWinding == 2) {
		mPsimq = mDLmq*(mPsikq1 / mLlkq1 + mPsikq2 / mLlkq2 + mIq);
	}
	else {
		mPsimq = mDLmq*(mPsikq1 / mLlkq1 + mIq);
	}
	mPsimd = mDLmd*(mPsifd / mLlfd + mPsikd / mLlkd + mId);

}


void VoltageBehindReactanceDP::step(SystemModel& system, Real fieldVoltage, Real mechPower, Real time) {

	stepInPerUnit(system.getOmega(), system.getTimeStep(), fieldVoltage, mechPower, time, system.getNumMethod());

	mVoltageVector = mVabc*mBase_v;
	mCurrentVector = mIabc*mBase_i;

}

void VoltageBehindReactanceDP::stepInPerUnit(Real om, Real dt, Real fieldVoltage, Real mechPower, Real time, NumericalMethod numMethod) {

	mVabc <<
		mVaRe,
		mVbRe,
		mVcRe,
		mVaIm,
		mVbIm,
		mVcIm;

	mIabc <<
		mIaRe,
		mIbRe,
		mIcRe,
		mIaIm,
		mIbIm,
		mIcIm;

	mDVabc <<
		mDVaRe,
		mDVbRe,
		mDVcRe,
		mDVaIm,
		mDVbIm,
		mDVcIm;


	mMechPower = mechPower / mNomPower;
	mMechTorque = -(mMechPower / 1);

	mPsid = mLl*mId + mPsimd;
	mPsiq = mLl*mIq + mPsimq;

	//mElecTorque = (mPsimd*mIq - mPsimq*mId);
	mElecTorque = (mPsid*mIq - mPsiq*mId);

	// Euler step forward	
	mOmMech = mOmMech + dt * (1. / (2. * mH) * (mElecTorque - mMechTorque));
	
	mThetaMech = mThetaMech + dt * ((mOmMech - 1) * mBase_OmMech);
	mThetaMech2 = mThetaMech2 + dt * (mOmMech* mBase_OmMech);


	CalculateLandR(mThetaMech2, 1, mOmMech);

	if (time < 0.1 || time > 0.2)
	{
		R_load <<
			1037.8378 / mBase_Z, 0, 0, 0, 0, 0,
			0, 1037.8378 / mBase_Z, 0, 0, 0, 0,
			0, 0, 1037.8378 / mBase_Z, 0, 0, 0,
			0, 0, 0, 1037.8378 / mBase_Z, 0, 0,
			0, 0, 0, 0, 1037.8378 / mBase_Z, 0,
			0, 0, 0, 0, 0, 1037.8378 / mBase_Z;
	}
	else
	{
		R_load <<
			0.001 / mBase_Z, 0, 0, 0, 0, 0,
			0, 0.001 / mBase_Z, 0, 0, 0, 0,
			0, 0, 0.001 / mBase_Z, 0, 0, 0,
			0, 0, 0, 0.001 / mBase_Z, 0, 0,
			0, 0, 0, 0, 0.001 / mBase_Z, 0,
			0, 0, 0, 0, 0, 0.001 / mBase_Z;
	}

	mIabc = mIabc - dt*mBase_OmElec*L_VP_SFA.inverse()*(mDVabc + (R_VP_SFA + R_load)*mIabc);
	mVabc = -R_load*mIabc;

	mIaRe = mIabc(0);
	mIbRe = mIabc(1);
	mIcRe = mIabc(2);
	mIaIm = mIabc(3);
	mIbIm = mIabc(4);
	mIcIm = mIabc(5);

	mVaRe = mVabc(0);
	mVbRe = mVabc(1);
	mVcRe = mVabc(2);
	mVaIm = mVabc(3);
	mVbIm = mVabc(4);
	mVcIm = mVabc(5);

	mIq = abcToDq0Transform(mThetaMech, mIaRe, mIbRe, mIcRe, mIaIm, mIbIm, mIcIm)(0);
	mId = abcToDq0Transform(mThetaMech, mIaRe, mIbRe, mIcRe, mIaIm, mIbIm, mIcIm)(1);
	mI0 = abcToDq0Transform(mThetaMech, mIaRe, mIbRe, mIcRe, mIaIm, mIbIm, mIcIm)(2);

	if (DampingWinding == 2) {
		mPsimq = mDLmq*(mPsikq1 / mLlkq1 + mPsikq2 / mLlkq2 + mIq);
	}
	else {
		mPsimq = mDLmq*(mPsikq1 / mLlkq1 + mIq);
	}
	mPsimd = mDLmd*(mPsifd / mLlfd + mPsikd / mLlkd + mId);

	mPsikq1 = mPsikq1 - dt*mBase_OmElec*(mRkq1 / mLlkq1)*(mPsikq1 - mPsimq);
	mPsikq2 = mPsikq2 - dt*mBase_OmElec*(mRkq2 / mLlkq2)*(mPsikq2 - mPsimq);
	mPsifd = mPsifd - dt*mBase_OmElec*((mRfd / mLlfd)*(mPsifd - mPsimd) - mVfd);
	mPsikd = mPsikd - dt*mBase_OmElec*(mRkd / mLlkd)*(mPsikd - mPsimd);

	// Calculate dynamic flux likages
	if (DampingWinding == 2) {
		mDPsiq = mDLmq*(mPsikq1 / mLlkq1) + mDLmq*(mPsikq2 / mLlkq2);
	}
	else {
		mDPsiq = mDLmq*(mPsikq1 / mLlkq1);
	}
	mDPsid = mDLmd*(mPsifd / mLlfd) + mDLmd*(mPsikd / mLlkd);

	if (DampingWinding == 2) {
		mDVq = mOmMech*mDPsid + mDLmq*mRkq1*(mDPsiq - mPsikq1) / (mLlkq1*mLlkq1) +
			mDLmq*mRkq2*(mDPsiq - mPsikq2) / (mLlkq2*mLlkq2) + (mRkq1 / (mLlkq1*mLlkq1) + mRkq2 / (mLlkq2*mLlkq2))*mDLmq*mDLmq*mIq;
	}
	else {
		mDVq = mOmMech*mDPsid + mDLmq*mRkq1*(mDPsiq - mPsikq1) / (mLlkq1*mLlkq1) + (mRkq1 / (mLlkq1*mLlkq1))*mDLmq*mDLmq*mIq;
	}
	mDVd = -mOmMech*mDPsiq + mDLmd*mRkd*(mDPsid - mPsikd) / (mLlkd*mLlkd) + (mDLmd / mLlfd)*mVfd +
		mDLmd*mRfd*(mDPsid - mPsifd) / (mLlfd*mLlfd) + (mRfd / (mLlfd*mLlfd) + mRkd / (mLlkd*mLlkd))*mDLmd*mDLmd*mId;

	mDVaRe = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(0);
	mDVbRe = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(1);
	mDVcRe = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(2);
	mDVaIm = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(3);
	mDVbIm = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(4);
	mDVcIm = dq0ToAbcTransform(mThetaMech, mDVd, mDVq, 0)(5);

}


//void VoltageBehindReactanceDP::CalculateLandR(Real theta, Real omega_s, Real omega, Real time)
//{
//	Complex complex1(cos(2 * theta), sin(2 * theta));
//	Complex complex2(cos(2 * theta - 2 * omega_s*mBase_OmMech*time), sin(2 * theta - 2 * omega_s*mBase_OmMech*time));
//	Complex complex3(0, omega_s);
//	Complex complex4(0, 2 * omega + omega_s);
//	Complex complex5(0, 2 * omega - omega_s);
//
//
//	MatrixComp Rs(3,3);
//	Rs <<
//		mResistanceMat(0, 0), 0, 0,
//		0, mResistanceMat(1, 1), 0,
//		0, 0, mResistanceMat(2, 2);
//
//	A1 = LD0 + LD1*complex1;
//	B1 = LD1*complex2;
//	A2 = Rs + complex3*LD0 + complex4*LD1*complex1;
//	B2 = complex5*LD1*complex2;
//
//	L_VP_SFA <<
//		A1(0, 0).real() + B1(0, 0).real(), A1(0, 1).real() + B1(0, 1).real(), A1(0, 2).real() + B1(0, 2).real(), -A1(0, 0).imag() + B1(0, 0).imag(), -A1(0, 1).imag() + B1(0, 1).imag(), -A1(0, 2).imag() + B1(0, 2).imag(),
//		A1(1, 0).real() + B1(1, 0).real(), A1(1, 1).real() + B1(1, 1).real(), A1(1, 2).real() + B1(1, 2).real(), -A1(1, 0).imag() + B1(1, 0).imag(), -A1(1, 1).imag() + B1(1, 1).imag(), -A1(1, 2).imag() + B1(1, 2).imag(),
//		A1(2, 0).real() + B1(2, 0).real(), A1(2, 1).real() + B1(2, 1).real(), A1(2, 2).real() + B1(2, 2).real(), -A1(2, 0).imag() + B1(2, 0).imag(), -A1(2, 1).imag() + B1(2, 1).imag(), -A1(2, 2).imag() + B1(2, 2).imag(),
//		A1(0, 0).imag() + B1(0, 0).imag(), A1(0, 1).imag() + B1(0, 1).imag(), A1(0, 2).imag() + B1(0, 2).imag(), A1(0, 0).real() - B1(0, 0).real(), A1(0, 1).real() - B1(0, 1).real(), A1(0, 2).real() - B1(0, 2).real(),
//		A1(1, 0).imag() + B1(1, 0).imag(), A1(1, 1).imag() + B1(1, 1).imag(), A1(1, 2).imag() + B1(1, 2).imag(), A1(1, 0).real() - B1(1, 0).real(), A1(1, 1).real() - B1(1, 1).real(), A1(1, 2).real() - B1(1, 2).real(),
//		A1(2, 0).imag() + B1(2, 0).imag(), A1(2, 1).imag() + B1(2, 1).imag(), A1(2, 2).imag() + B1(2, 2).imag(), A1(2, 0).real() - B1(2, 0).real(), A1(2, 1).real() - B1(2, 1).real(), A1(2, 2).real() - B1(2, 2).real();
//	R_VP_SFA <<
//		A2(0, 0).real() + B2(0, 0).real(), A2(0, 1).real() + B2(0, 1).real(), A2(0, 2).real() + B2(0, 2).real(), -A2(0, 0).imag() + B2(0, 0).imag(), -A2(0, 1).imag() + B2(0, 1).imag(), -A2(0, 2).imag() + B2(0, 2).imag(),
//		A2(1, 0).real() + B2(1, 0).real(), A2(1, 1).real() + B2(1, 1).real(), A2(1, 2).real() + B2(1, 2).real(), -A2(1, 0).imag() + B2(1, 0).imag(), -A2(1, 1).imag() + B2(1, 1).imag(), -A2(1, 2).imag() + B2(1, 2).imag(),
//		A2(2, 0).real() + B2(2, 0).real(), A2(2, 1).real() + B2(2, 1).real(), A2(2, 2).real() + B2(2, 2).real(), -A2(2, 0).imag() + B2(2, 0).imag(), -A2(2, 1).imag() + B2(2, 1).imag(), -A2(2, 2).imag() + B2(2, 2).imag(),
//		A2(0, 0).imag() + B2(0, 0).imag(), A2(0, 1).imag() + B2(0, 1).imag(), A2(0, 2).imag() + B2(0, 2).imag(), A2(0, 0).real() - B2(0, 0).real(), A2(0, 1).real() - B2(0, 1).real(), A2(0, 2).real() - B2(0, 2).real(),
//		A2(1, 0).imag() + B2(1, 0).imag(), A2(1, 1).imag() + B2(1, 1).imag(), A2(1, 2).imag() + B2(1, 2).imag(), A2(1, 0).real() - B2(1, 0).real(), A2(1, 1).real() - B2(1, 1).real(), A2(1, 2).real() - B2(1, 2).real(),
//		A2(2, 0).imag() + B2(2, 0).imag(), A2(2, 1).imag() + B2(2, 1).imag(), A2(2, 2).imag() + B2(2, 2).imag(), A2(2, 0).real() - B2(2, 0).real(), A2(2, 1).real() - B2(2, 1).real(), A2(2, 2).real() - B2(2, 2).real();
//
//
//}




void VoltageBehindReactanceDP::CalculateLandR(Real theta, Real omega_s, Real omega)
{

	DPSMatrix A(3, 3);
	DPSMatrix dA(3, 3);
	DPSMatrix Re_R(3, 3);
	DPSMatrix Im_R(3, 3);
	DPSMatrix L(3, 3);

	A <<
		cos(2 * theta), cos(2 * theta - 2.*PI / 3), cos(2 * theta + 2.*PI / 3),
		cos(2 * theta - 2 * PI / 3), cos(2 * theta - 4 * PI / 3), cos(2 * theta),
		cos(2 * theta + 2 * PI / 3), cos(2 * theta), cos(2 * theta + 4 * PI / 3);

	dA <<
		sin(2 * theta), sin(2 * theta - 2.*PI / 3), sin(2 * theta + 2.*PI / 3),
		sin(2 * theta - 2 * PI / 3), sin(2 * theta - 4 * PI / 3), sin(2 * theta),
		sin(2 * theta + 2 * PI / 3), sin(2 * theta), sin(2 * theta + 4 * PI / 3);
	dA = -2 * omega*dA;

	Re_R = mResistanceMat - mLb*dA;
	Im_R = omega_s*(LD0 - mLb*A);
	L = LD0 - mLb*A;


	R_VP_SFA <<
		Re_R, -Im_R,
		Im_R, Re_R;
	L_VP_SFA <<
		L, DPSMatrix::Zero(3, 3),
		DPSMatrix::Zero(3, 3), L;

}



void VoltageBehindReactanceDP::postStep(SystemModel& system) {


}



DPSMatrix VoltageBehindReactanceDP::abcToDq0Transform(Real theta, Real aRe, Real bRe, Real cRe, Real aIm, Real bIm, Real cIm) {
	// Balanced case
	Complex alpha(cos(2. / 3. * PI), sin(2. / 3. * PI));
	Complex thetaCompInv(cos(-theta), sin(-theta));
	MatrixComp AbcToPnz(3, 3);
	AbcToPnz <<
		1, 1, 1,
		1, alpha, pow(alpha, 2),
		1, pow(alpha, 2), alpha;
	AbcToPnz = (1. / 3.) * AbcToPnz;

	MatrixComp abcVector(3, 1);
	abcVector <<
		Complex(aRe, aIm),
		Complex(bRe, bIm),
		Complex(cRe, cIm);

	MatrixComp pnzVector(3, 1);
	pnzVector = AbcToPnz * abcVector * thetaCompInv;

	DPSMatrix dq0Vector(3, 1);
	dq0Vector <<
		pnzVector(1, 0).imag(),
		pnzVector(1, 0).real(),
		0;

	return dq0Vector;
}

DPSMatrix VoltageBehindReactanceDP::dq0ToAbcTransform(Real theta, Real d, Real q, Real zero) {
	// Balanced case
	Complex alpha(cos(2. / 3. * PI), sin(2. / 3. * PI));
	Complex thetaComp(cos(theta), sin(theta));
	MatrixComp PnzToAbc(3, 3);
	PnzToAbc <<
		1, 1, 1,
		1, pow(alpha, 2), alpha,
		1, alpha, pow(alpha, 2);

	MatrixComp pnzVector(3, 1);
	pnzVector <<
		0,
		Complex(d, q),
		Complex(0, 0);

	MatrixComp abcCompVector(3, 1);
	abcCompVector = PnzToAbc * pnzVector * thetaComp;

	Matrix abcVector(6, 1);
	abcVector <<
		abcCompVector(0, 0).real(),
		abcCompVector(1, 0).real(),
		abcCompVector(2, 0).real(),
		abcCompVector(0, 0).imag(),
		abcCompVector(1, 0).imag(),
		abcCompVector(2, 0).imag();

	return abcVector;
}