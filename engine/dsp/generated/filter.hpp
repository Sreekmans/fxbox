/* ------------------------------------------------------------
license: "GPL-3.0-or-later"
name: "fxbox_filter"
Code generated with Faust 2.70.3 (https://faust.grame.fr)
Compilation options: -a arch_minimal.cpp -lang cpp -ct 1 -cn FilterDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __FilterDsp_H__
#define  __FilterDsp_H__

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
#define FAUSTCLASS FilterDsp
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

static float FilterDsp_faustpower4_f(float value) {
	return value * value * value * value;
}

class FilterDsp : public dsp {
	
 private:
	
	int iVec0[2];
	int fSampleRate;
	float fConst1;
	float fConst2;
	FAUSTFLOAT fHslider0;
	float fRec0[2];
	float fConst3;
	float fConst4;
	float fRec5[2];
	FAUSTFLOAT fHslider1;
	float fRec6[2];
	float fConst5;
	FAUSTFLOAT fButton0;
	FAUSTFLOAT fHslider2;
	float fRec10[2];
	float fConst6;
	float fRec9[2];
	int iRec11[2];
	float fRec8[2];
	float fConst7;
	float fRec7[2];
	float fConst8;
	FAUSTFLOAT fHslider3;
	float fConst9;
	float fRec12[2];
	float fRec13[2];
	float fRec14[2];
	float fRec15[2];
	float fRec16[2];
	FAUSTFLOAT fHslider4;
	float fRec17[2];
	FAUSTFLOAT fHslider5;
	float fRec18[2];
	float fConst10;
	FAUSTFLOAT fHslider6;
	float fRec19[2];
	float fRec1[2];
	float fRec2[2];
	FAUSTFLOAT fHslider7;
	float fRec20[2];
	float fRec21[2];
	float fRec22[2];
	float fRec23[2];
	float fRec24[2];
	float fRec25[2];
	float fRec26[2];
	float fRec27[2];
	float fRec29[2];
	float fRec30[2];
	float fRec31[2];
	float fRec34[2];
	float fRec35[2];
	float fRec36[2];
	float fRec37[2];
	
 public:
	FilterDsp() {}

	void metadata(Meta* m) { 
		m->declare("analyzers.lib/amp_follower_ar:author", "Jonatan Liljedahl, revised by Romain Michon");
		m->declare("analyzers.lib/name", "Faust Analyzer Library");
		m->declare("analyzers.lib/version", "1.2.0");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/sAndH:author", "Romain Michon");
		m->declare("basics.lib/tabulateNd", "Copyright (C) 2023 Bart Brouns <bart@magnetophon.nl>");
		m->declare("basics.lib/version", "1.12.0");
		m->declare("compile_options", "-a arch_minimal.cpp -lang cpp -ct 1 -cn FilterDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("filename", "filter.dsp");
		m->declare("filters.lib/lowpass0_highpass1", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/svf:author", "Oleg Nesterov");
		m->declare("filters.lib/svf:copyright", "Copyright (C) 2020 Oleg Nesterov <oleg@redhat.com>");
		m->declare("filters.lib/svf:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.3.0");
		m->declare("license", "GPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.7.0");
		m->declare("name", "fxbox_filter");
		m->declare("noises.lib/name", "Faust Noise Generator Library");
		m->declare("noises.lib/version", "1.4.0");
		m->declare("oscillators.lib/hs_phasor:author", "Mike Olsen, revised by Stéphane Letz");
		m->declare("oscillators.lib/name", "Faust Oscillator Library");
		m->declare("oscillators.lib/version", "1.5.0");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "1.2.0");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/onePoleSwitching:author", "Jonatan Liljedahl, revised by Dario Sanfilippo");
		m->declare("signals.lib/onePoleSwitching:licence", "STK-4.3");
		m->declare("signals.lib/version", "1.5.0");
		m->declare("vaeffects.lib/moogLadder:author", "Dario Sanfilippo");
		m->declare("vaeffects.lib/moogLadder:license", "MIT-style STK-4.3 license");
		m->declare("vaeffects.lib/name", "Faust Virtual Analog Filter Effect Library");
		m->declare("vaeffects.lib/version", "1.2.1");
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
		fConst3 = std::exp(-(6.6666665f / fConst0));
		fConst4 = std::exp(-(2.5e+02f / fConst0));
		fConst5 = std::exp(-(333.33334f / fConst0));
		fConst6 = 1.0f / fConst0;
		fConst7 = 1.0f - fConst5;
		fConst8 = std::exp(-(5e+01f / fConst0));
		fConst9 = 1.0f - fConst8;
		fConst10 = 3.1415927f / fConst0;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(0.3f);
		fHslider1 = FAUSTFLOAT(0.0f);
		fButton0 = FAUSTFLOAT(0.0f);
		fHslider2 = FAUSTFLOAT(0.5f);
		fHslider3 = FAUSTFLOAT(0.0f);
		fHslider4 = FAUSTFLOAT(0.0f);
		fHslider5 = FAUSTFLOAT(1.2e+03f);
		fHslider6 = FAUSTFLOAT(0.0f);
		fHslider7 = FAUSTFLOAT(0.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			iVec0[l0] = 0;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec0[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fRec5[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec6[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fRec10[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec9[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			iRec11[l6] = 0;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec8[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fRec7[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec12[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fRec13[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fRec14[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec15[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2; l13 = l13 + 1) {
			fRec16[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 2; l14 = l14 + 1) {
			fRec17[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = l15 + 1) {
			fRec18[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 2; l16 = l16 + 1) {
			fRec19[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 2; l17 = l17 + 1) {
			fRec1[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 2; l18 = l18 + 1) {
			fRec2[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 2; l19 = l19 + 1) {
			fRec20[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 2; l20 = l20 + 1) {
			fRec21[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 2; l21 = l21 + 1) {
			fRec22[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 2; l22 = l22 + 1) {
			fRec23[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 2; l23 = l23 + 1) {
			fRec24[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 2; l24 = l24 + 1) {
			fRec25[l24] = 0.0f;
		}
		for (int l25 = 0; l25 < 2; l25 = l25 + 1) {
			fRec26[l25] = 0.0f;
		}
		for (int l26 = 0; l26 < 2; l26 = l26 + 1) {
			fRec27[l26] = 0.0f;
		}
		for (int l27 = 0; l27 < 2; l27 = l27 + 1) {
			fRec29[l27] = 0.0f;
		}
		for (int l28 = 0; l28 < 2; l28 = l28 + 1) {
			fRec30[l28] = 0.0f;
		}
		for (int l29 = 0; l29 < 2; l29 = l29 + 1) {
			fRec31[l29] = 0.0f;
		}
		for (int l30 = 0; l30 < 2; l30 = l30 + 1) {
			fRec34[l30] = 0.0f;
		}
		for (int l31 = 0; l31 < 2; l31 = l31 + 1) {
			fRec35[l31] = 0.0f;
		}
		for (int l32 = 0; l32 < 2; l32 = l32 + 1) {
			fRec36[l32] = 0.0f;
		}
		for (int l33 = 0; l33 < 2; l33 = l33 + 1) {
			fRec37[l33] = 0.0f;
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
	
	virtual FilterDsp* clone() {
		return new FilterDsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("fxbox_filter");
		ui_interface->addHorizontalSlider("cutoff", &fHslider5, FAUSTFLOAT(1.2e+03f), FAUSTFLOAT(4e+01f), FAUSTFLOAT(1.6e+04f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("depth", &fHslider4, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("drive", &fHslider6, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("env", &fHslider1, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->addButton("lforeset", &fButton0);
		ui_interface->addHorizontalSlider("mode", &fHslider7, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(4.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("rate", &fHslider2, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.02f), FAUSTFLOAT(1e+01f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("reso", &fHslider0, FAUSTFLOAT(0.3f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("shape", &fHslider3, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(3.0f), FAUSTFLOAT(1.0f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = fConst1 * float(fHslider0);
		float fSlow1 = fConst1 * float(fHslider1);
		float fSlow2 = float(fButton0);
		int iSlow3 = fSlow2 > 0.0f;
		int iSlow4 = int(fSlow2);
		float fSlow5 = fConst1 * float(fHslider2);
		float fSlow6 = float(fHslider3);
		float fSlow7 = fConst9 * float(fSlow6 == 3.0f);
		float fSlow8 = fConst9 * float(fSlow6 == 2.0f);
		float fSlow9 = fConst9 * float(fSlow6 == 0.0f);
		float fSlow10 = fConst9 * float(fSlow6 == 1.0f);
		float fSlow11 = fConst1 * float(fHslider4);
		float fSlow12 = fConst9 * float(fHslider5);
		float fSlow13 = fConst1 * float(fHslider6);
		float fSlow14 = float(fHslider7);
		float fSlow15 = fConst9 * float(fSlow14 == 4.0f);
		float fSlow16 = fConst9 * float(fSlow14 == 3.0f);
		float fSlow17 = fConst9 * float(fSlow14 == 2.0f);
		float fSlow18 = fConst9 * float(fSlow14 == 1.0f);
		float fSlow19 = fConst9 * float(fSlow14 == 0.0f);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			iVec0[0] = 1;
			fRec0[0] = fSlow0 + fConst2 * fRec0[1];
			float fTemp0 = 14.5f * fRec0[0] + 0.5f;
			float fTemp1 = float(input1[i0]);
			float fTemp2 = float(input0[i0]);
			float fTemp3 = std::fabs(0.5f * (fTemp2 + fTemp1));
			float fTemp4 = ((fTemp3 > fRec5[1]) ? fConst4 : fConst3);
			fRec5[0] = fTemp3 * (1.0f - fTemp4) + fRec5[1] * fTemp4;
			fRec6[0] = fSlow1 + fConst2 * fRec6[1];
			fRec10[0] = fSlow5 + fConst2 * fRec10[1];
			float fTemp5 = (((1 - iVec0[1]) | iSlow4) ? 0.0f : fRec9[1] + fConst6 * fRec10[0]);
			fRec9[0] = fTemp5 - std::floor(fTemp5);
			iRec11[0] = 1103515245 * iRec11[1] + 12345;
			fRec8[0] = (((fRec9[0] < fRec9[1]) | iSlow3) ? 4.656613e-10f * float(iRec11[0]) : fRec8[1]);
			fRec7[0] = fConst7 * fRec8[0] + fConst5 * fRec7[1];
			fRec12[0] = fSlow7 + fConst8 * fRec12[1];
			fRec13[0] = fConst7 * float(2 * (fRec9[0] < 0.5f) + -1) + fConst5 * fRec13[1];
			fRec14[0] = fSlow8 + fConst8 * fRec14[1];
			fRec15[0] = fSlow9 + fConst8 * fRec15[1];
			fRec16[0] = fSlow10 + fConst8 * fRec16[1];
			fRec17[0] = fSlow11 + fConst2 * fRec17[1];
			fRec18[0] = fSlow12 + fConst8 * fRec18[1];
			float fTemp6 = std::tan(fConst10 * std::min<float>(1.8e+04f, std::max<float>(2e+01f, fRec18[0] * std::pow(2.0f, 3.0f * fRec17[0] * (fRec16[0] * (1.0f - 4.0f * std::fabs(fRec9[0] + -0.5f)) + fRec15[0] * std::sin(6.2831855f * fRec9[0]) + fRec14[0] * fRec13[0] + fRec12[0] * fRec7[0]) + 4.0f * fRec6[0] * std::min<float>(1.0f, 4.0f * fRec5[0])))));
			float fTemp7 = fTemp6 * (fTemp6 + 1.0f / fTemp0) + 1.0f;
			fRec19[0] = fSlow13 + fConst2 * fRec19[1];
			float fTemp8 = std::pow(1e+01f, 0.9f * fRec19[0]);
			float fTemp9 = std::sqrt(fTemp8);
			float fTemp10 = tanhf(fTemp2 * fTemp8) / fTemp9;
			float fTemp11 = fRec1[1] + fTemp6 * (fTemp10 - fRec2[1]);
			float fTemp12 = fTemp11 / fTemp7;
			fRec1[0] = 2.0f * fTemp12 - fRec1[1];
			float fTemp13 = fRec2[1] + fTemp6 * fTemp11 / fTemp7;
			fRec2[0] = 2.0f * fTemp13 - fRec2[1];
			float fRec3 = fTemp12;
			float fRec4 = fTemp13;
			float fTemp14 = fRec3 / fTemp0;
			fRec20[0] = fSlow15 + fConst8 * fRec20[1];
			fRec21[0] = fSlow16 + fConst8 * fRec21[1];
			fRec22[0] = fSlow17 + fConst8 * fRec22[1];
			fRec23[0] = fSlow18 + fConst8 * fRec23[1];
			float fTemp15 = 1.5f * fRec0[0] + 1.0f;
			float fTemp16 = fTemp6 + 1.0f;
			float fTemp17 = 23.0f * fRec0[0] + -0.00010678119f;
			float fTemp18 = 0.1646572f * (FilterDsp_faustpower4_f(fTemp6) * fTemp17 / FilterDsp_faustpower4_f(fTemp16)) + 1.0f;
			float fTemp19 = fTemp17 * (1.0f - fTemp6 / fTemp16);
			float fTemp20 = fTemp6 * ((fTemp10 - 0.1646572f * fTemp19 * (fRec27[1] + fTemp6 * (fRec26[1] + fTemp6 * (fRec25[1] + fTemp6 * fRec24[1] / fTemp16) / fTemp16) / fTemp16)) / fTemp18 - fRec24[1]) / fTemp16;
			fRec24[0] = fRec24[1] + 2.0f * fTemp20;
			float fTemp21 = fTemp6 * (fRec24[1] + fTemp20 - fRec25[1]) / fTemp16;
			fRec25[0] = fRec25[1] + 2.0f * fTemp21;
			float fTemp22 = fTemp6 * (fRec25[1] + fTemp21 - fRec26[1]) / fTemp16;
			fRec26[0] = fRec26[1] + 2.0f * fTemp22;
			float fTemp23 = fTemp6 * (fRec26[1] + fTemp22 - fRec27[1]) / fTemp16;
			fRec27[0] = fRec27[1] + 2.0f * fTemp23;
			float fRec28 = fRec27[1] + fTemp23;
			fRec29[0] = fSlow19 + fConst8 * fRec29[1];
			output0[i0] = FAUSTFLOAT(fRec29[0] * fRec28 * fTemp15 + fRec23[0] * fRec4 + fRec22[0] * (fTemp10 - (fRec4 + fTemp14)) + fRec3 * fRec21[0] + fRec20[0] * (fTemp10 - fTemp14));
			float fTemp24 = tanhf(fTemp1 * fTemp8) / fTemp9;
			float fTemp25 = fRec30[1] + fTemp6 * (fTemp24 - fRec31[1]);
			float fTemp26 = fTemp25 / fTemp7;
			fRec30[0] = 2.0f * fTemp26 - fRec30[1];
			float fTemp27 = fRec31[1] + fTemp6 * fTemp25 / fTemp7;
			fRec31[0] = 2.0f * fTemp27 - fRec31[1];
			float fRec32 = fTemp26;
			float fRec33 = fTemp27;
			float fTemp28 = fRec32 / fTemp0;
			float fTemp29 = fTemp6 * ((fTemp24 - 0.1646572f * fTemp19 * (fRec37[1] + fTemp6 * (fRec36[1] + fTemp6 * (fRec35[1] + fTemp6 * fRec34[1] / fTemp16) / fTemp16) / fTemp16)) / fTemp18 - fRec34[1]) / fTemp16;
			fRec34[0] = fRec34[1] + 2.0f * fTemp29;
			float fTemp30 = fTemp6 * (fRec34[1] + fTemp29 - fRec35[1]) / fTemp16;
			fRec35[0] = fRec35[1] + 2.0f * fTemp30;
			float fTemp31 = fTemp6 * (fRec35[1] + fTemp30 - fRec36[1]) / fTemp16;
			fRec36[0] = fRec36[1] + 2.0f * fTemp31;
			float fTemp32 = fTemp6 * (fRec36[1] + fTemp31 - fRec37[1]) / fTemp16;
			fRec37[0] = fRec37[1] + 2.0f * fTemp32;
			float fRec38 = fRec37[1] + fTemp32;
			output1[i0] = FAUSTFLOAT(fRec29[0] * fRec38 * fTemp15 + fRec23[0] * fRec33 + fRec22[0] * (fTemp24 - (fRec33 + fTemp28)) + fRec21[0] * fRec32 + fRec20[0] * (fTemp24 - fTemp28));
			iVec0[1] = iVec0[0];
			fRec0[1] = fRec0[0];
			fRec5[1] = fRec5[0];
			fRec6[1] = fRec6[0];
			fRec10[1] = fRec10[0];
			fRec9[1] = fRec9[0];
			iRec11[1] = iRec11[0];
			fRec8[1] = fRec8[0];
			fRec7[1] = fRec7[0];
			fRec12[1] = fRec12[0];
			fRec13[1] = fRec13[0];
			fRec14[1] = fRec14[0];
			fRec15[1] = fRec15[0];
			fRec16[1] = fRec16[0];
			fRec17[1] = fRec17[0];
			fRec18[1] = fRec18[0];
			fRec19[1] = fRec19[0];
			fRec1[1] = fRec1[0];
			fRec2[1] = fRec2[0];
			fRec20[1] = fRec20[0];
			fRec21[1] = fRec21[0];
			fRec22[1] = fRec22[0];
			fRec23[1] = fRec23[0];
			fRec24[1] = fRec24[0];
			fRec25[1] = fRec25[0];
			fRec26[1] = fRec26[0];
			fRec27[1] = fRec27[0];
			fRec29[1] = fRec29[0];
			fRec30[1] = fRec30[0];
			fRec31[1] = fRec31[0];
			fRec34[1] = fRec34[0];
			fRec35[1] = fRec35[0];
			fRec36[1] = fRec36[0];
			fRec37[1] = fRec37[0];
		}
	}

};
} // namespace fxfaust

#endif
