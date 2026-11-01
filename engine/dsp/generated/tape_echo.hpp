/* ------------------------------------------------------------
license: "GPL-3.0-or-later"
name: "fxbox_tape_echo"
Code generated with Faust 2.70.3 (https://faust.grame.fr)
Compilation options: -a arch_minimal.cpp -lang cpp -ct 1 -cn TapeEchoDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __TapeEchoDsp_H__
#define  __TapeEchoDsp_H__

/*
 * Minimal Faust architecture for FXBox.
 * Produces a self-contained C++ class (no Faust runtime headers needed on the Pi).
 * The generated class is wrapped in a namespace by the build script.
 */
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#ifndef FXBOX_FAUST_BASE
#define FXBOX_FAUST_BASE
namespace fxfaust {

struct Meta {
    virtual ~Meta() {}
    virtual void declare(const char*, const char*) {}
};

struct UI {
    virtual ~UI() {}
    virtual void openTabBox(const char*) {}
    virtual void openHorizontalBox(const char*) {}
    virtual void openVerticalBox(const char*) {}
    virtual void closeBox() {}
    virtual void addButton(const char* label, FAUSTFLOAT* zone) = 0;
    virtual void addCheckButton(const char* label, FAUSTFLOAT* zone) = 0;
    virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
    virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
    virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
    virtual void addHorizontalBargraph(const char*, FAUSTFLOAT*, FAUSTFLOAT, FAUSTFLOAT) {}
    virtual void addVerticalBargraph(const char*, FAUSTFLOAT*, FAUSTFLOAT, FAUSTFLOAT) {}
    virtual void addSoundfile(const char*, const char*, void**) {}
    virtual void declare(FAUSTFLOAT*, const char*, const char*) {}
};

struct dsp {
    virtual ~dsp() {}
    virtual int getNumInputs() = 0;
    virtual int getNumOutputs() = 0;
    virtual void buildUserInterface(UI* ui_interface) = 0;
    virtual int getSampleRate() = 0;
    virtual void init(int sample_rate) = 0;
    virtual void instanceInit(int sample_rate) = 0;
    virtual void instanceConstants(int sample_rate) = 0;
    virtual void instanceResetUserInterface() = 0;
    virtual void instanceClear() = 0;
    virtual dsp* clone() = 0;
    virtual void metadata(Meta* m) = 0;
    virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) = 0;
};

} // namespace fxfaust
#endif

namespace fxfaust {
#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

/* link with : "" */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS TapeEchoDsp
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

#if defined(_WIN32)
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

class TapeEchoDspSIG0 {
	
  private:
	
	int iVec2[2];
	int iRec4[2];
	
  public:
	
	int getNumInputsTapeEchoDspSIG0() {
		return 0;
	}
	int getNumOutputsTapeEchoDspSIG0() {
		return 1;
	}
	
	void instanceInitTapeEchoDspSIG0(int sample_rate) {
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			iVec2[l5] = 0;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			iRec4[l6] = 0;
		}
	}
	
	void fillTapeEchoDspSIG0(int count, float* table) {
		for (int i1 = 0; i1 < count; i1 = i1 + 1) {
			iVec2[0] = 1;
			iRec4[0] = (iVec2[1] + iRec4[1]) % 65536;
			table[i1] = std::sin(9.58738e-05f * float(iRec4[0]));
			iVec2[1] = iVec2[0];
			iRec4[1] = iRec4[0];
		}
	}

};

static TapeEchoDspSIG0* newTapeEchoDspSIG0() { return (TapeEchoDspSIG0*)new TapeEchoDspSIG0(); }
static void deleteTapeEchoDspSIG0(TapeEchoDspSIG0* dsp) { delete dsp; }

class TapeEchoDspSIG1 {
	
  private:
	
	int iVec4[2];
	int iRec22[2];
	
  public:
	
