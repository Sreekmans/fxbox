/* ------------------------------------------------------------
license: "GPL-3.0-or-later"
name: "fxbox_chorus"
Code generated with Faust 2.70.3 (https://faust.grame.fr)
Compilation options: -a arch_minimal.cpp -lang cpp -ct 1 -cn ChorusDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __ChorusDsp_H__
#define  __ChorusDsp_H__

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
#define FAUSTCLASS ChorusDsp
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


class ChorusDsp : public dsp {
	
 private:
	
	int iVec0[2];
	int fSampleRate;
	float fConst1;
	float fConst2;
	FAUSTFLOAT fHslider0;
	float fRec2[2];
	float fVec1[2];
	float fConst4;
	float fConst5;
	float fRec1[2];
	int IOTA0;
	float fVec2[8192];
	FAUSTFLOAT fButton0;
	FAUSTFLOAT fHslider1;
	float fRec4[2];
	float fConst6;
	float fRec3[2];
	FAUSTFLOAT fHslider2;
	float fRec5[2];
	float fConst7;
	float fRec0[2];
	float fVec3[2];
	float fRec7[2];
	float fVec4[8192];
	float fRec6[2];
	
 public:
	ChorusDsp() {}

	void metadata(Meta* m) { 
		m->declare("compile_options", "-a arch_minimal.cpp -lang cpp -ct 1 -cn ChorusDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("delays.lib/fdelay3:author", "Julius O. Smith III");
		m->declare("delays.lib/fdelayltv:author", "Julius O. Smith III");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.1.0");
		m->declare("filename", "chorus.dsp");
		m->declare("filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/tf1:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.3.0");
		m->declare("license", "GPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.7.0");
		m->declare("name", "fxbox_chorus");
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
		float fConst3 = 1.0f / std::tan(21991.148f / fConst0);
		fConst4 = 1.0f - fConst3;
		fConst5 = 1.0f / (fConst3 + 1.0f);
		fConst6 = 1.0f / fConst0;
		fConst7 = 0.001f * fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(0.0f);
		fButton0 = FAUSTFLOAT(0.0f);
		fHslider1 = FAUSTFLOAT(0.8f);
		fHslider2 = FAUSTFLOAT(0.5f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec2[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fVec1[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec1[l3] = 0.0f;
		}
		IOTA0 = 0;
		for (int l4 = 0; l4 < 8192; l4 = l4 + 1) {
			fVec2[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec4[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec3[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec5[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fRec0[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fVec3[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fRec7[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 8192; l11 = l11 + 1) {
			fVec4[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec6[l12] = 0.0f;
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
	
	virtual ChorusDsp* clone() {
		return new ChorusDsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("fxbox_chorus");
		ui_interface->addHorizontalSlider("depth", &fHslider2, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("feedback", &fHslider0, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(0.9f), FAUSTFLOAT(0.0001f));
		ui_interface->addButton("lforeset", &fButton0);
		ui_interface->addHorizontalSlider("rate", &fHslider1, FAUSTFLOAT(0.8f), FAUSTFLOAT(0.02f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(0.0001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = fConst1 * float(fHslider0);
		int iSlow1 = int(float(fButton0));
		float fSlow2 = fConst1 * float(fHslider1);
		float fSlow3 = fConst1 * float(fHslider2);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iVec0[0] = 1;
			fRec2[0] = fSlow0 + fConst2 * fRec2[1];
			float fTemp0 = fRec2[0] * fRec0[1];
			fVec1[0] = fTemp0;
			fRec1[0] = -(fConst5 * (fConst4 * fRec1[1] - (fTemp0 + fVec1[1])));
			float fTemp1 = float(input0[i0]) + fRec1[0];
			fVec2[IOTA0 & 8191] = fTemp1;
			fRec4[0] = fSlow2 + fConst2 * fRec4[1];
			float fTemp2 = (((1 - iVec0[1]) | iSlow1) ? 0.0f : fRec3[1] + fConst6 * fRec4[0]);
			fRec3[0] = fTemp2 - std::floor(fTemp2);
			fRec5[0] = fSlow3 + fConst2 * fRec5[1];
			float fTemp3 = fConst7 * (3.5f * fRec5[0] * (1.0f - std::cos(6.2831855f * (fRec3[0] - std::floor(fRec3[0])))) + 12.0f);
			float fTemp4 = fTemp3 + -0.999995f;
			int iTemp5 = int(fTemp4);
			float fTemp6 = std::floor(fTemp4);
			float fTemp7 = fTemp3 + (-2.0f - fTemp6);
			float fTemp8 = fTemp3 + (-1.0f - fTemp6);
			float fTemp9 = fTemp3 - fTemp6;
			float fTemp10 = fTemp9 * fTemp8;
			fRec0[0] = (fTemp3 + (-3.0f - fTemp6)) * (fTemp7 * (0.5f * fTemp9 * fVec2[(IOTA0 - std::min<int>(4096, std::max<int>(0, iTemp5 + 1))) & 8191] - 0.16666667f * fVec2[(IOTA0 - std::min<int>(4096, std::max<int>(0, iTemp5))) & 8191] * fTemp8) - 0.5f * fTemp10 * fVec2[(IOTA0 - std::min<int>(4096, std::max<int>(0, iTemp5 + 2))) & 8191]) + 0.16666667f * fTemp10 * fTemp7 * fVec2[(IOTA0 - std::min<int>(4096, std::max<int>(0, iTemp5 + 3))) & 8191];
			output0[i0] = FAUSTFLOAT(fRec0[0]);
			float fTemp11 = fRec2[0] * fRec6[1];
			fVec3[0] = fTemp11;
			fRec7[0] = -(fConst5 * (fConst4 * fRec7[1] - (fTemp11 + fVec3[1])));
			float fTemp12 = float(input1[i0]) + fRec7[0];
			fVec4[IOTA0 & 8191] = fTemp12;
			float fTemp13 = fConst7 * (3.5f * fRec5[0] * (1.0f - std::cos(6.2831855f * (fRec3[0] + (0.25f - std::floor(fRec3[0] + 0.25f))))) + 12.0f);
			float fTemp14 = fTemp13 + -0.999995f;
			int iTemp15 = int(fTemp14);
			float fTemp16 = std::floor(fTemp14);
			float fTemp17 = fTemp13 + (-2.0f - fTemp16);
			float fTemp18 = fTemp13 + (-1.0f - fTemp16);
			float fTemp19 = fTemp13 - fTemp16;
			float fTemp20 = fTemp19 * fTemp18;
			fRec6[0] = (fTemp13 + (-3.0f - fTemp16)) * (fTemp17 * (0.5f * fTemp19 * fVec4[(IOTA0 - std::min<int>(4096, std::max<int>(0, iTemp15 + 1))) & 8191] - 0.16666667f * fVec4[(IOTA0 - std::min<int>(4096, std::max<int>(0, iTemp15))) & 8191] * fTemp18) - 0.5f * fTemp20 * fVec4[(IOTA0 - std::min<int>(4096, std::max<int>(0, iTemp15 + 2))) & 8191]) + 0.16666667f * fTemp20 * fTemp17 * fVec4[(IOTA0 - std::min<int>(4096, std::max<int>(0, iTemp15 + 3))) & 8191];
			output1[i0] = FAUSTFLOAT(fRec6[0]);
			iVec0[1] = iVec0[0];
			fRec2[1] = fRec2[0];
			fVec1[1] = fVec1[0];
			fRec1[1] = fRec1[0];
			IOTA0 = IOTA0 + 1;
			fRec4[1] = fRec4[0];
			fRec3[1] = fRec3[0];
			fRec5[1] = fRec5[0];
			fRec0[1] = fRec0[0];
			fVec3[1] = fVec3[0];
			fRec7[1] = fRec7[0];
			fRec6[1] = fRec6[0];
		}
	}

};
} // namespace fxfaust

#endif
