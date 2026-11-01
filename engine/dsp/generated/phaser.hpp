/* ------------------------------------------------------------
license: "GPL-3.0-or-later"
name: "fxbox_phaser"
Code generated with Faust 2.70.3 (https://faust.grame.fr)
Compilation options: -a arch_minimal.cpp -lang cpp -ct 1 -cn PhaserDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __PhaserDsp_H__
#define  __PhaserDsp_H__

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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS PhaserDsp
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

static float PhaserDsp_faustpower2_f(float value) {
	return value * value;
}

class PhaserDsp : public dsp {
	
 private:
	
	FAUSTFLOAT fButton0;
	int iVec0[2];
	int fSampleRate;
	float fConst1;
	float fConst2;
	FAUSTFLOAT fHslider0;
	float fRec2[2];
	float fConst3;
	float fRec1[2];
	FAUSTFLOAT fHslider1;
	float fRec3[2];
	float fConst4;
	float fConst5;
	float fConst6;
	float fConst7;
	float fConst9;
	float fConst10;
	FAUSTFLOAT fHslider2;
	float fRec8[2];
	float fRec7[3];
	float fRec6[3];
	float fRec5[3];
	float fRec4[3];
	float fRec0[2];
	float fRec13[3];
	float fRec12[3];
	float fRec11[3];
	float fRec10[3];
	float fRec9[2];
	
 public:
	PhaserDsp() {}

	void metadata(Meta* m) { 
		m->declare("compile_options", "-a arch_minimal.cpp -lang cpp -ct 1 -cn PhaserDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("filename", "phaser.dsp");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.3.0");
		m->declare("license", "GPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.7.0");
		m->declare("name", "fxbox_phaser");
		m->declare("oscillators.lib/hs_phasor:author", "Mike Olsen, revised by Stéphane Letz");
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
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		float fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, float(fSampleRate)));
		fConst1 = 44.1f / fConst0;
		fConst2 = 1.0f - fConst1;
		fConst3 = 1.0f / fConst0;
		fConst4 = 5.0625f / fConst0;
		fConst5 = 3.375f / fConst0;
		fConst6 = 2.25f / fConst0;
		fConst7 = 1.5f / fConst0;
		float fConst8 = std::exp(-(3141.5928f / fConst0));
		fConst9 = PhaserDsp_faustpower2_f(fConst8);
		fConst10 = 2.0f * fConst8;
	}
	
	virtual void instanceResetUserInterface() {
		fButton0 = FAUSTFLOAT(0.0f);
		fHslider0 = FAUSTFLOAT(0.4f);
		fHslider1 = FAUSTFLOAT(0.7f);
		fHslider2 = FAUSTFLOAT(0.3f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec2[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fRec1[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec3[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fRec8[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 3; l5 = l5 + 1) {
			fRec7[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 3; l6 = l6 + 1) {
			fRec6[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 3; l7 = l7 + 1) {
			fRec5[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 3; l8 = l8 + 1) {
			fRec4[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec0[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 3; l10 = l10 + 1) {
			fRec13[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 3; l11 = l11 + 1) {
			fRec12[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 3; l12 = l12 + 1) {
			fRec11[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 3; l13 = l13 + 1) {
			fRec10[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 2; l14 = l14 + 1) {
			fRec9[l14] = 0.0f;
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
	
	virtual PhaserDsp* clone() {
		return new PhaserDsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("fxbox_phaser");
		ui_interface->addHorizontalSlider("depth", &fHslider1, FAUSTFLOAT(0.7f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("feedback", &fHslider2, FAUSTFLOAT(0.3f), FAUSTFLOAT(0.0f), FAUSTFLOAT(0.9f), FAUSTFLOAT(0.0001f));
		ui_interface->addButton("lforeset", &fButton0);
		ui_interface->addHorizontalSlider("rate", &fHslider0, FAUSTFLOAT(0.4f), FAUSTFLOAT(0.02f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(0.0001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		int iSlow0 = int(float(fButton0));
		float fSlow1 = fConst1 * float(fHslider0);
		float fSlow2 = fConst1 * float(fHslider1);
		float fSlow3 = fConst1 * float(fHslider2);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iVec0[0] = 1;
			fRec2[0] = fSlow1 + fConst2 * fRec2[1];
			float fTemp0 = (((1 - iVec0[1]) | iSlow0) ? 0.0f : fRec1[1] + fConst3 * fRec2[0]);
			fRec1[0] = fTemp0 - std::floor(fTemp0);
			fRec3[0] = fSlow2 + fConst2 * fRec3[1];
			float fTemp1 = 6.2831855f * (3e+03f * fRec3[0] + 1.6e+02f) + -753.98224f;
			float fTemp2 = 0.5f * fTemp1 * (1.0f - std::cos(6.2831855f * (fRec1[0] - std::floor(fRec1[0])))) + 753.98224f;
			float fTemp3 = fRec4[1] * std::cos(fConst4 * fTemp2);
			float fTemp4 = fRec5[1] * std::cos(fConst5 * fTemp2);
			float fTemp5 = fRec6[1] * std::cos(fConst6 * fTemp2);
			float fTemp6 = fRec7[1] * std::cos(fConst7 * fTemp2);
			fRec8[0] = fSlow3 + fConst2 * fRec8[1];
			fRec7[0] = float(input0[i0]) + fRec8[0] * fRec0[1] + fConst10 * fTemp6 - fConst9 * fRec7[2];
			fRec6[0] = fRec7[2] + fConst9 * (fRec7[0] - fRec6[2]) - fConst10 * (fTemp6 - fTemp5);
			fRec5[0] = fRec6[2] + fConst9 * (fRec6[0] - fRec5[2]) - fConst10 * (fTemp5 - fTemp4);
			fRec4[0] = fRec5[2] + fConst9 * (fRec5[0] - fRec4[2]) - fConst10 * (fTemp4 - fTemp3);
			fRec0[0] = fRec4[2] + fConst9 * fRec4[0] - fConst10 * fTemp3;
			output0[i0] = FAUSTFLOAT(fRec0[0]);
			float fTemp7 = 0.5f * fTemp1 * (1.0f - std::cos(6.2831855f * (fRec1[0] + (0.25f - std::floor(fRec1[0] + 0.25f))))) + 753.98224f;
			float fTemp8 = fRec10[1] * std::cos(fConst4 * fTemp7);
			float fTemp9 = fRec11[1] * std::cos(fConst5 * fTemp7);
			float fTemp10 = fRec12[1] * std::cos(fConst6 * fTemp7);
			float fTemp11 = fRec13[1] * std::cos(fConst7 * fTemp7);
			fRec13[0] = float(input1[i0]) + fRec8[0] * fRec9[1] + fConst10 * fTemp11 - fConst9 * fRec13[2];
			fRec12[0] = fRec13[2] + fConst9 * (fRec13[0] - fRec12[2]) - fConst10 * (fTemp11 - fTemp10);
			fRec11[0] = fRec12[2] + fConst9 * (fRec12[0] - fRec11[2]) - fConst10 * (fTemp10 - fTemp9);
			fRec10[0] = fRec11[2] + fConst9 * (fRec11[0] - fRec10[2]) - fConst10 * (fTemp9 - fTemp8);
			fRec9[0] = fRec10[2] + fConst9 * fRec10[0] - fConst10 * fTemp8;
			output1[i0] = FAUSTFLOAT(fRec9[0]);
			iVec0[1] = iVec0[0];
			fRec2[1] = fRec2[0];
			fRec1[1] = fRec1[0];
			fRec3[1] = fRec3[0];
			fRec8[1] = fRec8[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec0[1] = fRec0[0];
			fRec13[2] = fRec13[1];
			fRec13[1] = fRec13[0];
			fRec12[2] = fRec12[1];
			fRec12[1] = fRec12[0];
			fRec11[2] = fRec11[1];
			fRec11[1] = fRec11[0];
			fRec10[2] = fRec10[1];
			fRec10[1] = fRec10[0];
			fRec9[1] = fRec9[0];
		}
	}

};
} // namespace fxfaust

#endif