	int getNumInputsTapeEchoDspSIG1() {
		return 0;
	}
	int getNumOutputsTapeEchoDspSIG1() {
		return 1;
	}
	
	void instanceInitTapeEchoDspSIG1(int sample_rate) {
		for (int l25 = 0; l25 < 2; l25 = l25 + 1) {
			iVec4[l25] = 0;
		}
		for (int l26 = 0; l26 < 2; l26 = l26 + 1) {
			iRec22[l26] = 0;
		}
	}
	
	void fillTapeEchoDspSIG1(int count, float* table) {
		for (int i2 = 0; i2 < count; i2 = i2 + 1) {
			iVec4[0] = 1;
			iRec22[0] = (iVec4[1] + iRec22[1]) % 65536;
			table[i2] = std::cos(9.58738e-05f * float(iRec22[0]));
			iVec4[1] = iVec4[0];
			iRec22[1] = iRec22[0];
		}
	}

};

static TapeEchoDspSIG1* newTapeEchoDspSIG1() { return (TapeEchoDspSIG1*)new TapeEchoDspSIG1(); }
static void deleteTapeEchoDspSIG1(TapeEchoDspSIG1* dsp) { delete dsp; }

static float TapeEchoDsp_faustpower2_f(float value) {
	return value * value;
}
static float ftbl0TapeEchoDspSIG0[65536];
static float ftbl1TapeEchoDspSIG1[65536];

class TapeEchoDsp : public dsp {
	
 private:
	
	int iVec0[2];
	int fSampleRate;
	float fConst0;
	float fConst2;
	float fConst4;
	float fConst6;
	float fConst7;
	float fConst8;
	float fConst9;
	float fRec2[2];
	float fRec1[3];
	float fConst10;
	float fConst11;
	float fConst12;
	FAUSTFLOAT fHslider0;
	float fRec3[2];
	int IOTA0;
	float fVec1[262144];
	float fConst13;
	float fRec5[2];
	float fConst15;
	float fConst16;
	float fRec13[2];
	float fRec14[2];
	int iRec15[2];
	float fRec12[2];
	float fConst18;
	float fConst19;
	float fRec11[2];
	float fRec10[2];
	float fRec9[2];
	float fRec8[2];
	float fRec7[2];
	float fRec6[2];
	float fConst20;
	float fRec16[2];
	FAUSTFLOAT fHslider1;
	float fRec17[2];
	float fConst21;
	FAUSTFLOAT fHslider2;
	float fConst22;
	float fRec18[2];
	float fRec0[3];
	float fRec21[2];
	float fRec20[3];
	float fVec3[262144];
	float fRec19[3];
	
 public:
	TapeEchoDsp() {}

	void metadata(Meta* m) { 
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/tabulateNd", "Copyright (C) 2023 Bart Brouns <bart@magnetophon.nl>");
		m->declare("basics.lib/version", "1.12.0");
		m->declare("compile_options", "-a arch_minimal.cpp -lang cpp -ct 1 -cn TapeEchoDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("delays.lib/fdelay3:author", "Julius O. Smith III");
		m->declare("delays.lib/fdelayltv:author", "Julius O. Smith III");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.1.0");
		m->declare("filename", "tape_echo.dsp");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/highpass:author", "Julius O. Smith III");
		m->declare("filters.lib/highpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/nlf2:author", "Julius O. Smith III");
		m->declare("filters.lib/nlf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/nlf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.3.0");
		m->declare("license", "GPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.7.0");
		m->declare("name", "fxbox_tape_echo");
		m->declare("noises.lib/name", "Faust Noise Generator Library");
		m->declare("noises.lib/version", "1.4.0");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/version", "1.5.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/version", "1.5.0");
	}

	virtual int getNumInputs() {
		return 2;
	}
	virtual int getNumOutputs() {
		return 2;
	}
	
	static void classInit(int sample_rate) {
		TapeEchoDspSIG0* sig0 = newTapeEchoDspSIG0();
		sig0->instanceInitTapeEchoDspSIG0(sample_rate);
		sig0->fillTapeEchoDspSIG0(65536, ftbl0TapeEchoDspSIG0);
		TapeEchoDspSIG1* sig1 = newTapeEchoDspSIG1();
		sig1->instanceInitTapeEchoDspSIG1(sample_rate);
		sig1->fillTapeEchoDspSIG1(65536, ftbl1TapeEchoDspSIG1);
		deleteTapeEchoDspSIG0(sig0);
		deleteTapeEchoDspSIG1(sig1);
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, float(fSampleRate)));
		float fConst1 = std::tan(11309.733f / fConst0);
		fConst2 = 2.0f * (1.0f - 1.0f / TapeEchoDsp_faustpower2_f(fConst1));
		float fConst3 = 1.0f / fConst1;
		fConst4 = (fConst3 + -1.4142135f) / fConst1 + 1.0f;
		float fConst5 = (fConst3 + 1.4142135f) / fConst1 + 1.0f;
		fConst6 = 1.0f / fConst5;
		fConst7 = 1.0f / std::tan(282.74335f / fConst0);
		fConst8 = 1.0f - fConst7;
		fConst9 = 1.0f / (fConst7 + 1.0f);
		fConst10 = 1.2f / fConst5;
		fConst11 = 44.1f / fConst0;
		fConst12 = 1.0f - fConst11;
		fConst13 = 0.55f / fConst0;
		float fConst14 = 1.8849556f / fConst0;
		fConst15 = std::cos(fConst14);
		fConst16 = std::sin(fConst14);
		float fConst17 = 1.0f / std::tan(0.9424778f / fConst0);
		fConst18 = 1.0f - fConst17;
		fConst19 = 1.0f / (fConst17 + 1.0f);
		fConst20 = 6.3f / fConst0;
		fConst21 = std::exp(-(4.0f / fConst0));
		fConst22 = 0.001f * fConst0 * (1.0f - fConst21);
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(0.35f);
		fHslider1 = FAUSTFLOAT(0.15f);
		fHslider2 = FAUSTFLOAT(375.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec2[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 3; l2 = l2 + 1) {
			fRec1[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec3[l3] = 0.0f;
		}
		IOTA0 = 0;
		for (int l4 = 0; l4 < 262144; l4 = l4 + 1) {
			fVec1[l4] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec5[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fRec13[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec14[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			iRec15[l10] = 0;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fRec12[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec11[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2; l13 = l13 + 1) {
			fRec10[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 2; l14 = l14 + 1) {
			fRec9[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = l15 + 1) {
			fRec8[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 2; l16 = l16 + 1) {
			fRec7[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 2; l17 = l17 + 1) {
			fRec6[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 2; l18 = l18 + 1) {
			fRec16[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 2; l19 = l19 + 1) {
			fRec17[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 2; l20 = l20 + 1) {
			fRec18[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 3; l21 = l21 + 1) {
			fRec0[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 2; l22 = l22 + 1) {
			fRec21[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 3; l23 = l23 + 1) {
			fRec20[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 262144; l24 = l24 + 1) {
			fVec3[l24] = 0.0f;
		}
		for (int l27 = 0; l27 < 3; l27 = l27 + 1) {
			fRec19[l27] = 0.0f;
		}
	}
	
	virtual void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	
	virtual void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	virtual TapeEchoDsp* clone() {
		return new TapeEchoDsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("fxbox_tape_echo");
		ui_interface->addHorizontalSlider("feedback", &fHslider0, FAUSTFLOAT(0.35f), FAUSTFLOAT(0.0f), FAUSTFLOAT(0.95f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("time", &fHslider2, FAUSTFLOAT(375.0f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(2e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("wow", &fHslider1, FAUSTFLOAT(0.15f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = fConst11 * float(fHslider0);
		float fSlow1 = fConst11 * float(fHslider1);
		float fSlow2 = fConst22 * float(fHslider2);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iVec0[0] = 1;
			fRec2[0] = -(fConst9 * (fConst8 * fRec2[1] - fConst7 * (fRec0[1] - fRec0[2])));
			fRec1[0] = fRec2[0] - fConst6 * (fConst4 * fRec1[2] + fConst2 * fRec1[1]);
			fRec3[0] = fSlow0 + fConst12 * fRec3[1];
			float fTemp0 = float(input0[i0]) + 0.8333333f * fRec3[0] * tanhf(fConst10 * (fRec1[2] + fRec1[0] + 2.0f * fRec1[1]));
			fVec1[IOTA0 & 262143] = fTemp0;
			int iTemp1 = 1 - iVec0[1];
			float fTemp2 = ((iTemp1) ? 0.0f : fConst13 + fRec5[1]);
			fRec5[0] = fTemp2 - std::floor(fTemp2);
			int iTemp3 = std::max<int>(0, std::min<int>(int(65536.0f * fRec5[0]), 65535));
			float fTemp4 = ftbl0TapeEchoDspSIG0[iTemp3];
			fRec13[0] = fConst16 * fRec14[1] + fConst15 * fRec13[1];
			fRec14[0] = float(iTemp1) + fConst15 * fRec14[1] - fConst16 * fRec13[1];
			int iTemp5 = (fRec13[1] <= 0.0f) & (fRec13[0] > 0.0f);
			iRec15[0] = 1103515245 * iRec15[1] + 12345;
			fRec12[0] = fRec12[1] * float(1 - iTemp5) + 4.656613e-10f * float(iRec15[0]) * float(iTemp5);
			fRec11[0] = -(fConst19 * (fConst18 * fRec11[1] - (fRec12[0] + fRec12[1])));
			fRec10[0] = -(fConst19 * (fConst18 * fRec10[1] - (fRec11[0] + fRec11[1])));
			fRec9[0] = -(fConst19 * (fConst18 * fRec9[1] - (fRec10[0] + fRec10[1])));
			fRec8[0] = -(fConst19 * (fConst18 * fRec8[1] - (fRec9[0] + fRec9[1])));
			fRec7[0] = -(fConst19 * (fConst18 * fRec7[1] - (fRec8[0] + fRec8[1])));
			fRec6[0] = fConst11 * fRec7[0] + fConst12 * fRec6[1];
			float fTemp6 = 0.0008f * fRec6[0];
			float fTemp7 = ((iTemp1) ? 0.0f : fConst20 + fRec16[1]);
			fRec16[0] = fTemp7 - std::floor(fTemp7);
			int iTemp8 = std::max<int>(0, std::min<int>(int(65536.0f * fRec16[0]), 65535));
			float fTemp9 = ftbl0TapeEchoDspSIG0[iTemp8];
			fRec17[0] = fSlow1 + fConst12 * fRec17[1];
			fRec18[0] = fSlow2 + fConst21 * fRec18[1];
			float fTemp10 = std::max<float>(2.0f, fRec18[0] + fConst0 * fRec17[0] * (0.0003f * fTemp9 + fTemp6 + 0.003f * fTemp4));
			float fTemp11 = fTemp10 + -0.999995f;
			int iTemp12 = int(fTemp11);
			float fTemp13 = std::floor(fTemp11);
			float fTemp14 = fTemp10 + (-2.0f - fTemp13);
			float fTemp15 = fTemp10 + (-1.0f - fTemp13);
			float fTemp16 = fTemp10 - fTemp13;
			float fTemp17 = fTemp16 * fTemp15;
			fRec0[0] = (fTemp10 + (-3.0f - fTemp13)) * (fTemp14 * (0.5f * fTemp16 * fVec1[(IOTA0 - std::min<int>(131072, std::max<int>(0, iTemp12 + 1))) & 262143] - 0.16666667f * fVec1[(IOTA0 - std::min<int>(131072, std::max<int>(0, iTemp12))) & 262143] * fTemp15) - 0.5f * fTemp17 * fVec1[(IOTA0 - std::min<int>(131072, std::max<int>(0, iTemp12 + 2))) & 262143]) + 0.16666667f * fTemp17 * fTemp14 * fVec1[(IOTA0 - std::min<int>(131072, std::max<int>(0, iTemp12 + 3))) & 262143];
			output0[i0] = FAUSTFLOAT(fRec0[0]);
			fRec21[0] = -(fConst9 * (fConst8 * fRec21[1] - fConst7 * (fRec19[1] - fRec19[2])));
			fRec20[0] = fRec21[0] - fConst6 * (fConst4 * fRec20[2] + fConst2 * fRec20[1]);
			float fTemp18 = float(input1[i0]) + 0.8333333f * fRec3[0] * tanhf(fConst10 * (fRec20[2] + fRec20[0] + 2.0f * fRec20[1]));
			fVec3[IOTA0 & 262143] = fTemp18;
			float fTemp19 = std::max<float>(2.0f, fRec18[0] + fConst0 * fRec17[0] * (fTemp6 + 0.003f * (0.9463001f * ftbl1TapeEchoDspSIG1[iTemp3] - 0.32328957f * fTemp4) - 0.0003f * (0.99609464f * fTemp9 + 0.088292226f * ftbl1TapeEchoDspSIG1[iTemp8])));
			float fTemp20 = fTemp19 + -0.999995f;
			int iTemp21 = int(fTemp20);
			float fTemp22 = std::floor(fTemp20);
			float fTemp23 = fTemp19 + (-2.0f - fTemp22);
			float fTemp24 = fTemp19 + (-1.0f - fTemp22);
			float fTemp25 = fTemp19 - fTemp22;
			float fTemp26 = fTemp25 * fTemp24;
			fRec19[0] = (fTemp19 + (-3.0f - fTemp22)) * (fTemp23 * (0.5f * fTemp25 * fVec3[(IOTA0 - std::min<int>(131072, std::max<int>(0, iTemp21 + 1))) & 262143] - 0.16666667f * fVec3[(IOTA0 - std::min<int>(131072, std::max<int>(0, iTemp21))) & 262143] * fTemp24) - 0.5f * fTemp26 * fVec3[(IOTA0 - std::min<int>(131072, std::max<int>(0, iTemp21 + 2))) & 262143]) + 0.16666667f * fTemp26 * fTemp23 * fVec3[(IOTA0 - std::min<int>(131072, std::max<int>(0, iTemp21 + 3))) & 262143];
			output1[i0] = FAUSTFLOAT(fRec19[0]);
			iVec0[1] = iVec0[0];
			fRec2[1] = fRec2[0];
			fRec1[2] = fRec1[1];
			fRec1[1] = fRec1[0];
			fRec3[1] = fRec3[0];
			IOTA0 = IOTA0 + 1;
			fRec5[1] = fRec5[0];
			fRec13[1] = fRec13[0];
			fRec14[1] = fRec14[0];
			iRec15[1] = iRec15[0];
			fRec12[1] = fRec12[0];
			fRec11[1] = fRec11[0];
			fRec10[1] = fRec10[0];
			fRec9[1] = fRec9[0];
			fRec8[1] = fRec8[0];
			fRec7[1] = fRec7[0];
			fRec6[1] = fRec6[0];
			fRec16[1] = fRec16[0];
			fRec17[1] = fRec17[0];
			fRec18[1] = fRec18[0];
			fRec0[2] = fRec0[1];
			fRec0[1] = fRec0[0];
			fRec21[1] = fRec21[0];
			fRec20[2] = fRec20[1];
			fRec20[1] = fRec20[0];
			fRec19[2] = fRec19[1];
			fRec19[1] = fRec19[0];
		}
	}

};
} // namespace fxfaust

#endif
